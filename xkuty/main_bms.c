#include <CMSIS/LPC11xx.h>
#include <kernel/thread.h>
#include <support/adc_hal.h>
#include <support/can_hal.h>
#include <support/misc/pca9635.h>
#include "bms/measure.h"
#include "xbms.h"

#if defined BOARD_MIORELAY1
#define LED1_MASK (1<<6)
#define LED2_MASK (1<<7)
#define LED_PORT LPC_GPIO2
#elif defined BOARD_BMS01
#define LED1_MASK (1<<9)
#define LED2_MASK 0
#define LED_PORT LPC_GPIO2
#else 
#error "Unsupported board"
#endif


void main()
{
	int result;
	pca9635_initialize(PCA9635_MODE_INVERT | PCA9635_MODE_TOTEMPOLE | PCA9635_MODE_OUTNE_2);
	
	measure_initialize();

	hal_adc_initialize(10000, 12);
	hal_can_initialize(0, 250000, CAN_INITF_DISABLE_RETRANSMISSION);

	CAN_BUFFER buffer;

	LPC_GPIO3->DIR |= 0xf;
	LPC_GPIO2->DIR |= (1<<5);
	LPC_GPIO2->MASKED_ACCESS[1<<5] = 1<<5;

	float an[16]; 
	LED_PORT->DIR |= LED1_MASK | LED2_MASK;
	while(1)
	{
		exos_thread_sleep(500);
		LED_PORT->MASKED_ACCESS[LED1_MASK] = LED1_MASK;
		LED_PORT->MASKED_ACCESS[LED2_MASK] = 0;
		exos_thread_sleep(500);
		LED_PORT->MASKED_ACCESS[LED1_MASK] = 0;
		LED_PORT->MASKED_ACCESS[LED2_MASK] = LED2_MASK;
		
		float total = 0;
		for(int inp = 0; inp < 12; inp++)
		{
			LPC_GPIO3->MASKED_ACCESS[0xF] = inp;
			total += an[inp] = hal_adc_read(1) * (3.3f/65535) * 1.64f;
		}

		float vin = hal_adc_read(0) * (3.3f/65535) * 1;
		buffer.u32[0] = (unsigned long)(vin * 65536);
		buffer.u32[1] = (unsigned long)(total * 65536);

        hal_can_send((CAN_EP) { .Id = 0x330 }, &buffer, 8, CANF_PRI_ANY);

		measure_update();
		buffer.u32[0] = measure_vbat();
		buffer.u32[1] = measure_current();
        hal_can_send((CAN_EP) { .Id = 0x331 }, &buffer, 8, CANF_PRI_ANY);
	}
}
