#include <CMSIS/LPC11xx.h>
#include <kernel/thread.h>
#include <kernel/event.h>
#include <support/can_hal.h>

#if defined BOARD_MIORELAY1

#define OUTPUT_PORT LPC_GPIO2
#define HEADL_MASK (1<<6)
#define TAILL_MASK (1<<7)

#define LED_PORT LPC_GPIO3
#define LED_MASK (1<<0)

#elif defined BOARD_XKUTYCPU1

#define OUTPUT_PORT LPC_GPIO2
#define HEADL_MASK (1<<7)
#define TAILL_MASK (1<<0)

#define LED_PORT LPC_GPIO3
#define LED_MASK (1<<0)

#else
#error "Unsupported board"
#endif

static const CAN_EP _eps[] = { {0x200, 0}, {0x201, 0} };

static int _can_setup(int index, CAN_EP *ep, CAN_MSG_FLAGS *pflags, void *state);
static EXOS_EVENT _can_event;
static unsigned char _relay_state = 0;

void main()
{
	int result;

	exos_event_create(&_can_event);
	hal_can_initialize(0, 250000);
	hal_fullcan_setup(_can_setup, NULL);

#if defined BOARD_MIORELAY1
	// enable CAN term
	LPC_GPIO2->DIR |= 1<<8;
	LPC_GPIO2->MASKED_ACCESS[1<<8] = 1<<8;
#endif

	LED_PORT->DIR |= LED_MASK;
	LED_PORT->MASKED_ACCESS[LED_MASK] = 0;	// led on

	OUTPUT_PORT->DIR |= HEADL_MASK | TAILL_MASK;
	OUTPUT_PORT->MASKED_ACCESS[HEADL_MASK | TAILL_MASK] = 0;	// FIXME: HEADL should be left active
	while(1)
	{
		if (0 == exos_event_wait(&_can_event, 100))
		{
			// TODO
		}
		else
		{
			LED_PORT->MASKED_ACCESS[LED_MASK] = ~LED_PORT->DATA;
			// background processing?

			CAN_BUFFER buf = (CAN_BUFFER) { 1, 2, 3, 4, 5, 6, 7, 8 };	// TODO
			hal_can_send((CAN_EP) { .Id = 0x300 }, &buf, 8, CANF_NONE);
		}

		OUTPUT_PORT->MASKED_ACCESS[HEADL_MASK] = (_relay_state & (1<<0)) ? HEADL_MASK : 0;
		OUTPUT_PORT->MASKED_ACCESS[TAILL_MASK] = (_relay_state & (1<<1)) ? TAILL_MASK : 0;
	}
}

void hal_can_received_handler(int index, CAN_MSG *msg)
{
	switch(msg->EP.Id)
	{
		case 0x200:
			_relay_state = msg->Data.u8[0];
			break;
		case 0x201:
			// TODO
			break;
	}

	exos_event_reset(&_can_event);
}

static int _can_setup(int index, CAN_EP *ep, CAN_MSG_FLAGS *pflags, void *state)
{
	int count = sizeof(_eps) / sizeof(CAN_EP);
	if (index < count)
	{
		*ep = _eps[index];
		*pflags = CANF_RXINT;
		return 1;
	}
	return 0;
}
