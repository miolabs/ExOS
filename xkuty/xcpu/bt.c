#include "bt.h"

#include <support/bluetooth/ble/server.h>
#include <kernel/thread.h>

static EXOS_BLE_SERVICE _service;
static const EXOS_BLE_UUID _service_uuid = BLE_SERVICE_UUID(0x1234);	// FIXME
static EXOS_BLE_CHAR _state_char;
static const EXOS_BLE_UUID _state_char_uuid = BLE_CHAR_UUID(0x1234);	// FIXME

static XCPU_BT_STATE _state = XCPU_BT_DISCONNECTED;
static int _wait = 0;
#define XCPU_BT_REFRESH_PERIOD 10

static EXOS_BLE_ATT_ERROR _handler(EXOS_BLE_SERVICE *service, EXOS_BLE_REQUEST *req);

int xcpu_bt_initialize()
{
#if defined BOARD_XKUTY_CPU1_EXTENDED
	exos_ble_server_initialize();

	exos_ble_service_create(&_service, &_service_uuid, _handler, EXOS_BLE_SERVICE_PRIMARY);
	exos_ble_characteristic_create(&_state_char, &_state_char_uuid, sizeof(XCPU_BT_STATE_CHAR_DATA));
	exos_ble_service_add_char(&_service, &_state_char);

	exos_ble_server_add_service(&_service);

	EXOS_BLE_SERVICE *services[] = { &_service };
	int done = exos_ble_server_start_advertising(services, 1, 20);
#endif

	return 1;
}

static EXOS_BLE_ATT_ERROR _handler(EXOS_BLE_SERVICE *service, EXOS_BLE_REQUEST *req)
{
	switch(req->Event)
	{
		case EXOS_BLE_EVENT_CONNECT:
			_state = XCPU_BT_CONNECTED;
			return EXOS_BLE_ATT_OK;
		case EXOS_BLE_EVENT_DISCONNECT:
			_state = XCPU_BT_DISCONNECTED;
			return EXOS_BLE_ATT_OK;
		default:
			break;
	}
	return EXOS_BLE_ATT_NOT_IMPLEMENTED;
}

int xcpu_bt_update(XCPU_BT_STATE_CHAR_DATA *data)
{
	if (_wait != 0) 
	{
		_wait--;
		return 0;
	}
	_wait = XCPU_BT_REFRESH_PERIOD;

	EXOS_BLE_SERVICE *services[] = { &_service };
	int done;

	switch(_state)
	{
		case XCPU_BT_DISCONNECTED:
			_state = XCPU_BT_CONNECTING;
			done = exos_ble_server_start_advertising(services, 1, 20);
			break;
		case XCPU_BT_CONNECTED:
			exos_ble_update_characteristic(&_state_char, data);
			break;
	}
}


