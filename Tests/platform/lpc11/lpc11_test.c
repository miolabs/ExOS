#include <CMSIS/LPC11xx.h>
#include <kernel/thread.h>

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

	LED_PORT->DIR |= LED1_MASK | LED2_MASK;
	while(1)
	{
		exos_thread_sleep(1000);
		LED_PORT->MASKED_ACCESS[LED1_MASK] = LED1_MASK;
		LED_PORT->MASKED_ACCESS[LED2_MASK] = 0;
		exos_thread_sleep(1000);
		LED_PORT->MASKED_ACCESS[LED1_MASK] = 0;
		LED_PORT->MASKED_ACCESS[LED2_MASK] = LED2_MASK;
	}
}
