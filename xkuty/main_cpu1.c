#include <kernel/thread.h>
#include <kernel/port.h>
#include <kernel/timer.h>
#include <support/can_hal.h>
#include <support/pwm_hal.h>
#include <support/adc_hal.h>
#include "xcpu.h"
#include "xcpu/board.h"
#include "xcpu/speed.h"
#include "xcpu/persist.h"
#include "xcpu/throttle_curves.h"
#include "pid.h"

#define MAX_SPEED (40)
#define BRAKE_THRESHOLD 30
#define PWM_RANGE 100
#define MOTOR_OFFSET 50
#define MOTOR_RANGE (170 - MOTOR_OFFSET)
//#define MOTOR_RANGE (160 - MOTOR_OFFSET)
#define WHEEL_RATIO_KMH (12.9 / 47)
#define WHEEL_RATIO_MPH (WHEEL_RATIO_KMH / 1.60934)
#define BATT_VOLTAGE_RATIO  16.9f

static const CAN_EP _eps[] = { {0x200, 0}, {0x201, 0} };
static int _can_setup(int index, CAN_EP *ep, CAN_MSG_FLAGS *pflags, void *state);
static EXOS_PORT _can_rx_port;
static EXOS_FIFO _can_free_msgs;
#define CAN_MSG_QUEUE 10
static XCPU_MSG _can_msg[CAN_MSG_QUEUE];
#ifdef DEBUG
static int _lost_msgs = 0;
#endif

static XCPU_OUTPUT_MASK _output_state = OUTPUT_NONE;
static XCPU_OUTPUT_MASK _default_output_state = OUTPUT_HEADL | OUTPUT_TAILL;

static PID_K _pid_k; 
static PID_STATE _pid;

static EXOS_TIMER _timer;
static void _run_diag();
static int _push_delay(int push, unsigned char *state, int limit);

static XCPU_PERSIST_DATA _storage;

static enum
{
        CONTROL_OFF = 0,
        CONTROL_ON,
        CONTROL_CRUISE,
} _control_state;
static XCPU_STATE _state; // comm state to share (with lcd)
static XCPU_DRIVE_MODE _drive_mode = XCPU_DRIVE_MODE_SOFT;

typedef struct 
{
	unsigned char  buttons;
	unsigned char  events;
	unsigned short  throttle_raw;
	unsigned char  brake_rear, brake_front;
	unsigned char  throttle_adj_min, throttle_adj_max;
} XLCD_INPUT;

static XLCD_INPUT _lcd = {0,0,0,0,0,0,0};

static XCPU_EVENTS _do_lcd_command(XCPU_MASTER_INPUT2 *input)
{
	int i;
	switch(input->Cmd)
	{
		case XCPU_CMD_POWER_ON:		return XCPU_EVENT_TURN_ON;
		case XCPU_CMD_POWER_OFF:	return XCPU_EVENT_TURN_OFF;
		case XCPU_CMD_SET_DRIVE_MODE:
			_drive_mode = input->Data[0];
			break;
		case XCPU_CMD_SET_CURVE:
			for(i = 0; i < 7; i++)
				_storage.CustomCurve[i] = input->Data[i];
			break;
	}
	return 0;
}

static XCPU_EVENTS _read_can_messages()
{
	XCPU_EVENTS cmd_events = 0;
	XCPU_MSG *xmsg;
	while(xmsg = (XCPU_MSG *)exos_port_get_message(&_can_rx_port, 0), xmsg != NULL)
	{
		CAN_BUFFER *data = &xmsg->CanMsg.Data;
		switch (xmsg->CanMsg.EP.Id)
		{
			case 0x200:
				_lcd.buttons = data->u8[0];
				_lcd.throttle_raw = data->u8[1] | ( data->u8[2] << 8);
				_lcd.brake_rear = data->u8[3];
				_lcd.brake_front = data->u8[4];
				_lcd.throttle_adj_min = data->u8[5];
				_lcd.throttle_adj_max = data->u8[6];
				_lcd.events = data->u8[7];
				break;

			case 0x201:
				cmd_events |= _do_lcd_command((XCPU_MASTER_INPUT2 *)data);
				break;
		}
		exos_fifo_queue(&_can_free_msgs, (EXOS_NODE *)xmsg);
	}
	return cmd_events;
}

