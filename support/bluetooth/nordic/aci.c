#include "aci.h"
#include <kernel/thread.h>
#include <kernel/port.h>
#include <support/gpio_hal.h>
#include <support/ssp_hal.h>
#include "aci_support.h"

#define THREAD_STACK 1024
static EXOS_THREAD _thread;
static unsigned char _stack[THREAD_STACK] __attribute__((aligned(16)));
static void *_service(void *arg);

#define ACI_CMD_RESPONSE_TIMEOUT 500

static EXOS_PORT _service_port;
static ACI_REQUEST *_pending_request;
static EXOS_EVENT _rdy_event;
static EXOS_EVENT _connected_event;
static EXOS_EVENT _advertising_event;
static ACI_DEVICE_STATE _state;

#if defined BOARD_OLIMEX_P1XXX

#define ACI_GPIO_RDY_PORT 1
#define ACI_GPIO_RDY_PIN 0
#define ACI_GPIO_REQ_PORT 1
#define ACI_GPIO_REQ_PIN 1
#define ACI_GPIO_RESET_PORT 2
#define ACI_GPIO_RESET_PIN 9
#define ACI_SSP_MODULE 1

#elif defined BOARD_XKUTY_CPU1

#define ACI_GPIO_RDY_PORT 1
#define ACI_GPIO_RDY_PIN 4
#define ACI_GPIO_REQ_PORT 1
#define ACI_GPIO_REQ_PIN 10
#define ACI_GPIO_RESET_PORT 0
#define ACI_GPIO_RESET_PIN 2
#define ACI_SSP_MODULE 0

#elif defined BOARD_BTSMART

#define ACI_GPIO_RDY_PORT 1
#define ACI_GPIO_RDY_PIN 0
#define ACI_GPIO_REQ_PORT 1
#define ACI_GPIO_REQ_PIN 1
#define ACI_GPIO_RESET_PORT 1
#define ACI_GPIO_RESET_PIN 10
#define ACI_SSP_MODULE 1

#endif

#define ACI_GPIO_RDY_MASK (1<<ACI_GPIO_RDY_PIN)
#define ACI_GPIO_REQ_MASK (1<<ACI_GPIO_REQ_PIN)
#define ACI_GPIO_RESET_MASK (1<<ACI_GPIO_RESET_PIN)

void aci_initialize()
{
	hal_gpio_config(ACI_GPIO_RDY_PORT, ACI_GPIO_RDY_MASK, 0);
    hal_gpio_write(ACI_GPIO_REQ_PORT, ACI_GPIO_REQ_MASK, ACI_GPIO_REQ_MASK);	// release REQN
	hal_gpio_config(ACI_GPIO_REQ_PORT, ACI_GPIO_REQ_MASK, ACI_GPIO_REQ_MASK);

	exos_port_create(&_service_port, NULL);
	_state = ACI_STATE_RESET;
	_pending_request = NULL;

	// reset NRF800x
    hal_gpio_write(ACI_GPIO_RESET_PORT, ACI_GPIO_RESET_MASK, ACI_GPIO_RESET_MASK);
	hal_gpio_config(ACI_GPIO_RESET_PORT, ACI_GPIO_RESET_MASK, ACI_GPIO_RESET_MASK);
    hal_gpio_write(ACI_GPIO_RESET_PORT, ACI_GPIO_RESET_MASK, 0);
	exos_thread_sleep(1);
    hal_gpio_write(ACI_GPIO_RESET_PORT, ACI_GPIO_RESET_MASK, ACI_GPIO_RESET_MASK);

	exos_event_create(&_rdy_event);
	exos_event_create(&_connected_event);
	exos_event_create(&_advertising_event);
	exos_thread_create(&_thread, 1, _stack, THREAD_STACK, NULL, _service, NULL);

	// wait for setup event / send configuration
	exos_event_wait(&_rdy_event, 1000);
	if (aci_support_setup())
	{
		_state = ACI_STATE_STANDBY;
		exos_event_reset(&_rdy_event);
	}
}

static void _rdy_handler(int port, int pin)
{
	exos_event_set(&_rdy_event);
}

static void _complete_request(ACI_REQUEST *req, int length, ACI_COMMAND_RESPONSE_EVENT_DATA *data)
{
	if (req->State == ACI_REQUEST_PENDING && 
		req->Command == data->CommandOpCode)
	{
		req->Status = data->Status;
		for (int i = 0; i < (length - 2); i++) 
			req->Data[i] = data->ResponseData[i];

		exos_event_set(&req->Done);
		req->State = ACI_REQUEST_DONE;
	}
}

static void _received(unsigned char *buffer, int length)
{
	int offset = 0;
    ACI_EVENT ev = (ACI_EVENT)buffer[offset++];

	switch(ev)
	{
		case 0:
			// NOP
			break;
		case ACI_EVENT_DEVICE_STARTED:
			if (_state == ACI_STATE_RESET)
				_state = ACI_STATE_SETUP;
			break;
		case ACI_EVENT_COMMAND_RESPONSE:
			if (_pending_request != NULL)
				_complete_request(_pending_request, length - offset, 
					(ACI_COMMAND_RESPONSE_EVENT_DATA *)&buffer[offset]);
			break;
		case ACI_EVENT_CONNECTED:
			// TODO: check contents
			exos_event_set(&_connected_event);
			break;
		case ACI_EVENT_DISCONNECTED:
			// TODO: check contents
			exos_event_reset(&_connected_event);
			break;
		case ACI_EVENT_PIPE_STATUS:
			// TODO: find existing service/char in hal ---------------------------------
			break;
		default:
			// TODO
			break;
	}
	// TODO
}

