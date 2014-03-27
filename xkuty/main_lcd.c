#include "xdisplay.h"
#include "xcpu.h"
#include "xanalog.h"
#include "xiap.h"
#include "event_recording.h"
#include "multipacket_msg.h"

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

// Check both "hal_can_received_handler" & "_get_can_messages" when changing this list
static const CAN_EP _eps[] = {{0x300, LCD_CAN_BUS}, {0x301, LCD_CAN_BUS}, {0x302, LCD_CAN_BUS}, {0x303, LCD_CAN_BUS}};

static DASH_DATA _dash = 
{ 
	.DriveMode = XCPU_DRIVE_MODE_SOFT, 
	.ThrottleMin = 0, 
	.ThrottleMax = 0, 
	.CustomCurve = {0},
	.Phones = {{0,""},{0,""},{0,""},{0,""},{0,""},{0,""}},
	.AdjThrottleMin = 0,
	.AdjThrottleMax = 0,
	.PhoneAddOrDel = 0, 
	.PhoneLine = 0
};

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
	char SwitchLights;
	char SwitchUnits;
	char AdjUp, AdjDown;
	char AdjMaxSpeedDown, AdjMaxSpeedUp;
	char NextDriveMode;
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

	events |= _trigger_event(&_trigs.AdjUp, XCPU_EVENT_ADJUST_UP);
	events |= _trigger_event(&_trigs.AdjDown, XCPU_EVENT_ADJUST_DOWN);
	events |= _trigger_event(&_trigs.AdjMaxSpeedUp, XCPU_EVENT_ADJUST_MAX_SPEED_UP);
	events |= _trigger_event(&_trigs.AdjMaxSpeedDown, XCPU_EVENT_ADJUST_MAX_SPEED_DOWN);
	events |= _trigger_event(&_trigs.SwitchLights, XCPU_EVENT_SWITCH_LIGHTS);
	events |= _trigger_event(&_trigs.SwitchUnits, XCPU_EVENT_SWITCH_UNITS);
	events |= _trigger_event(&_trigs.NextDriveMode, XCPU_EVENT_NEXT_DRIVE_MODE);

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

static void _get_can_messages()
{
	XCPU_MSG *xmsg = (XCPU_MSG *)exos_port_get_message(&_can_rx_port);
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
			case 0x303:
			{
				// Large message pass protocol, using 0x302 & 0x303
				int msg_len = 0;
				const unsigned char* multi_msg = multipacket_msg_receive (&msg_len, &xmsg->CanMsg);
				if (multi_msg)
				{
					switch(multi_msg[0])
					{
						case XCPU_MULTI_CUSTOM_CURVE:
							for(int i=0; i<7; i++)
								_dash.CustomCurve[i] = multi_msg[1 + i];
							break;
						case XCPU_MULTI_PHONE_LOG:
						{
							int dst_len = sizeof(_dash.Phones);
							unsigned char* dst = (unsigned char*)&_dash.Phones[0]; 
							__mem_copy(dst, dst + dst_len, &multi_msg[1]);
						}
						break;
					}
				}
			}
			break;

		}
		exos_fifo_queue(&_can_free_msgs, (EXOS_NODE *)xmsg);
        xmsg = (XCPU_MSG *)exos_port_get_message(&_can_rx_port);
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

static int _next_phone_entry (int start) 
{
	int i, found = -1;
	for(i=start; i<XCPU_VIEW_PHONES; i++)	// Only XCPU_VIEW_PHONES options on screen
	{
		if (_dash.Phones[i].flags & 1)
		{
			found = i;
			break;
		}
	}
	return found;
}

