#include "xdisplay.h"
#include "xcpu.h"
#include "xanalog.h"
#include "xiap.h"
#include "event_recording.h"

#include <kernel/thread.h>
#include <kernel/event.h>
#include <kernel/timer.h>
#include <kernel/machine/hal.h>
#include <support/can_hal.h>
#include <support/apple/iap.h>
#include <support/lcd/lcd.h>
#include <usb/host.h>

#include <stdio.h>
#include <assert.h>

#define MAIN_LOOP_TIME 20
#define RESEND_COUNTER 25

static int _can_setup(int index, CAN_EP *ep, CAN_MSG_FLAGS *pflags, void *state);

static const CAN_EP _eps[] = {{0x300, LCD_CAN_BUS}, {0x301, LCD_CAN_BUS}, {0x302, LCD_CAN_BUS}, {0x303, LCD_CAN_BUS}, {0x304, LCD_CAN_BUS}};

static DASH_DATA _dash = 
{ 
	.DriveMode = XCPU_DRIVE_MODE_SOFT, 
	.ThrottleMin = 0, 
	.ThrottleMax = 0, 
	.CustomCurve = {0,0,0,0,0,0,0},
	{{0,""},{0,""},{0,""},{0,""},{0,""},{0,""}}
 };


static unsigned long _large_message_marks = 0;
static unsigned char _large_message_buffer[128];

char _phone_add_or_del = 0, _phone_line = 0;

static const EVREC_CHECK _maintenance_screen_access[] =
{
	{BRAKE_FRONT_MASK | BRAKE_REAR_MASK, CHECK_PRESSED},
	{BRAKE_FRONT_MASK | BRAKE_REAR_MASK, CHECK_RELEASED},
	{BRAKE_FRONT_MASK, CHECK_PRESSED},
	{BRAKE_FRONT_MASK, CHECK_RELEASED},
	{BRAKE_REAR_MASK, CHECK_RELEASED},
	{BRAKE_REAR_MASK, CHECK_PRESSED},
	{0x00000000, CHECK_END},
};

static const EVREC_CHECK _user_screen_access[] =
{
	{BRAKE_FRONT_MASK | CRUISE_MASK, CHECK_PRESSED},
	{BRAKE_FRONT_MASK | CRUISE_MASK, CHECK_RELEASED},
	{0x00000000, CHECK_END}
};

/* Old combo
static const EVREC_CHECK _mode_screen_access[] =
{
	{BRAKE_REAR_MASK | BRAKE_FRONT_MASK, CHECK_RELEASED},
	{BRAKE_REAR_MASK | BRAKE_FRONT_MASK, CHECK_PRESSED},
	{BRAKE_REAR_MASK | BRAKE_FRONT_MASK, CHECK_RELEASED},
	{BRAKE_REAR_MASK | BRAKE_FRONT_MASK, CHECK_PRESSED},
	{BRAKE_REAR_MASK | BRAKE_FRONT_MASK, CHECK_RELEASED},
	{0x00000000, CHECK_END},
};*/

unsigned short _adj_throttle_max=0, _adj_throttle_min=0;

static char _configuring = 0;

typedef struct
{
	char switch_lights;
	char switch_units;
	char adj_up, adj_down;
	char adj_max_speed_down, adj_max_speed_up;
	char next_drive_mode;
	//char confirm_new_phone, next_phone, choose_phone;
} EVENT_TRIGGERS;

static EVENT_TRIGGERS _trigs = {};

unsigned char _cmd_resend_cnt = 0;
XCPU_MASTER_INPUT2 _cmd_out = { .Cmd = 0 };

static void _set_command_to_xcpu(XCPU_COMMANDS command, unsigned char* data, int args)
{
	int i;
	_cmd_out.Cmd = command;
	for(i = 0; i < args; i++)
		_cmd_out.Data[i] = data[i];
	_cmd_resend_cnt = RESEND_COUNTER;
}

static inline int _trigger_event(unsigned char* counter, int flag)
{
	if(*counter == 0)
		return 0;
	*counter = *counter - 1;
	return flag;
}