static unsigned char _rbit8(unsigned char c)
{
	unsigned char r = 0;
	unsigned char m = 0x80;
	while(c != 0)
	{
		if (c & 1) r |= m;
		c >>= 1;
		m >>= 1;
	}
	return r;
}

static void *_service(void *arg)
{
	EXOS_EVENT *events[] = { &_service_port.Event, &_rdy_event };
	unsigned char buffer[32];
	int payload, cmd_len;

	exos_thread_sleep(63);
	hal_gpio_set_handler(ACI_GPIO_RDY_PORT, ACI_GPIO_RDY_PIN, HAL_GPIO_INT_FALLING_EDGE, &_rdy_handler);
	hal_ssp_initialize(ACI_SSP_MODULE, 2000000, HAL_SSP_MODE_SPI, 0);
	if (hal_gpio_read(ACI_GPIO_RDY_PORT, ACI_GPIO_RDY_MASK) == 0) exos_event_set(&_rdy_event);

	while(1)
	{
		if (-1 == exos_event_wait_multiple(events, 2, 1000))	// FIXME: change to TIMEOUT_NEVER
			continue;

		ACI_REQUEST *req = (ACI_REQUEST *)exos_port_get_message(&_service_port, 0);
		if (req != NULL)
		{
			//TODO: cancel message if wrong state

			cmd_len = req->Length;
			buffer[0] = _rbit8(cmd_len + 1);
			buffer[1] = _rbit8(req->Command);
			hal_gpio_write(ACI_GPIO_REQ_PORT, ACI_GPIO_REQ_MASK, 0);	// assert REQN
			exos_event_wait(&_rdy_event, EXOS_TIMEOUT_NEVER);
		}
		else if (_rdy_event.State)
		{
			buffer[0] = buffer[1] = cmd_len = 0;
			hal_gpio_write(ACI_GPIO_REQ_PORT, ACI_GPIO_REQ_MASK, 0);	// assert REQN
		}
		else continue;

		hal_ssp_transmit(ACI_SSP_MODULE, buffer, buffer, 2);

		int resp_len = _rbit8(buffer[1]);
		payload = (resp_len > cmd_len) ? resp_len : cmd_len;
		if (payload > 31) payload = 31;
		
		if (req != NULL)
		{
			for(int i = 0; i < payload; i++) buffer[i] = _rbit8(req->Data[i]); 
			hal_ssp_transmit(ACI_SSP_MODULE, buffer, buffer, payload);
			req->State = ACI_REQUEST_PENDING;
			_pending_request = req;
		}
		else
		{
			for(int i = 0; i < payload; i++) buffer[i] = 0; 
			hal_ssp_transmit(ACI_SSP_MODULE, buffer, buffer, payload);
		}

		for(int i = 0; i < resp_len; i++) buffer[i] = _rbit8(buffer[i]);
		_received(buffer, resp_len);

		exos_event_reset(&_rdy_event);	// NOTE: this shall stay after _received() because it may change state
		hal_gpio_write(ACI_GPIO_REQ_PORT, ACI_GPIO_REQ_MASK, ACI_GPIO_REQ_MASK);	// release REQN	
	}
}

static int _do_request(ACI_REQUEST *req)
{
	req->State = ACI_REQUEST_QUEUED;
	exos_event_create(&req->Done);
	exos_port_send_message(&_service_port, (EXOS_MESSAGE *)req);
	exos_event_wait(&req->Done, EXOS_TIMEOUT_NEVER);
	return (req->State == ACI_REQUEST_DONE);
}

int aci_send_setup(ACI_REQUEST *req, int *pcomplete)
{
	if (_state == ACI_STATE_SETUP)
	{
		if (_do_request(req))
		{
			ACI_STATUS_CODE status = req->Status;
			switch(status)
			{
				case ACI_STATUS_TRANSACTION_COMPLETE:
					*pcomplete = 1;
				case ACI_STATUS_TRANSACTION_CONTINUE:
					return 1;
			}
		}
	}
	return 0;
}

static int _start_advertising(ACI_COMMAND cmd, unsigned short adv_interval)
{
	adv_interval += ((adv_interval * 6) / 10);	// ms -> 0.625 ms
	if (adv_interval < 32) adv_interval = 32;

	ACI_REQUEST req;
	req.Command = cmd; 
	req.Length = sizeof(ACI_CONNECT_COMMAND_DATA);
	*((ACI_CONNECT_COMMAND_DATA *)&req.Data) = (ACI_CONNECT_COMMAND_DATA) { 
		.Timeout = 0, .AdvInterval = adv_interval };

	if (_state == ACI_STATE_STANDBY)
	{
		if (_do_request(&req))
		{
			ACI_STATUS_CODE status = req.Status;
			return (status == ACI_STATUS_SUCCESS);
		}
	}
	return 0;
}

int aci_broadcast(unsigned short adv_interval)
{
	return _start_advertising(ACI_COMMAND_BROADCAST, adv_interval);
}

int aci_connect(unsigned short adv_interval)
{
	return _start_advertising(ACI_COMMAND_CONNECT, adv_interval);
}

int aci_connect_wait(unsigned short adv_interval, unsigned int timeout)
{
	if (aci_connect(adv_interval))
	{
		exos_event_wait(&_connected_event, timeout);
		return _connected_event.State;
	}
	return 0;
}
