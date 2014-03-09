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
		return EXOS_BLE_ERROR_CHARACTERISTIC_ALREADY_ADDED;
	
	characteristic->Service = service;
	list_add_tail(&service->Characteristics, (EXOS_NODE *)characteristic);
	return EXOS_BLE_OK;
}

static void _renum_chars()
{
	int index = 0;
	exos_mutex_lock(&_services_lock);
	FOREACH(node, &_services)
	{
		EXOS_BLE_SERVICE *service = (EXOS_BLE_SERVICE *)node;
		FOREACH(char_node, &service->Characteristics)
		{
			EXOS_BLE_CHAR *characteristic = (EXOS_BLE_CHAR *)char_node;
			characteristic->Index = index++;
		}
	}
	exos_mutex_unlock(&_services_lock);
}

EXOS_BLE_ERROR exos_ble_server_start_advertising(EXOS_BLE_SERVICE *services[], unsigned int count, unsigned int adv_interval)
{
	_renum_chars();

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
	if (characteristic == NULL || data == NULL)
		return EXOS_BLE_ERROR_CHARACTERISTIC_INVALID;
	
	// TODO: use appropriate call for char type
	return ble_hal_set_local_data(characteristic, data);
}

void exos_ble_send_event_to_all_services(EXOS_BLE_EVENT event)
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

static EXOS_BLE_CHAR *_find_char(int index)
{
	EXOS_BLE_CHAR *found = NULL;
	exos_mutex_lock(&_services_lock);
	FOREACH(node, &_services)
	{
		EXOS_BLE_SERVICE *service = (EXOS_BLE_SERVICE *)node;
		FOREACH(char_node, &service->Characteristics)
		{
			EXOS_BLE_CHAR *characteristic = (EXOS_BLE_CHAR *)char_node;
			if (characteristic->Index == index)
			{
				found = characteristic;
				break;
			}
		}
	}
	exos_mutex_unlock(&_services_lock);
	return found;
}

void exos_ble_handle_received_data(int pipe, unsigned char *data, int data_length)	// FIXME
{
	EXOS_BLE_CHAR *characteristic = _find_char(pipe - 1);	// FIXME
	if (characteristic != NULL)
	{
		EXOS_BLE_REQUEST req = (EXOS_BLE_REQUEST) { .Event = EXOS_BLE_EVENT_WRITE_CHARACTERISTIC, 
			.Characteristic = characteristic, 
			.Data = data, .Length = data_length };
		EXOS_BLE_SERVICE *service = characteristic->Service;
		if (service->Handler != NULL)			
			service->Handler(service, &req);
	}
}



