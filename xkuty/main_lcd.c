#include "xdisplay.h"
#include "xcpu.h"
#include "xanalog.h"
#include "event_recording.h"

#include <kernel/thread.h>
#include <kernel/event.h>
#include <kernel/timer.h>
#include <support/can_hal.h>
#include <support/apple/iap.h>
#include <support/lcd/lcd.h>
#include <usb/host.h>

#include <stdio.h>
#include <assert.h>

#define MAIN_LOOP_TIME 20
#define RESEND_COUNTER 25

static int _can_setup(int index, CAN_EP *ep, CAN_MSG_FLAGS *pflags, void *state);

static const CAN_EP _eps[] = {{0x300, LCD_CAN_BUS}, {0x301, LCD_CAN_BUS} };

static DASH_DATA _dash = { .ActiveConfig = { .DriveMode = XCPU_DRIVE_MODE_SOFT, .ThrottleMin = 0, .ThrottleMax = 0 }};

static const EVREC_CHECK _maintenance_screen_access[] =
{
	{BRAKE_FRONT_MASK | BRAKE_REAR_MASK | CRUISE_MASK, CHECK_PRESSED},
	{BRAKE_FRONT_MASK | BRAKE_REAR_MASK | CRUISE_MASK, CHECK_RELEASED},
	{BRAKE_FRONT_MASK, CHECK_PRESSED},
    {BRAKE_REAR_MASK | CRUISE_MASK, CHECK_RELEASED},
	{BRAKE_FRONT_MASK, CHECK_PRESSED},
	{BRAKE_FRONT_MASK, CHECK_RELEASED},
	{0x00000000, CHECK_END},
};

static const EVREC_CHECK _mode_screen_access[] =
{
	{BRAKE_REAR_MASK | BRAKE_FRONT_MASK, CHECK_RELEASED},
	{BRAKE_REAR_MASK | BRAKE_FRONT_MASK, CHECK_PRESSED},
	{BRAKE_REAR_MASK | BRAKE_FRONT_MASK, CHECK_RELEASED},
	{BRAKE_REAR_MASK | BRAKE_FRONT_MASK, CHECK_PRESSED},
	{BRAKE_REAR_MASK | BRAKE_FRONT_MASK, CHECK_RELEASED},
	{0x00000000, CHECK_END},
};

static int _configuring = 0;
static int _adj_up = 0, _adj_down = 0;
static int _switch_units_cnt = 0, _adj_throttle_cnt = 0, _adj_drive_mode_cnt = 0;

static XCPU_BUTTONS _read_send_inputs(int state)
{
	xanalog_update();
	XCPU_BUTTONS buttons = xanalog_read_digital();
	if ( _configuring)
		buttons |= XCPU_BUTTON_CONFIGURING;
	if (_adj_up)
		buttons |= XCPU_BUTTON_ADJUST_UP;
	if (_adj_down)
		buttons |= XCPU_BUTTON_ADJUST_DOWN;

	if ( _switch_units_cnt > 0)
		buttons |= XCPU_BUTTON_SWITCH_UNITS, _switch_units_cnt--;
	if ( _adj_throttle_cnt > 0)
        buttons |= XCPU_BUTTON_ADJ_THROTTLE, _adj_throttle_cnt--;
	if ( _adj_drive_mode_cnt > 0)
		buttons |= _dash.Temp.DriveMode << XCPU_BUTTON_ADJ_DRIVE_MODE_SHIFT, _adj_drive_mode_cnt--;

	// Record inputs for sequence triggering (to start debug services)
	event_record(buttons & (XCPU_BUTTON_THROTTLE_OPEN |
					XCPU_BUTTON_BRAKE_REAR | XCPU_BUTTON_BRAKE_FRONT |
					XCPU_BUTTON_CRUISE | XCPU_BUTTON_HORN));

	ANALOG_INPUT *ain_throttle = xanalog_input(THROTTLE_IDX);
	unsigned int throttle = ain_throttle->Scaled;

	ANALOG_INPUT *ain_brake_rear = xanalog_input(BRAKE_REAR_IDX);
	ANALOG_INPUT *ain_brake_front = xanalog_input(BRAKE_FRONT_IDX);

	CAN_BUFFER buf = (CAN_BUFFER) { buttons & 0xff, 
									throttle & 0xff, // Throttle low
									throttle >> 8,	// Throttle high
									ain_brake_rear->Scaled >> 4,
									ain_brake_front->Scaled >> 4, 
									_dash.Temp.ThrottleMin >> 4, 
									_dash.Temp.ThrottleMax >> 4,
									buttons >> 8 };
	hal_can_send((CAN_EP) { .Id = 0x200, .Bus = LCD_CAN_BUS }, &buf, 8, CANF_PRI_ANY);
	return buttons;
}


