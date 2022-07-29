#ifndef BLE_TEST_CAN_H
#define BLE_TEST_CAN_H

typedef struct
{
	unsigned char Id[3];
	unsigned char Data[8];
} BLE_CAN_TRANSMIT_DATA;

typedef struct
{
	unsigned char Sequence;
	unsigned char Id[3];
	unsigned char Data[8];
} BLE_CAN_REPORT_DATA;

// prototypes
void ble_can_initialize();
void ble_can_transmit(BLE_CAN_TRANSMIT_DATA *data);
int ble_can_read_data(BLE_CAN_REPORT_DATA *data);

#endif // BLE_TEST_CAN_H


