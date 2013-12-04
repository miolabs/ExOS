#include "aci.h"
#include <kernel/thread.h>
#include <kernel/event.h>
#include <support/board_hal.h>
#include <support/ssp_hal.h>

#define THREAD_STACK 1536
static EXOS_THREAD _thread;
static unsigned char _stack[THREAD_STACK] __attribute__((aligned(16)));
static void *_service(void *arg);

static void _rdy_handler(int pin);
static EXOS_EVENT _rdy_event;

#define ACI_GPIO_RDY 0x100
#define ACI_SSP_MODULE 0

void aci_initialize()
{
	exos_event_create(&_rdy_event);
	exos_thread_create(&_thread, 1, _stack, THREAD_STACK, NULL, _service, NULL);
}

static void _rdy_handler(int pin)
{
	exos_event_reset(&_rdy_event);
}

static void *_service(void *arg)
{
	hal_board_set_handler(ACI_GPIO_RDY, HAL_INT_FALLING_EDGE, &_rdy_handler);
	hal_ssp_initialize(ACI_SSP_MODULE, 100000, HAL_SSP_MODE_SPI, 0);

	while(1)
	{
		if (!exos_event_wait(&_rdy_event, 1000))
		{
			continue;
		}

		//hal_ssp_transmit(ACI_SSP_MODULE, unsigned char *outbuf, unsigned char *inbuf, int length);
	}
}