static int _batt_level ()
{
	int batt = hal_adc_read(1) >> 6;	// Battery voltage = Input adc / BATT_VOLTAGE_RATIO
	const int bot = (int)(3.5f * 13.0f * BATT_VOLTAGE_RATIO);	// Discharged level 3.5v
	const int top = (int)(4.0f * 13.0f * BATT_VOLTAGE_RATIO); // Charged value 4.0v
	batt = ((batt - bot) * (0x10000 / (top - bot))) >> 8;
	if (batt < 0) batt = 0;
	if (batt > 0xff) batt = 0xff;
	return batt;
}

static void _send_can_messages(unsigned int speed, unsigned int distance)
{
	int i;
	CAN_BUFFER buf;

	for(i = 0; i< 7; i++)
		buf.u8[i] = _storage.CustomCurve[i];

	hal_can_send((CAN_EP) { .Id = 0x302 }, &buf, 8, CANF_NONE);

	exos_thread_sleep (4);  // Temporary fix; packets get overwritten

	buf.u32[0] = buf.u32[1] = 0;
	buf.u8[0]  = _storage.ThrottleAdjMin;
	buf.u8[1]  = _storage.ThrottleAdjMax;
	buf.u16[1] = _drive_mode;
	hal_can_send((CAN_EP) { .Id = 0x301 }, &buf, 8, CANF_NONE);

	exos_thread_sleep (4);  // IDEM

	buf.u8[0] = speed;
	buf.u8[1] = _batt_level();
	buf.u8[2] = _state;
	buf.u8[3] = _storage.WheelRatioAdj;
	buf.u32[1] = distance;
	hal_can_send((CAN_EP) { .Id = 0x300 }, &buf, 8, CANF_NONE);

	exos_thread_sleep (4);  // IDEM
}


typedef struct { float speed, dt, ratio, s_partial; } SPEED_DATA;

static void _speed_calculation(SPEED_DATA *sp, int mag_miles, float wheel_ratio_adjust)
{
	static int 	speed_wait = 0;
	float dt2 = 0;
	int space = speed_read(&dt2);
	sp->dt += dt2;
	if (space != 0)
	{
		// Magnitude miles, false (km) true (miles)
		sp->ratio = mag_miles ? WHEEL_RATIO_MPH : WHEEL_RATIO_KMH;
		sp->ratio += sp->ratio * wheel_ratio_adjust * 0.01F;
		sp->speed = sp->dt != 0 ? ((space / sp->dt) *  sp->ratio) : 99.0f;
		sp->s_partial += space;
		dt2 = sp->dt;
		sp->dt = 0;
		speed_wait = 0;
	}
	else
	{
		if (speed_wait < 10) 
			speed_wait++;
		else 
			sp->speed = 0.0f;
	}
}

typedef struct 
{
	unsigned char start;
	unsigned char cruise;
	unsigned char off;
	unsigned char up,down;
	unsigned char swtch;
	unsigned char mode;
	unsigned char adj;
	unsigned char lights;
//	unsigned char ios_power_on, ios_power_off;
//	unsigned char ios_mode;
} PUSH_CNT;


