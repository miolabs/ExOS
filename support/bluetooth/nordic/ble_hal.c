#include <support/bluetooth/ble/server.h>
#include "aci.h"

void ble_hal_initialize()
{
	aci_initialize();
}

EXOS_BLE_ERROR ble_hal_start_advertising(EXOS_BLE_SERVICE *services[], unsigned int count, unsigned int adv_interval)
{
	//int done = aci_broadcast(adv_interval > 32767 ? 32767 : adv_interval);
	int done = aci_connect(adv_interval > 32767 ? 32767 : adv_interval);
	return done ? EXOS_BLE_OK : EXOS_BLE_ERROR_REJECTED;
}

int exos_ble_server_is_advertising()
{
	// TODO
}

int exos_ble_server_is_connected()
{
	int done = aci_is_connected();
	return done;
}

int exos_ble_server_wait_connexion(int timeout)
{
	// TODO
}

EXOS_BLE_ERROR ble_hal_set_local_data(EXOS_BLE_CHAR *characteristic, void *data)
{
	int done = aci_is_connected();
	if (done)
	{
		done = aci_send_data(characteristic->Index + 1, // TODO: translate char->Index to pipe number
			data, characteristic->Size);
	}
	else
	{
		done = aci_set_local_data(characteristic->Index + 1, // TODO: translate char->Index to pipe number
			data, characteristic->Size);
	}
	return done ? EXOS_BLE_OK : EXOS_BLE_ERROR_REJECTED; // FIXME
}

