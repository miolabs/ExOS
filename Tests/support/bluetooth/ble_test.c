#include <support/bluetooth/ble/server.h>
#include <kernel/thread.h>

static EXOS_BLE_SERVICE _service;
static const EXOS_BLE_UUID _service_uuid = BLE_SERVICE_UUID(0x1234);
static EXOS_BLE_CHAR _state_char;

static EXOS_BLE_ATT_ERROR _handler(EXOS_BLE_SERVICE *service, EXOS_BLE_REQUEST *req);

void main()
{
	exos_ble_server_initialize();

	exos_ble_service_create(&_service, &_service_uuid, _handler, EXOS_BLE_SERVICE_PRIMARY);
	
	// TODO: add characteristics

	exos_ble_server_add_service(&_service);

	EXOS_BLE_SERVICE *services[] = { &_service };
	int done = exos_ble_server_start_advertising(services, 1, 20);

	while(1)
	{
		exos_thread_sleep(1000);
	}
}

static EXOS_BLE_ATT_ERROR _handler(EXOS_BLE_SERVICE *service, EXOS_BLE_REQUEST *req)
{
	switch(req->Event)
	{
		default:
			break;
	}
	return EXOS_BLE_ATT_NOT_IMPLEMENTED;
}