static XCPU_BUTTONS _read_send_inputs(int state)
{
	xanalog_update();
	XCPU_BUTTONS buttons = xanalog_read_digital();

	int events = 0;
	if (_configuring)
		events |= XCPU_EVENT_CONFIGURING;

	events |= _trigger_event(&_trigs.adj_up, XCPU_EVENT_ADJUST_UP);
	events |= _trigger_event(&_trigs.adj_down, XCPU_EVENT_ADJUST_DOWN);
	events |= _trigger_event(&_trigs.adj_max_speed_up, XCPU_EVENT_ADJUST_MAX_SPEED_UP);
	events |= _trigger_event(&_trigs.adj_max_speed_down, XCPU_EVENT_ADJUST_MAX_SPEED_DOWN);
	events |= _trigger_event(&_trigs.switch_lights, XCPU_EVENT_SWITCH_LIGHTS);
	events |= _trigger_event(&_trigs.switch_units, XCPU_EVENT_SWITCH_UNITS);
	events |= _trigger_event(&_trigs.next_drive_mode, XCPU_EVENT_NEXT_DRIVE_MODE);
	/*events |= _trigger_event(&_trigs.next_phone, XCPU_EVENT_ADJUST_DOWN);
	events |= _trigger_event(&_trigs.confirm_new_phone, XCPU_EVENT_NEXT_PHONE);
	events |= _trigger_event(&_trigs.choose_phone, XCPU_EVENT_CHOOSE_PHONE);*/

	// Record inputs for sequence triggering (to start debug services)
	 int input_status = (( buttons & XCPU_BUTTON_BRAKE_REAR) ? BRAKE_REAR_MASK : 0) |
					(( buttons & XCPU_BUTTON_BRAKE_FRONT) ? BRAKE_FRONT_MASK : 0) |
					(( buttons & XCPU_BUTTON_CRUISE) ? CRUISE_MASK : 0) |
					(( buttons & XCPU_BUTTON_HORN) ? HORN_MASK : 0);
	event_record(input_status);

	ANALOG_INPUT *ain_throttle = xanalog_input(THROTTLE_IDX);
	unsigned int throttle = ain_throttle->Scaled;

	ANALOG_INPUT *ain_brake_rear = xanalog_input(BRAKE_REAR_IDX);
	ANALOG_INPUT *ain_brake_front = xanalog_input(BRAKE_FRONT_IDX);

	// Security
	if (_configuring)
	{
		throttle = 0;
		buttons &= ~XCPU_BUTTON_HORN;
		buttons &= ~XCPU_BUTTON_CRUISE;
	}


	XCPU_MASTER_INPUT buf = (XCPU_MASTER_INPUT) 
	{
		.ThrottleRaw = throttle,
		.BrakeRear = ain_brake_rear->Scaled >> 4, 
		.BrakeFront = ain_brake_front->Scaled>> 4,
		.Events =  events,
		.Buttons = buttons & 0xff
	};
	hal_can_send((CAN_EP) { .Id = 0x200, .Bus = LCD_CAN_BUS }, (CAN_BUFFER*)&buf, 8, CANF_PRI_ANY);

	return buttons;
}


static EXOS_PORT _can_rx_port;
static EXOS_FIFO _can_free_msgs;
#define CAN_MSG_QUEUE 10
static XCPU_MSG _can_msg[CAN_MSG_QUEUE];

static inline int _next_power_of_2(int n)
{
	n--;
	n |= n >> 1; 
	n |= n >> 2; 
	n |= n >> 4;
	n |= n >> 8;
	n |= n >> 16;
	return n+1;
}

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
				_dash.Speed = tmsg->Speed;
				_dash.BatteryLevel = tmsg->BattLevel;
				_dash.CpuStatus	= tmsg->State;
				_dash.Distance = tmsg->Distance;
   				_dash.DriveMode = tmsg->DriveMode;
			}
			break;

			case 0x301:
			{
				XCPU_MASTER_OUT2* tmsg = (XCPU_MASTER_OUT2*)&xmsg->CanMsg.Data.u8[0];
				ANALOG_INPUT *ain_throttle = xanalog_input(THROTTLE_IDX);
                _dash.SpeedAdjust = tmsg->WheelRatio;
				ain_throttle->Min = tmsg->ThrottleMin << 4;
				ain_throttle->Max = tmsg->ThrottleMax << 4;
				_dash.MaxSpeed = tmsg->MaxSpeed;
                //_dash.AppliedThrottle = tmsg->applied_throttle;
			}
			break;
	
			case 0x302:
			{
				for(int i=0; i<7; i++)
					_dash.CustomCurve[i] = xmsg->CanMsg.Data.u8[i];
			}
			break;

			case 0x303:
			case 0x304:
			{
				// Large message pass protocol
				XCPU_MASTER_OUT3* tmsg = (XCPU_MASTER_OUT3*)&xmsg->CanMsg.Data.u8[0];
				assert((tmsg->idx & 0x7f) <= 121);	// 128 bytes buffer
				for(int i=0; i<7; i++)
					_large_message_buffer[(tmsg->idx & 0x7f) * 7] = tmsg->data[i];
				if (tmsg->idx == 0)
					_large_message_marks = 1;	// Init of message recording
				else
					_large_message_marks |= 1 << (tmsg->idx & 0x7f);
				if (tmsg->idx & 0x80)	// End of large message mark
				{
					int msg_len = sizeof(_dash.PhoneList);
					unsigned char* dst = (unsigned char*)&_dash.PhoneList[0]; 
					int marks_needed = _next_power_of_2(msg_len) - 1;
					if ((marks_needed & _large_message_marks) == marks_needed)
						__mem_copy(dst, dst + msg_len, _large_message_buffer);
                    _large_message_marks = 0;
				}
			}
			break;

		}
		exos_fifo_queue(&_can_free_msgs, (EXOS_NODE *)xmsg);
        xmsg = (XCPU_MSG *)exos_port_get_message(&_can_rx_port, 0);
	}
}

