#include "ble_can.h"
#include <support/misc/can_receiver.h>

static CAN_HANDLER _handler;

void ble_can_initialize()
{
	hal_can_initialize(0, 125000, CAN_INITF_NONE);
	can_receiver_initialize();

	can_receiver_add_handler(&_handler, 0, 0, 0);
}

int ble_can_read_data(BLE_CAN_REPORT_DATA *data)
{
	CAN_MSG msg;
	static unsigned seq = 0;

	int done = can_receiver_read(&_handler, &msg, 1000);
	if (done)
	{
		data->Sequence = seq++;
		data->Id[0] = msg.EP.Id >> 16;
		data->Id[1] = msg.EP.Id >> 8;
		data->Id[2] = msg.EP.Id;
		for (int i = 0; i < 8; i++) 
			data->Data[i] = i < msg.Length ? msg.Data.u8[i] : 0;
	}
	return done;
}

void ble_can_transmit(BLE_CAN_TRANSMIT_DATA *data)
{
	CAN_EP ep = (CAN_EP) { .Id = (data->Id[0] << 16) | (data->Id[1] << 8) | data->Id[0] };
	CAN_BUFFER buf;
	for (int i = 0; i < 8; i++) buf.u8[i] = data->Data[i];
	hal_can_send(ep, &buf, 8, CANF_PRI_ANY);
}


