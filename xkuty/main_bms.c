#include <CMSIS/LPC11xx.h>
#include <kernel/thread.h>

#include <support/can_hal.h>
#include "bms/measure.h"
#include "bms/balancer.h"
#include "bms/switch.h"
#include "xbms.h"

#ifndef VCHG_MIN
#define VCHG_MIN 12000
#endif

#ifndef VBAT_MIN
#define VBAT_MIN (2750 * CELL_COUNT)
#endif
#ifndef VBAT_MAX
#define VBAT_MAX (4200 * CELL_COUNT)
#endif

#if defined BOARD_BMS01
#define LED1_MASK (1<<9)
#define LED_PORT LPC_GPIO2
#else 
#error "Unsupported board"
#endif

static void _panic(int count)
{
	while(1)
	{
		for (int i = 0; i < count; i++)
		{
			exos_thread_sleep(250);
			LED_PORT->MASKED_ACCESS[LED1_MASK] = 0;
			exos_thread_sleep(250);
			LED_PORT->MASKED_ACCESS[LED1_MASK] = LED1_MASK;
		}
	
		exos_thread_sleep(1000);
	}
}

void main()
{
	LED_PORT->DIR |= LED1_MASK;
	LED_PORT->MASKED_ACCESS[LED1_MASK] = LED1_MASK;
	
	if (!switch_initialize()) _panic(2);
	if (!balancer_initialize()) _panic(3);
	if (!measure_initialize()) _panic(5);

//	hal_can_initialize(0, 250000, CAN_INITF_DISABLE_RETRANSMISSION);

	CAN_BUFFER buffer;
	XBMS_STATE state = XBMS_STATE_ON;
	int iter_count = 0;
	int cell;
	while(1)
	{
		exos_thread_sleep(500);
		LED_PORT->MASKED_ACCESS[LED1_MASK] = LED1_MASK;
		exos_thread_sleep(500);
		LED_PORT->MASKED_ACCESS[LED1_MASK] = 0;
		
		XBMS_STATUS status = XBMS_STATUS_IDLE;
		MEASURE_ERROR err = measure_update();
		switch(err)
		{
			case MEASURE_SUPERVISOR_FAILURE: 
				status |= XBMS_STATUS_ERROR | XBMS_STATUS_SUPERVISOR_FAILURE;
				break;
			case MEASURE_VIN_MISMATCH:
				status |= XBMS_STATUS_ERROR | XBMS_STATUS_VIN_SENSE_MISMATCH;
				break;
		}

		int vchg = measure_vchg();
		if (vchg > VCHG_MIN)
			status |= XBMS_STATUS_CHARGER_PRESENT;
		int vbat = measure_vbat();
		if (vbat < VBAT_MIN)
			status |= XBMS_STATUS_LOW_BATTERY;
		else if (vbat > VBAT_MAX)
			status |= XBMS_STATUS_FULL;

		switch(state)
		{
			case XBMS_STATE_OFF:
				// TODO: disable stay_en
				
				if (iter_count < 3) iter_count++;
				else state = XBMS_STATE_ON;
				break;
			case XBMS_STATE_ON:
				if (status & XBMS_STATUS_CHARGER_PRESENT)
				{
					switch_charger(1);
					state = XBMS_STATE_CHARGE;
				}
				break;
			case XBMS_STATE_CHARGE:
				if (!(status & XBMS_STATUS_CHARGER_PRESENT))
				{
					switch_charger(0);
					state = XBMS_STATE_ON;
				}
				else if (status & XBMS_STATUS_FULL)
				{
					switch_charger(0);
					state = XBMS_STATE_CHARGE_FULL;
				}
				else
				{
					if (measure_cell_balance(&cell))
					{
						balancer_on(cell);
						state = XBMS_STATE_CHARGE_BALANCE;
						iter_count = 3;
					}
				}
				break;
			case XBMS_STATE_CHARGE_BALANCE:
				if (state & XBMS_STATUS_FULL)
				{
					switch_charger(0);
					state = XBMS_STATE_CHARGE_FULL;
				}
				else if (iter_count > 0) iter_count--;
				else
				{
					balancer_off();
					state = XBMS_STATE_CHARGE;
				}
				break;
			case XBMS_STATE_CHARGE_FULL:
				if (!(status & XBMS_STATUS_CHARGER_PRESENT))
				{
					state = XBMS_STATE_ON;
				}
				else if (!(status & XBMS_STATUS_FULL))
				{
					switch_charger(1);
					state = XBMS_STATE_CHARGE;
				}
				else
				{
					if (measure_cell_balance(&cell))
					{
						balancer_on(cell);
						state = XBMS_STATE_FULL_BALANCE;
						iter_count = 3;
					}
				}
				break;
			case XBMS_STATE_FULL_BALANCE:
				if (!(state & XBMS_STATUS_FULL))
				{
					switch_charger(1);
					state = XBMS_STATE_CHARGE_BALANCE;
				}
				else if (iter_count > 0) iter_count--;
				else
				{
					balancer_off();
					state = XBMS_STATE_CHARGE_FULL;
				}
				break;
		}

//		buffer.u32[0] = (unsigned long)(vbat * 65536);
//		buffer.u32[1] = measure_current();
//        hal_can_send((CAN_EP) { .Id = 0x330 }, &buffer, 8, CANF_PRI_ANY);
//
//
//		buffer.u32[0] = measure_vbat();
//		buffer.u32[1] = 
//        hal_can_send((CAN_EP) { .Id = 0x331 }, &buffer, 8, CANF_PRI_ANY);
	}
}