void main()
{
	int result;
	hal_pwm_initialize(PWM_TIMER_MODULE, PWM_RANGE - 1, 200000);
	hal_pwm_set_output(PWM_TIMER_MODULE, 0, 1025);

	hal_adc_initialize(1000, 16);

	PUSH_CNT push = { };
	SPEED_DATA sp = { .speed = 0 };
	speed_initialize();
	_pid_k = (PID_K) { .P = 5, .I = 5, .CMin = 0, .CMax = 255 };

	exos_timer_create(&_timer, 50, 50, exos_signal_alloc());

	exos_port_create(&_can_rx_port, NULL);
	exos_fifo_create(&_can_free_msgs, NULL);
	for(int i = 0; i < CAN_MSG_QUEUE; i++) exos_fifo_queue(&_can_free_msgs, (EXOS_NODE *)&_can_msg[i]);

	hal_can_initialize(0, 250000);
	hal_fullcan_setup(_can_setup, NULL);

#ifdef DEBUG
	xcpu_board_output(_output_state);
	exos_thread_sleep(500);
	xcpu_board_output(_output_state | OUTPUT_HORN);
	exos_thread_sleep(100);
	xcpu_board_output(_output_state);
	exos_thread_sleep(200);
	xcpu_board_output(_output_state | OUTPUT_HORN);
	exos_thread_sleep(100);
	xcpu_board_output(_output_state);
#endif

	unsigned char led = 0;
        
	if (!persist_load(&_storage))
	{
		_storage = (XCPU_PERSIST_DATA) { .Magic = XCPU_PERSIST_MAGIC,
				.TotalSteps = 0,
				.ConfigBits = XCPU_CONFIGF_NONE,
				.DriveMode = XCPU_DRIVE_MODE_SOFT,
				.WheelRatioAdj = 0,
				.ThrottleAdjMin = 66, 
				.ThrottleAdjMax = 166,
				.CustomCurve = {255,255,255,255,255,255,255} };
	}

	set_custom_curve_ptr(&_storage.CustomCurve);

	_control_state = CONTROL_OFF;
	_output_state = OUTPUT_NONE;
	_state = XCPU_STATE_OFF;

	int throttle_target = 0;
	while(1)
	{
		exos_timer_wait(&_timer);

		XCPU_EVENTS events = _read_can_messages();
		events |= _lcd.events;

		int curve_in = (int)(4096.f * (sp.speed / (float) MAX_SPEED));
		int throttle_factor = get_curve_value(curve_in, _drive_mode) >> 4;
		unsigned char throttle = (_lcd.throttle_raw * throttle_factor) >> 12;

#ifndef __XCPU_VIRTUAL__
		// read speed sensors
		_speed_calculation(&sp, (_state & XCPU_STATE_MILES) ? 1:0, (float)_storage.WheelRatioAdj);
#endif

		switch(_control_state)
		{
			case CONTROL_OFF:
				_output_state = OUTPUT_NONE;
				if ((events & XCPU_EVENT_TURN_ON) ||
					_push_delay(xcpu_board_input(INPUT_BUTTON_START), &push.start, 10))
				{
					_control_state = CONTROL_ON;
					_state = (_storage.ConfigBits & XCPU_CONFIGF_MILES) ? XCPU_STATE_ON | XCPU_STATE_MILES : XCPU_STATE_ON;
					_state |= XCPU_STATE_NEUTRAL;
					_drive_mode = _storage.DriveMode;
					_run_diag();
				}
				break;
			case CONTROL_CRUISE:
				{
					int input_throttle = throttle;
					throttle = pid(&_pid, sp.speed, &_pid_k, 0.05F);
					if (throttle > 250)
					{
						throttle--;
					}

					if (input_throttle > (throttle + 20) ||
						(_output_state & OUTPUT_BRAKEL))
					{
						_control_state = CONTROL_ON;
						_state &= ~XCPU_STATE_CRUISE_ON;
					}
				}
				// NOTE: case fall down
			case CONTROL_ON:
				_output_state = _default_output_state;
				if (_lcd.brake_rear > BRAKE_THRESHOLD || _lcd.brake_front > BRAKE_THRESHOLD)
				{
					_output_state |= OUTPUT_BRAKEL;
                    //  Rear brake disables throttling
					int dis_throttle = _lcd.brake_front > BRAKE_THRESHOLD;
					if (_drive_mode != XCPU_DRIVE_MODE_RACING)
						dis_throttle |= _lcd.brake_rear > BRAKE_THRESHOLD;
					if (dis_throttle)
					{
						throttle = 0;
						_output_state |= OUTPUT_EBRAKE;
					}
				}

				if (_push_delay(_lcd.events & XCPU_EVENT_SWITCH_LIGHTS, &push.lights, 5))
					if ( _default_output_state == OUTPUT_NONE)
						_default_output_state = OUTPUT_HEADL | OUTPUT_TAILL;
					else
						_default_output_state = OUTPUT_NONE;
		
				if ((_lcd.buttons & XCPU_BUTTON_HORN) && !(_lcd.events & XCPU_EVENT_CONFIGURING))
					_output_state |= OUTPUT_HORN;

				if (_push_delay(_lcd.events & XCPU_EVENT_ADJUST_UP, &push.up, 5) &&
					_storage.WheelRatioAdj < 10)
				{
					_storage.WheelRatioAdj++;
				}
				if (_push_delay(_lcd.events & XCPU_EVENT_ADJUST_DOWN, &push.down, 5) &&
					_storage.WheelRatioAdj > -10)
				{
					_storage.WheelRatioAdj--;
				}
				if (_push_delay(_lcd.events & XCPU_EVENT_SWITCH_UNITS, &push.swtch, 5))
				{
					_state ^= XCPU_STATE_MILES;
				}

				if (_push_delay(_lcd.events & XCPU_EVENT_ADJUST_THROTTLE, &push.adj, 5))
				{
					_storage.ThrottleAdjMin = _lcd.throttle_adj_min;
					_storage.ThrottleAdjMax = _lcd.throttle_adj_max;
				}

				if (_push_delay(_lcd.buttons & XCPU_BUTTON_CRUISE, &push.cruise, 5))
				{
					if (sp.speed < 0.01f)	// 0.0f value is forced
					{
						_state ^= XCPU_STATE_NEUTRAL;
					}
					else
					{
						if (sp.speed > 10.0f)
						{
							_pid.SetPoint = sp.speed;
							_pid.Integral = throttle / _pid_k.I;

							_state ^= XCPU_STATE_CRUISE_ON;
							_control_state = _state & XCPU_STATE_CRUISE_ON ? CONTROL_CRUISE : CONTROL_ON;
						}
					}
				}

				if ((events & XCPU_EVENT_TURN_OFF) ||
					_push_delay(xcpu_board_input(INPUT_BUTTON_START), &push.start, 10) ||
					_push_delay(_lcd.buttons & XCPU_BUTTON_CRUISE, &push.off, 50))
				{
					if (sp.speed == 0.0f)  // 0.0f value is forced
					{
						hal_pwm_set_output(PWM_TIMER_MODULE, 0, PWM_RANGE + 1); //      disable pwm

						_storage.TotalSteps += sp.s_partial;
						sp.s_partial = 0;
						_storage.ConfigBits = XCPU_CONFIGF_NONE;
						if (_state & XCPU_STATE_MILES) 
							_storage.ConfigBits |= XCPU_CONFIGF_MILES;

						_storage.DriveMode = _drive_mode;

						persist_save(&_storage);

						_output_state = OUTPUT_NONE;
						_control_state = CONTROL_OFF;
						_state = XCPU_STATE_OFF;
					}
					else 
						_state |= XCPU_STATE_WARNING;
				}

				// TODO: check bms and engine controller

				// update throttle out
				if (_control_state != CONTROL_OFF)
				{
					unsigned char th_lim = (_state & XCPU_STATE_NEUTRAL) ? 0 
							: MOTOR_OFFSET + ((throttle * MOTOR_RANGE) >> 8);
					int pwm_val = PWM_RANGE - ((th_lim * PWM_RANGE) >> 8);
					hal_pwm_set_output(PWM_TIMER_MODULE, 0, pwm_val);

#ifdef __XCPU_VIRTUAL__
#define MAX_ACC 20.0F
					float max_speed = _state & XCPU_STATE_MILES ? 30 : 40;
					float th_ratio = (_state & XCPU_STATE_NEUTRAL) ? 0 : (throttle / 256.0F); 
					float drag_ratio = sp.speed / max_speed;
					if (_output_state & OUTPUT_BRAKEL) drag_ratio = 1 - ((1 - drag_ratio) * 0.1f); 
					float acc = MAX_ACC * (th_ratio - (drag_ratio * drag_ratio * drag_ratio));
					sp.speed += acc * 0.050f;	// dt
#endif
				}
				break;                          
			}

			xcpu_board_led(led = !led);     // toggle led
			float dist = (((_storage.TotalSteps + sp.s_partial) * (sp.ratio / 3.6f)) / 100);
			_send_can_messages((unsigned int)sp.speed, (unsigned long)dist);

			xcpu_board_output(_output_state);
        }
}

void hal_can_received_handler(int index, CAN_MSG *msg)
{
        XCPU_MSG *xmsg;
        switch(msg->EP.Id)
        {
                case 0x200:                
				case 0x201:
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
                *ep = _eps[index];
                *pflags = CANF_RXINT;
                return 1;
        }
        return 0;
}

static int _push_delay(int push, unsigned char *state, int limit)
{
        int count = *state;
        count = push ? count + 1 : 0;
        if (count < limit)
        {
                *state = count;
                return 0;
        }
        if (count == limit)
        {
                *state = count;
                return 1;
        }
        return 0;
}

static void _run_diag()
{
}