#include <support/bluetooth/ble/server.h>
#include "aci.h"

void ble_hal_initialize()
{
	aci_initialize();
}

EXOS_BLE_ERROR ble_hal_connect(EXOS_BLE_SERVICE *services[], unsigned int count, unsigned int adv_interval)
{
	if (adv_interval > 32767) adv_interval = 32767;
	ACI_STATUS_CODE status = aci_connect(adv_interval, 0);
	return (status == ACI_STATUS_SUCCESS) ? EXOS_BLE_OK : 
		((status == ACI_STATUS_ERROR_BOND_REQUIRED) ? EXOS_BLE_ERROR_BOND_REQUIRED : EXOS_BLE_ERROR_REJECTED);
}

EXOS_BLE_ERROR ble_hal_bond(EXOS_BLE_SERVICE *services[], unsigned int count, unsigned int adv_interval)
{
	if (adv_interval > 32767) adv_interval = 32767;
	ACI_STATUS_CODE status = aci_bond(adv_interval, 60);	// FIXME: define bond timeout seconds
	return (status == ACI_STATUS_SUCCESS) ? EXOS_BLE_OK : EXOS_BLE_ERROR_REJECTED;
}

EXOS_BLE_ERROR ble_hal_disconnect()
{
	ACI_STATUS_CODE status = aci_reset();	// FIXME: we call reset instead of disconnect because we want bond status to be cleared
	return (status == ACI_STATUS_SUCCESS) ? EXOS_BLE_OK : EXOS_BLE_ERROR_REJECTED;
}

int ble_hal_server_is_connected()
{
	int done = aci_is_connected();
	return done;
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