static int _count_phone_entries () 
{
	int i, c = 0;
	for(i=0; i<XCPU_VIEW_PHONES; i++)	// Only XCPU_VIEW_PHONES options on screen
		if ((_dash.Phones[i].flags & 1) == 0)
			c++;
	return c;
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
				_trigs.AdjUp = RESEND_COUNTER;
			if (buttons & XCPU_BUTTON_BRAKE_FRONT)
				_trigs.AdjDown = RESEND_COUNTER;
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
					case 0:	_trigs.SwitchUnits = RESEND_COUNTER; break;
					case 1:	*state = ST_ADJUST_WHEEL_DIA; break;
					case 2: *state = ST_ADJUST_THROTTLE_MAX; break;
					case 3: _trigs.SwitchLights = RESEND_COUNTER; break;
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
					case 0: _trigs.NextDriveMode = RESEND_COUNTER; break;
					case 1: _trigs.SwitchUnits = RESEND_COUNTER; break;
					case 2: _trigs.SwitchLights = RESEND_COUNTER; break;
					case 3: *state = ST_SHOW_PHONES;
							_dash.PhoneLine = _next_phone_entry ( 0);
                            _dash.PhoneLine = (_dash.PhoneLine >= 0) ? _dash.PhoneLine : 0;
                            _dash.PhoneAddOrDel = 0;
							break; 
					default: break;
				}
			}
			if (event_happening(_menu_exit, 8)) // || event_happening(_menu_press, 8) 
				*state = ST_DASH;
			break;

		case ST_ADJUST_MAX_SPEED:
			if(buttons & XCPU_BUTTON_BRAKE_REAR)
				_trigs.AdjMaxSpeedUp = RESEND_COUNTER;
			if(buttons & XCPU_BUTTON_BRAKE_FRONT)
				_trigs.AdjMaxSpeedDown = RESEND_COUNTER;
			if (event_happening(_menu_press, 1) || event_happening(_menu_exit, 1))
				*state = ST_DASH;
			break;

		case ST_ADD_PHONE:
			if (event_happening(_menu_press, 1) && (_dash.Phones[XCPU_NEW_PHONE].flags & 1))
				_set_command_to_xcpu(XCPU_CMD_CONFIRM_PHONE, 0, 0),
				*state = ST_DASH;
			if (event_happening(_menu_exit, 1))
				*state = ST_DASH;
			break;

		case ST_SHOW_PHONES:
			if (event_happening(_menu_move, 1))
			{
				// Move menu marker to the next valid phone entry, or "add" option.
				if ( _dash.PhoneLine == XCPU_NEW_PHONE)	// We are in "add phone"
				{
					_dash.PhoneLine = _next_phone_entry (0);
					if( _dash.PhoneLine < 0)
						_dash.PhoneLine = 0;	// Log is empty, keep
				}
				else
				{
					int next = XCPU_NEW_PHONE;
					if (_dash.PhoneLine < (XCPU_NEW_PHONE - 1))
						next = _next_phone_entry (_dash.PhoneLine + 1);	
					_dash.PhoneLine = ( next < 0) ? XCPU_NEW_PHONE : next; // Move to "add phone" if suitable
				}
                _dash.PhoneAddOrDel = 0;
			}
			// Press offers to delete an entry, or enter into the "add phone" screen
			if (event_happening(_menu_press, 1))
			{
				if(_dash.PhoneLine == XCPU_NEW_PHONE)	// Add phone option
				{
					if (_count_phone_entries() > 0)
						*state = ST_ADD_PHONE;
				}
				else
				{
					if( _dash.PhoneAddOrDel == 0)
						_dash.PhoneAddOrDel = 1;
					else
					{
						// Delete is confirmed
						unsigned char arg = _dash.PhoneLine;
						_set_command_to_xcpu(XCPU_CMD_REMOVE_PHONE, &arg, 2);
						_dash.PhoneAddOrDel = 0;
					}
				}
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
	for(int i = 0; i < CAN_MSG_QUEUE; i++) 
		exos_fifo_queue(&_can_free_msgs, (EXOS_NODE *)&_can_msg[i]);

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
	//init_state = ST_SHOW_PHONES;

	int prev_cpu_state = 0;	// Default state is OFF, wait for master to start
	while(1)
	{
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

		//_dash.CpuStatus |= XCPU_STATE_ON;
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
	if((msg->EP.Id >= 0x300) && (msg->EP.Id <= 0x303))
	{
		xmsg = (XCPU_MSG *)exos_fifo_dequeue(&_can_free_msgs);
		if (xmsg != NULL)
		{
			xmsg->CanMsg = *msg;
			exos_port_send_message(&_can_rx_port, (EXOS_MESSAGE *)xmsg);
		}
	}
#ifdef DEBUG
	else _lost_msgs++;
#endif
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