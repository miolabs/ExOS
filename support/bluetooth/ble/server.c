#include "server.h"
#include <kernel/panic.h>
#include <kernel/mutex.h>

static EXOS_LIST _services;
static EXOS_MUTEX _services_lock;

void exos_ble_server_initialize()
{
	list_initialize(&_services);
	exos_mutex_create(&_services_lock);

	ble_hal_initialize();
}

void exos_ble_service_create(EXOS_BLE_SERVICE *service, const EXOS_BLE_UUID *uuid, EXOS_BLE_SERVICE_HANDLER handler, EXOS_BLE_SERVICE_FLAGS flags)
{
	*service = (EXOS_BLE_SERVICE) {
		.UUID = uuid,
		.Handler = handler,
		.Flags = flags & EXOS_BLE_SERVICE_CREATE_MASK };
	list_initialize(&service->Characteristics);
}

void exos_ble_server_add_service(EXOS_BLE_SERVICE *service)
{
	exos_mutex_lock(&_services_lock);
#ifdef DEBUG
	if (list_find_node(&_services, (EXOS_NODE *)service))
		kernel_panic(KERNEL_ERROR_LIST_ALREADY_CONTAINS_NODE);
#endif
	list_add_tail(&_services, (EXOS_NODE *)service);
	exos_mutex_unlock(&_services_lock);
}

EXOS_BLE_ERROR exos_ble_service_add_char(EXOS_BLE_SERVICE *service, EXOS_BLE_CHAR *characteristic)
{
	if (characteristic == NULL) kernel_panic(KERNEL_ERROR_NULL_POINTER);
	if (characteristic->Service != NULL)
		return EXOS_BLE_ERROR_CHRACTERISTIC_ALREADY_ADDED;
	
	characteristic->Service = service;
	list_add_tail(&service->Characteristics, (EXOS_NODE *)characteristic);
	return EXOS_BLE_OK;
}

EXOS_BLE_ERROR exos_ble_server_start_advertising(EXOS_BLE_SERVICE *services[], unsigned int count, unsigned int adv_interval)
{
	if (services == NULL) kernel_panic(KERNEL_ERROR_NULL_POINTER);
	return ble_hal_start_advertising(services, count, adv_interval);
}

void exos_ble_characteristic_create(EXOS_BLE_CHAR *characteristic, const EXOS_BLE_UUID *uuid, unsigned short size)
{
	*characteristic = (EXOS_BLE_CHAR) {
		.UUID = uuid,
		.Size = size };
}

EXOS_BLE_ERROR exos_ble_update_characteristic(EXOS_BLE_CHAR *characteristic, void *data)
{
	
}


static void _send_request_to_all_services(EXOS_BLE_EVENT event)
{
	EXOS_BLE_REQUEST req = (EXOS_BLE_REQUEST) { .Event = event };
	exos_mutex_lock(&_services_lock);
	FOREACH(node, &_services)
	{
		EXOS_BLE_SERVICE *service = (EXOS_BLE_SERVICE *)node;
		if (service->Handler != NULL)			
			service->Handler(service, &req);
	}
	exos_mutex_unlock(&_services_lock);	
}



