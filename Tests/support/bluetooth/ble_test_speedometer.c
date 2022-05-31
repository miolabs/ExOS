#include <support/bluetooth/ble/server.h>
#include <support/board_hal.h>
#include <kernel/thread.h>
#include "ble_can.h"
#include <kernel/signal.h>
#include <kernel/timer.h>

static EXOS_BLE_SERVICE _service;
static const EXOS_BLE_UUID _service_uuid = BLE_SERVICE_UUID(0xE805);
static EXOS_BLE_CHAR _tx_char;
static const EXOS_BLE_UUID _tx_char_uuid = BLE_CHAR_UUID(0xE806);
static EXOS_BLE_CHAR _report_char;
static const EXOS_BLE_UUID _report_char_uuid = BLE_CHAR_UUID(0xE807);

static EXOS_BLE_ATT_ERROR _handler(EXOS_BLE_SERVICE *service, EXOS_BLE_REQUEST *req);
static void _connect();
static void _update(BLE_CAN_REPORT_DATA *data);

static enum { STATE_DISCONNECTED = 0, STATE_CONNECTING, STATE_CONNECTED } _state;

void main()
{
#ifdef BOARD_XKUTY_CPU1
	ble_can_initialize();
#endif
	exos_ble_server_initialize(NULL);

	exos_ble_service_create(&_service, &_service_uuid, _handler, EXOS_BLE_SERVICE_PRIMARY);
	exos_ble_characteristic_create(&_tx_char, &_tx_char_uuid, 11);
	exos_ble_service_add_char(&_service, &_tx_char);
	exos_ble_characteristic_create(&_report_char, &_report_char_uuid, 12);
	exos_ble_service_add_char(&_service, &_report_char);

	exos_ble_server_add_service(&_service);

	_connect();

	while(1)
	{
#ifdef BOARD_XKUTY_CPU1
		BLE_CAN_REPORT_DATA data;
                hal_led_set(0, 1);
                exos_thread_sleep(1000);
                hal_led_set(0, 0);
                if(ble_can_read_data(&data))
                {
                  _update(&data);
                }

#else
		exos_thread_sleep(1000);
#endif
	}
}

static void _connect()
{
	EXOS_BLE_SERVICE *services[] = { &_service };
	int done = exos_ble_server_start_advertising(services, 1, 20, EXOS_BLECF_NONE);
	_state = (done == EXOS_BLE_OK) ? STATE_CONNECTING : STATE_DISCONNECTED;
}

static void _update(BLE_CAN_REPORT_DATA *data)
{
	switch(_state)
	{
		case STATE_DISCONNECTED:
			_connect();
			break;
		case STATE_CONNECTED:
			if (data != NULL)
				exos_ble_update_characteristic(&_report_char, data);
			break;
	}
}

static EXOS_BLE_ATT_ERROR _handler(EXOS_BLE_SERVICE *service, EXOS_BLE_REQUEST *req)
{
	switch(req->Event)
	{
		case EXOS_BLE_EVENT_CONNECT:
			_state = STATE_CONNECTED;
			hal_led_set(0, 1);
			return EXOS_BLE_ATT_OK;
		case EXOS_BLE_EVENT_DISCONNECT:
			_state = STATE_DISCONNECTED;
			hal_led_set(0, 0);
			return EXOS_BLE_ATT_OK;
		case EXOS_BLE_EVENT_WRITE_CHARACTERISTIC:
#ifdef BOARD_XKUTY_CPU1
			ble_can_transmit((BLE_CAN_TRANSMIT_DATA *)req->Data);
#endif
			return EXOS_BLE_ATT_OK;
		default:
			break;
	}
	return EXOS_BLE_ATT_NOT_IMPLEMENTED;
}

