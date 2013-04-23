#include <support/lcd/lcd.h>
#include <support/adc_hal.h>
#include <kernel/thread.h>
#include <kernel/event.h>
#include <support/can_hal.h>

static int _can_setup(int index, CAN_EP *ep, CAN_MSG_FLAGS *pflags, void *state);

static const CAN_EP _eps[] = {
	{0x300, LCD_CAN_BUS}, {0x301, LCD_CAN_BUS} };

static EXOS_EVENT _can_event;

void main()
{
	exos_event_create(&_can_event);
	hal_can_initialize(LCD_CAN_BUS, 250000);
	hal_fullcan_setup(_can_setup, NULL);

	lcd_initialize();
	lcdcon_gpo_backlight(1);

	hal_adc_initialize(1000, 16);
	while(1)
	{
		if (0 == exos_event_wait(&_can_event, 100))
		{
		}
		else
		{
			unsigned short ain[6];
			for(int i = 0; i < 6; i++)
				ain[i] = hal_adc_read(i);

			unsigned char relays = 0;
			if (ain[3] < 0x8000) relays |= (1<<0);
			if (ain[4] < 0x8000) relays |= (1<<1);

			CAN_BUFFER buf = (CAN_BUFFER) { relays, 2, 3, 4, 5, 6, 7, 8 };
			hal_can_send((CAN_EP) { .Id = 0x200, .Bus = LCD_CAN_BUS }, &buf, 8, CANF_PRI_ANY);
		}

		// TODO: show inputs
	}
}

void hal_can_received_handler(int index, CAN_MSG *msg)
{
	// TODO: process msg
	switch(msg->EP.Id)
	{
		case 0x300:
//			RELAY_PORT->MASKED_ACCESS[RELAY1] = (msg->Data.u8[0] & (1<<0)) ? RELAY1 : 0;
//			RELAY_PORT->MASKED_ACCESS[RELAY2] = (msg->Data.u8[0] & (1<<1)) ? RELAY2 : 0;
			break;
	}

	exos_event_reset(&_can_event);
}

static int _can_setup(int index, CAN_EP *ep, CAN_MSG_FLAGS *pflags, void *state)
{
	int count = sizeof(_eps) / sizeof(CAN_EP);
	if (index < count)
	{
		*pflags = CANF_RXINT;
		*ep = _eps[index];
		return 1;
	}
	return 0;
}



