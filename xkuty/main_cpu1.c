#include <CMSIS/LPC11xx.h>
#include <kernel/thread.h>
#include <kernel/port.h>
#include <kernel/timer.h>
#include <support/can_hal.h>
#include "xcpu.h"

#if defined BOARD_MIORELAY1

#define OUTPUT_PORT LPC_GPIO2
#define HEADL_MASK (1<<6)
#define TAILL_MASK (1<<7)
#define OUTPUT_MASK (HEADL_MASK | TAILL_MASK)

#define LED_PORT LPC_GPIO3
#define LED_MASK (1<<0)

#elif defined BOARD_XKUTYCPU1

#define OUTPUT_PORT LPC_GPIO2
#define HEADL_MASK (1<<7)
#define TAILL_MASK (1<<0)
#define BRAKEL_MASK (1<<6)
#define HORN_MASK (1<<8)
#define SENSOREN_MASK (1<<10)
#define OUTPUT_MASK (HEADL_MASK | TAILL_MASK | BRAKEL_MASK | HORN_MASK | SENSOREN_MASK)

#define LED_PORT LPC_GPIO3
#define LED_MASK (1<<0)

#define INPUT_PORT LPC_GPIO3
#define INPUT_STOP (1<<1)
#define INPUT_ERROR_OUTPUT (1<<2)
#define INPUT_ERROR_SENSOR (1<<3)

#else
#error "Unsupported board"
#endif

static const CAN_EP _eps[] = { {0x200, 0}, {0x201, 0} };
static int _can_setup(int index, CAN_EP *ep, CAN_MSG_FLAGS *pflags, void *state);
static EXOS_PORT _can_rx_port;
static EXOS_FIFO _can_free_msgs;
#define CAN_MSG_QUEUE 10
static XCPU_MSG _can_msg[CAN_MSG_QUEUE];

static EXOS_TIMER _timer;
static void _run_diag();

static enum
{
	OUTPUT_NONE = 0,
	OUTPUT_HEADL = (1<<0),
	OUTPUT_TAILL = (1<<1),
	OUTPUT_BRAKEL = (1<<2),
	OUTPUT_HORN = (1<<3),
} _output_state;

static enum
{
	CONTROL_OFF = 0,
	CONTROL_ON,
	CONTROL_CRUISE,
} _control_state;
static XCPU_STATE _state; // comm state to share (with lcd)


void main()
{
	int result;
	LED_PORT->DIR |= LED_MASK;
	LED_PORT->MASKED_ACCESS[LED_MASK] = 0;	// led on

	OUTPUT_PORT->DIR |= OUTPUT_MASK;
	OUTPUT_PORT->MASKED_ACCESS[OUTPUT_MASK] = 0;	// FIXME: HEADL should be left active

	exos_timer_create(&_timer, 100, 100, exos_signal_alloc());

	exos_port_create(&_can_rx_port, NULL);
	exos_fifo_create(&_can_free_msgs, NULL);
	for(int i = 0; i < CAN_MSG_QUEUE; i++) exos_fifo_queue(&_can_free_msgs, (EXOS_NODE *)&_can_msg[i]);

	hal_can_initialize(0, 250000);
	hal_fullcan_setup(_can_setup, NULL);

#if defined BOARD_MIORELAY1
	// enable CAN term
	LPC_GPIO2->DIR |= 1<<8;
	LPC_GPIO2->MASKED_ACCESS[1<<8] = 1<<8;
#endif

	OUTPUT_PORT->MASKED_ACCESS[TAILL_MASK] = TAILL_MASK;
	OUTPUT_PORT->MASKED_ACCESS[BRAKEL_MASK] = BRAKEL_MASK;
	OUTPUT_PORT->MASKED_ACCESS[BRAKEL_MASK | TAILL_MASK] = 0;

	OUTPUT_PORT->MASKED_ACCESS[HEADL_MASK] = HEADL_MASK;
	OUTPUT_PORT->MASKED_ACCESS[HEADL_MASK] = 0;

	OUTPUT_PORT->MASKED_ACCESS[HORN_MASK] = HORN_MASK;
	exos_thread_sleep(100);
	OUTPUT_PORT->MASKED_ACCESS[HORN_MASK] = 0;
	exos_thread_sleep(100);
	OUTPUT_PORT->MASKED_ACCESS[HORN_MASK] = HORN_MASK;
	exos_thread_sleep(100);
	OUTPUT_PORT->MASKED_ACCESS[HORN_MASK] = 0;

	_control_state = CONTROL_OFF;
	_output_state = OUTPUT_NONE;
	_state = XCPU_STATE_OFF;

	int speed = 0;
	int km = 0;
	int batt = 0;
	int push = 0;
	while(1)
	{
		exos_timer_wait(&_timer);



//		if (0 == exos_event_wait(&_can_event, 100))
//		{
//			// TODO
//		}
//		else
		{
			LED_PORT->MASKED_ACCESS[LED_MASK] = ~LED_PORT->DATA;
			// background processing?

			speed = (speed + 1) & 255;
			km++;
			batt = (km / 3) & 255;
			CAN_BUFFER buf;
			buf.u8[0] = speed;
			buf.u8[1] = batt;
			buf.u32[1] = km;
			hal_can_send((CAN_EP) { .Id = 0x300 }, &buf, 8, CANF_NONE);
		}

		switch(_control_state)
		{
			case CONTROL_OFF:
				_output_state = 0;
				push = !(INPUT_PORT->DATA & INPUT_STOP) ? push++ : 0;
				if (push > 500)
				{
					push = 0;
					_output_state = OUTPUT_HEADL | OUTPUT_TAILL;
					_control_state = CONTROL_ON;
					_state = XCPU_STATE_ON;

					_run_diag();
				}
				break;
			case CONTROL_CRUISE:
			case CONTROL_ON:
				// TODO: update brake and throttle out

				push = !(INPUT_PORT->DATA & INPUT_STOP) ? push++ : 0;
				if (push > 500)
				{
					push = 500;
					if (speed == 0)
					{
						push = 0;
						_output_state = OUTPUT_NONE;
						_control_state = CONTROL_OFF;
						_state = XCPU_STATE_OFF;
					}
				}
				// TODO: check battery and engine
				break;				
		}

		OUTPUT_PORT->MASKED_ACCESS[HEADL_MASK] = (_output_state & OUTPUT_HEADL) ? HEADL_MASK : 0;
		OUTPUT_PORT->MASKED_ACCESS[TAILL_MASK] = (_output_state & OUTPUT_TAILL) ? TAILL_MASK : 0;
		OUTPUT_PORT->MASKED_ACCESS[BRAKEL_MASK] = (_output_state & OUTPUT_BRAKEL) ? BRAKEL_MASK : 0;
		OUTPUT_PORT->MASKED_ACCESS[HORN_MASK] = (_output_state & OUTPUT_HORN) ? HORN_MASK : 0;
		

	}
}

void hal_can_received_handler(int index, CAN_MSG *msg)
{
	switch(msg->EP.Id)
	{
		case 0x200:
//			_relay_state = msg->Data.u8[0];
			break;
		case 0x201:
			// TODO
			break;
	}

	//exos_event_reset(&_can_event);
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

static void _run_diag()
{
}