static EXOS_PORT _can_rx_port;
static EXOS_FIFO _can_free_msgs;
#define CAN_MSG_QUEUE 10
static XCPU_MSG _can_msg[CAN_MSG_QUEUE];


static void _get_can_messages()
{
	XCPU_MSG *xmsg = (XCPU_MSG *)exos_port_get_message(&_can_rx_port, 0);
	while (xmsg != NULL)
	{
		
		switch(xmsg->CanMsg.EP.Id)
		{
			case 0x300:
			{
				XCPU_MASTER_OUT1* tmsg = (XCPU_MASTER_OUT1*)&xmsg->CanMsg.Data.u8[0];
				_dash.Speed = tmsg->speed;
				_dash.battery_level_fx8 = tmsg->battery_level_fx8;
				_dash.CpuStatus	= tmsg->status;
                _dash.ActiveConfig.SpeedAdjust = tmsg->speed_adjust;
				_dash.Distance = tmsg->distance;
			}
			break;

			case 0x301:
			{
				XCPU_MASTER_OUT2* tmsg = (XCPU_MASTER_OUT2*)&xmsg->CanMsg.Data.u8[0];
				ANALOG_INPUT *ain_throttle = xanalog_input(THROTTLE_IDX);
				ain_throttle->Min = tmsg->throttle_adj_min << 4;
				ain_throttle->Max = tmsg->throttle_adj_max << 4;
				_dash.ActiveConfig.DriveMode = tmsg->drive_mode;
			}
			break;
		}
		exos_fifo_queue(&_can_free_msgs, (EXOS_NODE *)xmsg);
        xmsg = (XCPU_MSG *)exos_port_get_message(&_can_rx_port, 0);
	}
}

static void _state_machine(XCPU_BUTTONS buttons, DISPLAY_STATE *state)
{
	const EVREC_CHECK _adj_exit[] = {{CRUISE_MASK, CHECK_RELEASE}, {0x00000000, CHECK_END}};
	const EVREC_CHECK _menu_move[] = {{BRAKE_FRONT_MASK, CHECK_RELEASE}, {0x00000000, CHECK_END}};
	const EVREC_CHECK _menu_press[]= {{HORN_MASK, CHECK_RELEASE}, {0x00000000, CHECK_END}};
	static int menu_op = 0;

	ANALOG_INPUT *ain_throttle;
    _configuring = 1;
	switch(*state)
	{
		case ST_DASH:
			_configuring = 0;
			if ((_dash.Speed == 0) && (_dash.CpuStatus & XCPU_STATE_NEUTRAL))
			{
				if (event_happening(_maintenance_screen_access, 50)) // 1 second
				{
					_dash.Temp = _dash.ActiveConfig;
					menu_op = 0;
					*state = ST_FACTORY_MENU;
				}
				else if (event_happening(_mode_screen_access, 100)) // 2 second
				{
					_dash.Temp = _dash.ActiveConfig;
					*state = ST_ADJUST_DRIVE_MODE;
				}
			}
			break;
		case ST_DEBUG_INPUT:
			if ((buttons & XCPU_BUTTON_HORN) && (buttons & XCPU_BUTTON_CRUISE))
				*state = ST_DASH;
			break;

		case ST_ADJUST_WHEEL_DIA:
			_adj_down = (buttons & XCPU_BUTTON_BRAKE_REAR) ? 1 : 0;
			_adj_up = (buttons & XCPU_BUTTON_BRAKE_FRONT) ? 1 : 0;
			if (event_happening(_adj_exit, 1))
				*state = ST_DASH;
			break;

		case ST_ADJUST_THROTTLE_MAX:
			ain_throttle = xanalog_input(THROTTLE_IDX);
			_dash.Temp.ThrottleMax = ain_throttle->Current >> 8;
			if (event_happening(_adj_exit, 1))
				*state = ST_ADJUST_THROTTLE_MIN;
			break;
		case ST_ADJUST_THROTTLE_MIN:
			ain_throttle = xanalog_input(THROTTLE_IDX);
			_dash.Temp.ThrottleMin = ain_throttle->Current >> 8;
			if (event_happening(_adj_exit, 1))
			{
				_dash.ActiveConfig = _dash.Temp;
				_adj_throttle_cnt = RESEND_COUNTER;
				*state = ST_DASH;
			}
			break;

		case ST_FACTORY_MENU:
			if (event_happening(_menu_move, 1))
				_dash.CurrentMenuOption = ++_dash.CurrentMenuOption % 5;
			if (event_happening(_menu_press, 1))
			{
				switch(_dash.CurrentMenuOption)
				{
					case 0:  _switch_units_cnt = RESEND_COUNTER; break;
					case 1: *state = ST_ADJUST_WHEEL_DIA; break;
					case 2: *state = ST_ADJUST_THROTTLE_MAX; break;
					case 3: _dash.CpuStatus ^= XCPU_STATE_LIGHT_OFF; break;
					case 4: *state = ST_DEBUG_INPUT; break;
					default: assert(0);
				}
			}
			if (event_happening(_adj_exit, 1))
				*state = ST_DASH;
			break;

		case ST_ADJUST_DRIVE_MODE:
			if (event_happening(_menu_move, 1))
				_dash.Temp.DriveMode = ++_dash.Temp.DriveMode % 3;
			if (event_happening(_adj_exit, 8))
			{
				_dash.ActiveConfig = _dash.Temp;
				_adj_drive_mode_cnt = RESEND_COUNTER;
				*state = ST_DASH;
			}
			break;
	}
}

