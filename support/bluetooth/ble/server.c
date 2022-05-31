#include "server.h"
#include <support/bluetooth/ble_hal.h>
#include <kernel/panic.h>
#include <kernel/mutex.h>

static EXOS_LIST _services;
static EXOS_MUTEX _services_lock;
static EXOS_BLE_BOND_CALLBACK _bond_callback = NULL;

void exos_ble_server_initialize(EXOS_BLE_BOND_CALLBACK callback)
{
	list_initialize(&_services);
	exos_mutex_create(&_services_lock);

	ble_hal_initialize();
	_bond_callback = callback;
}

void exos_ble_bond_event(EXOS_BLE_BOND_DATA *data)
{
	if (_bond_callback != NULL)
		_bond_callback(data);
}

int exos_ble_server_is_advertising()
{
	// TODO
}

int exos_ble_server_is_connected()
{
	return ble_hal_server_is_connected();
}

int exos_ble_server_wait_connexion(int timeout)
{
	// TODO
}

void exos_ble_service_create(EXOS_BLE_SERVICE *service, const EXOS_BLE_UUID *uuid, EXOS_BLE_SERVICE_HANDLER handler, EXOS_BLE_SERVICE_FLAGS flags)
{
	*service = (EXOS_BLE_SERVICE) {
		.UUID = uuid,
		.Handler = handler,
		.Flags = flags & EXOS_BLE_SERVICE_CREATE_MASK };
	list_initialize(&service->Characteristics);
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

void exos_ble_server_add_service(EXOS_BLE_SERVICE *service)
{
	exos_mutex_lock(&_services_lock);
#ifdef DEBUG
	if (list_find_node(&_services, (EXOS_NODE *)service))
		kernel_panic(KERNEL_ERROR_LIST_ALREADY_CONTAINS_NODE);
#endif
	list_add_tail(&_services, (EXOS_NODE *)service);

    _renum_chars();
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

EXOS_BLE_ERROR exos_ble_server_start_advertising(EXOS_BLE_SERVICE *services[], unsigned int count, unsigned int adv_interval, EXOS_BLE_CONNECT_FLAGS flags)
{
	if (services == NULL) kernel_panic(KERNEL_ERROR_NULL_POINTER);

	EXOS_BLE_ERROR error;
	if (flags & EXOS_BLECF_BOND) 
	{
		error = ble_hal_bond(services, count, adv_interval);
	}
	else 
	{
		error = ble_hal_connect(services, count, adv_interval);
	}
	return error;
}

EXOS_BLE_ERROR exos_ble_server_disconnect()
{
	EXOS_BLE_ERROR error = ble_hal_disconnect();	
	return error;
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