static void	_get_iphone_messages()
{
	XIAP_FRAME_FROM_IOS frame;
	int done = xiap_get_frame(&frame);
	if (done) switch(frame.Command)
	{
		case IOS_COMMAND_POWER_ON:
			_set_command_to_xcpu(XCPU_CMD_POWER_ON, 0, 0);
			break;
		case IOS_COMMAND_POWER_OFF:
			_set_command_to_xcpu(XCPU_CMD_POWER_OFF, 0, 0);
			break;
		case IOS_COMMAND_ADJUST_DRIVE_MODE:
			_set_command_to_xcpu(XCPU_CMD_SET_DRIVE_MODE, _cmd_out.Data, 2);
			break;
		case IOS_COMMAND_SET_CUSTOM_CURVE:
			_set_command_to_xcpu(XCPU_CMD_SET_CURVE, _cmd_out.Data, 7);
			break;
	}
}


static void _state_machine(XCPU_BUTTONS buttons, DISPLAY_STATE *state)
{
	const EVREC_CHECK _menu_exit[] = {{CRUISE_MASK, CHECK_RELEASE}, {0x00000000, CHECK_END}};
	const EVREC_CHECK _menu_move[] = {{BRAKE_FRONT_MASK, CHECK_RELEASE}, {0x00000000, CHECK_END}};
	const EVREC_CHECK _menu_press[]= {{HORN_MASK, CHECK_RELEASE}, {0x00000000, CHECK_END}};

	ANALOG_INPUT *ain_throttle;
    _configuring = 1;
	switch(*state)
	{
		case ST_DASH:
			_configuring = 0;
			if ((_dash.Speed == 0) && (_dash.CpuStatus & XCPU_STATE_NEUTRAL))
			{
				/*if (event_happening(_maintenance_screen_access, 100)) // 2 second
					*state = ST_FACTORY_MENU;
				else if (event_happening(_mode_screen_access, 100)) // 2 second
					*state = ST_USER_MENU; 
				*/
			}
			break;
		case ST_DEBUG_INPUT:
			if (event_happening(_menu_exit, 1))
				*state = ST_DASH;
			break;

		case ST_ADJUST_WHEEL_DIA:
			if (buttons & XCPU_BUTTON_BRAKE_REAR)
				_trigs.adj_up = RESEND_COUNTER;
			if (buttons & XCPU_BUTTON_BRAKE_FRONT)
				_trigs.adj_down = RESEND_COUNTER;
			if ( event_happening(_menu_exit, 1) ||  event_happening(_menu_press, 1))
				*state = ST_DASH;
			break;

		case ST_ADJUST_THROTTLE_MAX:
			ain_throttle = xanalog_input(THROTTLE_IDX);
			_adj_throttle_max = ain_throttle->Current;
			if (event_happening(_menu_press, 1))
				*state = ST_ADJUST_THROTTLE_MIN;
			break;
		case ST_ADJUST_THROTTLE_MIN:
			ain_throttle = xanalog_input(THROTTLE_IDX);
			_adj_throttle_min = ain_throttle->Current;
			if (event_happening(_menu_press, 1))
			{
				unsigned char args[2] = {_adj_throttle_min >> 4, _adj_throttle_max >> 4};
				_set_command_to_xcpu(XCPU_CMD_ADJUST_THROTTLE, args, 2);
				*state = ST_DASH;
			}
			break;
		// Disabled menu
		case ST_FACTORY_MENU:
			if (event_happening(_menu_move, 1))
				_dash.CurrentMenuOption = ++_dash.CurrentMenuOption % 6;
			if (event_happening(_menu_press, 1))
			{
				switch(_dash.CurrentMenuOption)
				{
					case 0:	_trigs.switch_units = RESEND_COUNTER; break;
					case 1:	*state = ST_ADJUST_WHEEL_DIA; break;
					case 2: *state = ST_ADJUST_THROTTLE_MAX; break;
					case 3: _trigs.switch_lights = RESEND_COUNTER; break;
					case 4: *state = ST_DEBUG_INPUT; break;
					case 5: *state = ST_ADJUST_MAX_SPEED; break;
					default: assert(0);
				}
			}
			if (event_happening(_menu_exit, 1))
				*state = ST_DASH;
			break;

		case ST_USER_MENU:
        	if (event_happening(_menu_move, 1))
				_dash.CurrentMenuOption = ++_dash.CurrentMenuOption % 3;
			if (event_happening(_menu_press, 1))
			{
				switch(_dash.CurrentMenuOption)
				{
					case 0: _trigs.next_drive_mode = RESEND_COUNTER; break;
					case 1: _trigs.switch_units = RESEND_COUNTER; break;
					case 2: _trigs.switch_lights = RESEND_COUNTER; break;
					//case 3: break; // Move to new phone menu
					default: break;
				}
			}
			if (event_happening(_menu_exit, 8)) // || event_happening(_menu_press, 8) 
				*state = ST_DASH;
			break;

		case ST_ADJUST_MAX_SPEED:
			if(buttons & XCPU_BUTTON_BRAKE_REAR)
				_trigs.adj_max_speed_up = RESEND_COUNTER;
			if(buttons & XCPU_BUTTON_BRAKE_FRONT)
				_trigs.adj_max_speed_down = RESEND_COUNTER;
			if (event_happening(_menu_press, 1) || event_happening(_menu_exit, 1))
				*state = ST_DASH;
			break;

		case ST_ADD_PHONE:
			/*if (event_happening(_menu_press, 1) && _dash.PhoneList[5].active)
				_trigs.confirm_new_phone = RESEND_COUNTER,
				*state = ST_DASH;
			if (event_happening(_menu_exit, 1))
				*state = ST_DASH;*/		
			break;

		case ST_SHOW_PHONES:
			/*if (event_happening(_menu_move, 1))
				_trigs.next_phone = RESEND_COUNTER;
			if (event_happening(_menu_press, 1))
				_trigs.choose_phone = RESEND_COUNTER;*/
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

	hal_can_initialize(LCD_CAN_BUS, 250000, 0); //CAN_INITF_DISABLE_RETRANSMISSION);
	hal_fullcan_setup(_can_setup, NULL);

	xanalog_initialize();

	unsigned int st_time_base = 0;
    unsigned int time_base = exos_timer_time();
	unsigned int prev_time = 0;
    unsigned int intro_next_state = ST_DASH;

	int screen_count = 0;

	DISPLAY_STATE init_state = ST_LOGO_IN;
	DISPLAY_STATE state = init_state;

//_dash.CpuStatus |= XCPU_STATE_ON;

	int prev_cpu_state = 0;	// Default state is OFF, wait for master to start
	while(1)
	{
		//_dash.status |= XCPU_STATE_ON;
		// Read CAN messages from master 
		_get_can_messages();

		// get iPhone messages
		_get_iphone_messages();

		// send pending commands
		if (_cmd_resend_cnt > 0)
		{
			hal_can_send((CAN_EP) { .Id = 0x201, .Bus = LCD_CAN_BUS }, (CAN_BUFFER *)&_cmd_out, 8, CANF_PRI_ANY);
			_cmd_resend_cnt--;
		}

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
				if (event_happening(_user_screen_access, 50))
					intro_next_state = ST_USER_MENU;
				xdisplay_intro(&state, &st_time_base, time, intro_next_state);
			}
			else
			{
                intro_next_state = ST_DASH;
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

		xiap_send_frame(&_dash);
        prev_cpu_state = _dash.CpuStatus;
	}
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