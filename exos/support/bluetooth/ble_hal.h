#ifndef EXOS_BLE_HAL_H
#define EXOS_BLE_HAL_H

#include <support/bluetooth/ble/server.h>

// hal interface
void ble_hal_initialize();
int ble_hal_set_bond_callback(EXOS_BLE_BOND_CALLBACK callback);
EXOS_BLE_ERROR ble_hal_connect(EXOS_BLE_SERVICE *services[], unsigned int count, unsigned int adv_interval);
EXOS_BLE_ERROR ble_hal_bond(EXOS_BLE_SERVICE *services[], unsigned int count, unsigned int adv_interval);
EXOS_BLE_ERROR ble_hal_disconnect();
int ble_hal_server_is_connected();
EXOS_BLE_ERROR ble_hal_set_local_data(EXOS_BLE_CHAR *characteristic, void *data);



#endif // EXOS_BLE_HAL_H