void main()
{
	usb_host_initialize();
	xdisplay_initialize();

	exos_port_create(&_can_rx_port, NULL);
	exos_port_create(&_can_rx_port, NULL);
	exos_fifo_create(&_can_free_msgs, NULL);
	for(int i = 0; i < CAN_MSG_QUEUE; i++) exos_fifo_queue(&_can_free_msgs, (EXOS_NODE *)&_can_msg[i]);

	hal_can_initialize(LCD_CAN_BUS, 250000);
	hal_fullcan_setup(_can_setup, NULL);

	xanalog_initialize();

	unsigned int st_time_base = 0;
    unsigned int time_base = exos_timer_time();
	unsigned int prev_time = 0;

	int screen_count = 0;
#ifdef DEBUG
    DISPLAY_STATE init_state = ST_LOGO_IN;
#else
    DISPLAY_STATE init_state = ST_LOGO_IN;
#endif
	DISPLAY_STATE state = init_state;

	int prev_cpu_state = 0;	// Default state is OFF, wait for master to start
	while(1)
	{
		//_dash.status |= XCPU_STATE_ON;
		// Read CAN messages from master 
		_get_can_messages();

		if (_dash.CpuStatus & XCPU_STATE_ON)
		{
			if ((prev_cpu_state & XCPU_STATE_ON) == 0)
			{
				// Switch ON, Restart main loop
				time_base = exos_timer_time();
				prev_time, st_time_base = 0;
               	lcdcon_gpo_backlight(1);
                state = init_state;
				screen_count = 0;

                xanalog_reset_filters();
			}

			unsigned int time = exos_timer_time();
			unsigned int req_update_time = MAIN_LOOP_TIME;
			time -= time_base;
			unsigned int elapsed_time = time - prev_time;

			XCPU_BUTTONS buttons = _read_send_inputs(state);

			xdisplay_clean_screen();

			int frame_skips = 0;
			if ((state > ST_INTRO_SECTION) && (state < ST_INTRO_SECTION_END))
			{
				frame_skips = 0;
				xdisplay_intro(&state, &st_time_base, time);
			}
			else
			{
				frame_skips = 4;
				_state_machine(buttons, &state);
				xdisplay_runtime_screens(state, &_dash);
			}

			// Screen conversion & dump
			if (screen_count > frame_skips)
			{
				xdisplay_dump();
                screen_count = 0;
			}
			screen_count++;

			if (elapsed_time < req_update_time)
				exos_thread_sleep( req_update_time - elapsed_time);
			prev_time = time;
		} // dash status ON
		else
		{
			// DASH OFF
			exos_thread_sleep(50);

			if (prev_cpu_state & XCPU_STATE_ON) 
			{
				xdisplay_clean_screen();
				xdisplay_dump();
				lcdcon_gpo_backlight(0);
			}
		}

        prev_cpu_state = _dash.CpuStatus;
	}	// while (1)
}

#ifdef DEBUG
static int _lost_msgs = 0;
#endif

void hal_can_received_handler(int index, CAN_MSG *msg)
{ 
	XCPU_MSG *xmsg;
	switch(msg->EP.Id)
	{
		case 0x300:
		case 0x301:
			xmsg = (XCPU_MSG *)exos_fifo_dequeue(&_can_free_msgs);
			if (xmsg != NULL)
			{
				xmsg->CanMsg = *msg;
				exos_port_send_message(&_can_rx_port, (EXOS_MESSAGE *)xmsg);
			}
#ifdef DEBUG
			else _lost_msgs++;
#endif
			break;
	}
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

void usb_host_add_drivers()
{
	usbd_hid_initialize();
	iap_initialize();
}