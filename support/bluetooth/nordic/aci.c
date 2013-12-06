#include "aci.h"
#include <kernel/thread.h>
#include <kernel/fifo.h>
#include <support/gpio_hal.h>
#include <support/ssp_hal.h>

#define THREAD_STACK 1024
static EXOS_THREAD _thread;
static unsigned char _stack[THREAD_STACK] __attribute__((aligned(16)));
static void *_service(void *arg);

static EXOS_EVENT _cmd_event;
static EXOS_FIFO _cmd_fifo;
static void _rdy_handler(int pin);
static EXOS_EVENT _rdy_event;

#define ACI_GPIO_RDY_PORT 1
#define ACI_GPIO_RDY_PIN 0
#define ACI_GPIO_RDY_MASK (1<<ACI_GPIO_RDY_PIN)
#define ACI_GPIO_REQ_PORT 1
#define ACI_GPIO_REQ_MASK (1<<1)
#define ACI_SSP_MODULE 1

void aci_initialize()
{
	hal_gpio_config(ACI_GPIO_RDY_PORT, ACI_GPIO_RDY_MASK, 0);
    hal_gpio_write(ACI_GPIO_REQ_PORT, ACI_GPIO_REQ_MASK, ACI_GPIO_REQ_MASK);	// release REQN
	hal_gpio_config(ACI_GPIO_REQ_PORT, ACI_GPIO_REQ_MASK, ACI_GPIO_REQ_MASK);

	exos_event_create(&_cmd_event);
	exos_fifo_create(&_cmd_fifo, NULL);

	exos_event_create(&_rdy_event);
	exos_thread_create(&_thread, 1, _stack, THREAD_STACK, NULL, _service, NULL);

	// TODO: wait for setup event / send configuration
}

static void _rdy_handler(int pin)
{
	exos_event_set(&_rdy_event);
}

static void _received(unsigned char *buffer, int length)
{
}

static void *_service(void *arg)
{
	EXOS_EVENT *events[] = { &_cmd_event, &_rdy_event };
	unsigned char buffer[32];
	int payload, cmd_len;

	hal_gpio_set_handler(ACI_GPIO_RDY_PORT, ACI_GPIO_RDY_PIN, HAL_INT_FALLING_EDGE, &_rdy_handler);
	hal_ssp_initialize(ACI_SSP_MODULE, 100000, HAL_SSP_MODE_SPI, 0);

	while(1)
	{
		if (!exos_event_wait_multiple(events, 2, 1000))
		{
			continue;
		}

		ACI_COMMAND *cmd = (ACI_COMMAND *)exos_fifo_dequeue(&_cmd_fifo);
		if (cmd != NULL)
		{
			buffer[0] = cmd_len = cmd->Length;
			buffer[1] = cmd->Command;
		}
		else
		{
			buffer[0] = buffer[1] = cmd_len = 0;
		}
		hal_ssp_transmit(ACI_SSP_MODULE, buffer, buffer, 2);

		int resp_len = buffer[1];
		payload = (resp_len > cmd_len) ? resp_len : cmd_len;
		if (payload > 32) payload = 32;
		
		if (cmd != NULL)
			hal_ssp_transmit(ACI_SSP_MODULE, cmd->Data, buffer, payload);
		else
		{
			for(int i = 0; i < payload; i++) buffer[i] = 0; 
			hal_ssp_transmit(ACI_SSP_MODULE, buffer, buffer, payload);
		}

		_received(buffer, resp_len);

		exos_event_reset(&_rdy_event);
		
		if (!_cmd_event.State)
			hal_gpio_write(ACI_GPIO_REQ_PORT, ACI_GPIO_REQ_MASK, ACI_GPIO_REQ_MASK);	// release REQN
	}
}
