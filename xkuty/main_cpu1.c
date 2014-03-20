#include <kernel/thread.h>
#include <kernel/port.h>
#include <kernel/timer.h>
#include <support/pwm_hal.h>
#include <support/lpc11/uart.h>
#include "xcpu.h"
#include "xcpu/board.h"
#include "xcpu/speed.h"
#include "xcpu/persist.h"
#include "xcpu/bt.h"
#include "xcpu/can_comms.h"
#include "xcpu/sensor_cpu1.h"
#include "xcpu/throttle_curves.h"
#include "pid.h"

#define MAX_SPEED (40)
#define BRAKE_THRESHOLD 50
#define PWM_RANGE 100
#define MOTOR_OFFSET 50
#define MOTOR_RANGE (170 - MOTOR_OFFSET)

#define MAIN_LOOP_TIME 50		// Ms
#define MAIN_LOOP_FREQ (1000 / MAIN_LOOP_TIME)

static const XCPU_OUTPUT_MASK _default_output_state = OUTPUT_HEADL | OUTPUT_TAILL | OUTPUT_MOTOREN | OUTPUT_POWEREN;

static PID_K _pid_k; 
static PID_STATE _pid;
static PID_K _speed_limit_pid_k;
static PID_STATE _speed_limit_pid;

static EXOS_TIMER _timer;
static void _run_diag();
static int _push_delay(int push, unsigned short *state, int limit);

static XCPU_PERSIST_DATA _storage;

static enum
{
        CONTROL_OFF = 0,
        CONTROL_ON,
        CONTROL_CRUISE,
        CONTROL_LIGHTS_OFF_STAND_BY
} _control_state;
static XCPU_STATE _state; // comm state to share (with lcd)
static XCPU_DRIVE_MODE _drive_mode = XCPU_DRIVE_MODE_SOFT;

static XCPU_MASTER_INPUT _lcd;

static XCPU_EVENTS _can_read_messages(char *got_lcd_input);
static void _can_send_messages(unsigned speed, unsigned long dist, unsigned throttle);
static XCPU_EVENTS _bt_read_events();
static void _bt_send_messages(unsigned speed, unsigned long dist, unsigned throttle);
static void _shutdown(XCPU_SPEED_DATA *sp);

typedef struct 
{
	unsigned short start;
	unsigned short cruise;
	unsigned short off;
	unsigned short up,down;
	unsigned short max_speed_up, max_speed_down;
	unsigned short swtch;
	unsigned short mode;
	unsigned short lights;
    unsigned short auto_shutdow;
	unsigned short auto_lights_off;
    unsigned short next_drive;
//	unsigned short ios_power_on, ios_power_off;
//	unsigned short ios_mode;
} PUSH_CNT;

#define UART_BUFFER_SIZE 4
static char _input_buffer[UART_BUFFER_SIZE];
static char _output_buffer[UART_BUFFER_SIZE];

#define LCD_INPUT_TIMEOUT  (200)
static char throttle_timeout = LCD_INPUT_TIMEOUT / MAIN_LOOP_TIME;

void main()
{
	int result;
	hal_pwm_initialize(PWM_TIMER_MODULE, PWM_RANGE - 1, 200000);
	hal_pwm_set_output(PWM_TIMER_MODULE, 0, 1025);

	UART_CONTROL_BLOCK cb = (UART_CONTROL_BLOCK) { .Baudrate = 9600,
		.InputBuffer = (UART_BUFFER) { .Size = UART_BUFFER_SIZE, .Buffer = _input_buffer },
		.OutputBuffer = (UART_BUFFER) { .Size = UART_BUFFER_SIZE, .Buffer = _output_buffer }};
	uart_initialize(0, &cb);

	xcpu_sensor_initialize();
	PUSH_CNT push = { };
	XCPU_SPEED_DATA sp = { .speed = 0 };
	xcpu_speed_initialize();
	_pid_k = (PID_K) { .P = 25, .I = 15, .CMin = 0, .CMax = 255 };
	_speed_limit_pid_k = (PID_K) { .P = 25, .I = 15, .CMin = -255, .CMax = 0 };

	// TODO: CHECK THESE PARAMS
	exos_timer_create(&_timer, MAIN_LOOP_TIME, MAIN_LOOP_TIME, exos_signal_alloc());

	xcpu_can_initialize();
	xcpu_board_output(OUTPUT_NONE);

	if (!persist_load(&_storage))
	{
		_storage = (XCPU_PERSIST_DATA) { .Magic = XCPU_PERSIST_MAGIC,
				.TotalSteps = 0,
				.ConfigBits = XCPU_CONFIGF_NONE,
				.DriveMode = XCPU_DRIVE_MODE_SOFT,
				.WheelRatioAdj = 0,
				.ThrottleAdjMin = 66, 
				.ThrottleAdjMax = 166,
				.MaxSpeed = 40,
				.CustomCurve = {255, 255, 255, 255, 255, 255, 255} };
	}

	xcpu_bt_initialize();

	set_custom_curve_ptr(_storage.CustomCurve);

	_speed_limit_pid.SetPoint = 0.0f;
	_speed_limit_pid.Integral = 0.0f; //throttle / _pid_k.I;

	_control_state = CONTROL_OFF;
	XCPU_OUTPUT_MASK output_state = OUTPUT_NONE;
	XCPU_OUTPUT_MASK output_state_on = _default_output_state;

	_state = XCPU_STATE_OFF;
	unsigned char led = 0, led_duty;

	int throttle_target = 0;
	while(1)
	{
		exos_timer_wait(&_timer);
		led_duty = 1;
		int no_activity;

		char got_lcd_input;
		XCPU_EVENTS events = _can_read_messages(&got_lcd_input) | _bt_read_events();
		events |= _lcd.Events;

		// CAN communication breakdown, safe measures (throttle to 0)
		throttle_timeout--;
		if (got_lcd_input)
			throttle_timeout = LCD_INPUT_TIMEOUT / MAIN_LOOP_TIME;
		if (throttle_timeout <= 0)
			_lcd.ThrottleRaw = 0, throttle_timeout = 0;

#ifdef __XCPU_BLUETOOTH_VIA_UART__
		unsigned char bt_value;
		int done = uart_read(0, &bt_value, 1);
		if (done)
		{
			switch(bt_value)
			{
				case 'E': events |= XCPU_EVENT_TURN_ON;	break;
				case 'D': events |= XCPU_EVENT_TURN_OFF;	break;
			}
		}
#endif

		int curve_in = (int)(4096.f * (sp.speed / (float) MAX_SPEED));
		int throttle_factor = get_curve_value(curve_in, _drive_mode) >> 4;
		unsigned char throttle = (_lcd.ThrottleRaw * throttle_factor) >> 12;

#ifndef __XCPU_VIRTUAL__
		// read speed sensors
		xcpu_speed_calculation(&sp, (_state & XCPU_STATE_MILES) ? 1:0, (float)_storage.WheelRatioAdj);
#endif

		switch(_control_state)
		{
			case CONTROL_OFF:
				output_state = OUTPUT_NONE;
				if ((events & XCPU_EVENT_TURN_ON) ||
					_push_delay(xcpu_board_input(INPUT_STOP_BUT), &push.start, 10))
				{
					_control_state = CONTROL_ON;
					_state = (_storage.ConfigBits & XCPU_CONFIGF_MILES) ? XCPU_STATE_ON | XCPU_STATE_MILES : XCPU_STATE_ON;
					_state |= XCPU_STATE_NEUTRAL;
					_drive_mode = _storage.DriveMode;
                    output_state = output_state_on;
					_run_diag();
				}
				break;
			case CONTROL_LIGHTS_OFF_STAND_BY:
				output_state = OUTPUT_NONE;
				led_duty = 3;
				_state |= XCPU_STATE_NEUTRAL;

				// Shut down when activity ceases for 60 seconds 
				no_activity = (_lcd.Buttons == 0) && (events == 0) && (sp.speed < 0.1f);
				if(_push_delay(no_activity, &push.auto_shutdow, MAIN_LOOP_FREQ * 30))
				{
					push.auto_shutdow = 0;
					_shutdown(&sp);
				}
				else if (!no_activity)	// Disable pre-sleep, lights on
				{
					_control_state = CONTROL_ON;
					output_state = output_state_on;
				}
				break;
			case CONTROL_CRUISE:
				{
					int input_throttle = throttle;
					throttle = pid(&_pid, sp.speed, &_pid_k, MAIN_LOOP_TIME / 1000.0f);
					if (throttle > 250)
					{
						throttle--;
					}

					if (input_throttle > (throttle + 10))
						throttle = input_throttle;

					// Exit if brake pressed, or CAN comm. interrupted
					if ((output_state & OUTPUT_BRAKEL) || (throttle_timeout == 0))
					{
						_control_state = CONTROL_ON;
						_state &= ~XCPU_STATE_CRUISE_ON;
					}
				}
				// NOTE: case fall down
			case CONTROL_ON:
				output_state = (_state & XCPU_STATE_LIGHT_OFF) ? 
					output_state & ~(OUTPUT_HEADL | OUTPUT_HIGHBL | OUTPUT_TAILL) :
					output_state_on;
                led_duty = 6;

				// Speed limit brake
				float speed_excess = sp.speed - _storage.MaxSpeed;
				if (speed_excess > 0.0f)
				{
					float speed_limiter = pid(&_speed_limit_pid, speed_excess, &_speed_limit_pid_k, MAIN_LOOP_TIME / 1000.0f);
					speed_limiter = __LIMIT(speed_limiter, -255.0f, 0.0f);
					throttle = __LIMIT(((int)throttle + (int)speed_limiter), 0, 255);
				}

#ifndef DISABLE_STAND_BY
				// Lights off when activity ceases for 30 seconds
				no_activity = (_lcd.Buttons == 0) && (events == 0) && (sp.speed < 0.1f);
				if(_push_delay(no_activity, &push.auto_lights_off, MAIN_LOOP_FREQ * 300))
				{
                    push.auto_lights_off = 0;
					_control_state = CONTROL_LIGHTS_OFF_STAND_BY;
				}
#endif

				if (_lcd.BrakeRear > BRAKE_THRESHOLD || _lcd.BrakeFront > BRAKE_THRESHOLD)
				{
					output_state |= OUTPUT_BRAKEL;
                    //  Rear brake disables throttling
					int dis_throttle = _lcd.BrakeFront > BRAKE_THRESHOLD;
					if (_drive_mode != XCPU_DRIVE_MODE_RACING)
						dis_throttle |= _lcd.BrakeRear > BRAKE_THRESHOLD;
					if (dis_throttle)
					{
						throttle = 0;
						output_state |= OUTPUT_EBRAKE;
					}
				}

				if (_push_delay(_lcd.Events & XCPU_EVENT_SWITCH_LIGHTS, &push.lights, 5))
					_state ^= XCPU_STATE_LIGHT_OFF;
		
				if ((_lcd.Buttons & XCPU_BUTTON_HORN) && !(_lcd.Events & XCPU_EVENT_CONFIGURING))
					output_state |= OUTPUT_HORN;

				if (_push_delay(_lcd.Events & XCPU_EVENT_ADJUST_UP, &push.up, 5) &&
					_storage.WheelRatioAdj < 10)
				{
					_storage.WheelRatioAdj++;
				}
				if (_push_delay(_lcd.Events & XCPU_EVENT_ADJUST_DOWN, &push.down, 5) &&
					_storage.WheelRatioAdj > -10)
				{
					_storage.WheelRatioAdj--;
				}
				if (_push_delay(_lcd.Events & XCPU_EVENT_ADJUST_MAX_SPEED_UP, &push.max_speed_up, 5) &&
					_storage.MaxSpeed < MAX_SPEED)
				{
					_storage.MaxSpeed++;
				}
				if (_push_delay(_lcd.Events & XCPU_EVENT_ADJUST_MAX_SPEED_DOWN, &push.max_speed_down, 5) &&
					_storage.MaxSpeed > 0)
				{
					_storage.MaxSpeed--;
				}
				if (_push_delay(_lcd.Events & XCPU_EVENT_SWITCH_UNITS, &push.swtch, 5))
				{
					_state ^= XCPU_STATE_MILES;
				}
				if (_push_delay(_lcd.Events & XCPU_EVENT_NEXT_DRIVE_MODE, &push.next_drive, 5))
				{
					_drive_mode = (_drive_mode + 1) % XCPU_DRIVE_MODE_COUNT;
				}
				if (_push_delay(_lcd.Buttons & XCPU_BUTTON_CRUISE, &push.cruise, 5))
				{
					// Enable/disable NEUTRAL
					if (_state & XCPU_STATE_NEUTRAL)
					{	
						_state ^= XCPU_STATE_NEUTRAL;	// Exit from neutral
					}
					else
					{
						if (sp.speed > 10.0f)	// Enable neutral (if speed > 10 km/h)
						{
							_pid.SetPoint = sp.speed;
							_pid.Integral = throttle / _pid_k.I;

							_state ^= XCPU_STATE_CRUISE_ON;
							_control_state = _state & XCPU_STATE_CRUISE_ON ? CONTROL_CRUISE : CONTROL_ON;
						}
						if (sp.speed < 0.1f)
							_state ^= XCPU_STATE_NEUTRAL;	// Enable neutral
					}
				}

				if ((events & XCPU_EVENT_TURN_OFF) ||
					_push_delay(xcpu_board_input(INPUT_STOP_BUT), &push.start, MAIN_LOOP_FREQ) ||
					_push_delay(_lcd.Buttons & XCPU_BUTTON_CRUISE, &push.off, 50))
				{
					if (sp.speed <= 0.1f)  // 0.0f value is forced
					{
						_shutdown(&sp);
					}
					else 
						_state |= XCPU_STATE_WARNING;
				}

				if ((events & XCPU_EVENT_ENTER_BOOTLOADER) &&
					_state == XCPU_STATE_OFF)
				{
					persist_enter_bootloader();
				}

				// TODO: check bms and engine controller

				// update throttle out
				if (_control_state != CONTROL_OFF)
				{
					unsigned char th_lim = MOTOR_OFFSET + ((throttle * MOTOR_RANGE) >> 8);
                    if (_state & XCPU_STATE_NEUTRAL)
						th_lim = 0;
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
					if (sp.speed < 0) sp.speed = 0;
#endif
				}
				break;                          
			}

			xcpu_board_led(led < led_duty);
			led++; if (led >= MAIN_LOOP_FREQ) led = 0;

			float dist = (((_storage.TotalSteps + sp.s_partial) * (sp.ratio / 3.6f)) / 100);
			
			_can_send_messages((unsigned int)sp.speed, (unsigned long)dist, throttle);
			_bt_send_messages((unsigned int)sp.speed, (unsigned long)dist, throttle);
			xcpu_board_output(output_state);
        }
}

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
		case XCPU_CMD_ADJUST_THROTTLE:
        	_storage.ThrottleAdjMin = input->Data[0];
			_storage.ThrottleAdjMax = input->Data[1];
			break;
		case XCPU_CMD_INVOKE_BOOTLOADER: 
			return XCPU_EVENT_TURN_OFF | XCPU_EVENT_ENTER_BOOTLOADER;
	}
	return 0;
}

static XCPU_EVENTS _can_read_messages(char *got_lcd_input)
{
	XCPU_EVENTS cmd_events = 0;
	XCPU_MSG *xmsg;
    *got_lcd_input = 0;
	while(xmsg = xcpu_can_get_message(), xmsg != NULL)
	{
		CAN_BUFFER *data = &xmsg->CanMsg.Data;
		switch (xmsg->CanMsg.EP.Id)
		{
			case 0x200:
				_lcd = *((XCPU_MASTER_INPUT *)data);
                *got_lcd_input = 1;
				break;

			case 0x201:
				cmd_events |= _do_lcd_command((XCPU_MASTER_INPUT2 *)data);
				break;
		}
		xcpu_can_release_message(xmsg);
	}
	return cmd_events;
}

static void _can_send_messages(unsigned speed, unsigned long dist, unsigned throttle)
{
	XCPU_MASTER_OUT1 report = (XCPU_MASTER_OUT1) {
		.Speed = speed, 
		.BattLevel = xcpu_sensor_batt_level(),
		.State = _state,
		.DriveMode = _drive_mode,
		.Distance = dist };
	XCPU_MASTER_OUT2 adj = (XCPU_MASTER_OUT2) {
		.ThrottleMax = _storage.ThrottleAdjMax,
		.ThrottleMin = _storage.ThrottleAdjMin,
		.WheelRatio = _storage.WheelRatioAdj,
		.MaxSpeed = _storage.MaxSpeed };
	xcpu_can_send_messages(&report, &adj);
	//&_storage.CustomCurve[0]);
}

static XCPU_EVENTS _bt_read_events()
{
#ifdef BOARD_XKUTY_CPU1_EXTENDED
	XCPU_BT_CMD_CHAR_DATA cmd;
	if (xcpu_bt_get_cmd(&cmd))
	{
		switch(cmd.Command)
		{
			case XCPU_BT_CMD_POWER:
				return cmd.Param ? XCPU_EVENT_TURN_ON : XCPU_EVENT_TURN_OFF;
			case XCPU_BT_CMD_DRIVE_MODE:
				_drive_mode = cmd.Param;
				break;
		}
	}
#endif
	return 0;
}

static void _bt_send_messages(unsigned speed, unsigned long dist, unsigned throttle)
{
#ifdef BOARD_XKUTY_CPU1_EXTENDED
	XCPU_BT_STATE_CHAR_DATA report = (XCPU_BT_STATE_CHAR_DATA) {
		.Speed = speed, 
		.BattLevel = xcpu_sensor_batt_level(),
		.State = _state,
		.DriveMode = _drive_mode,
		.Distance = dist };
	xcpu_bt_update(&report);
#endif
}

static int _push_delay(int push, unsigned short *state, int limit)
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

static void _shutdown(XCPU_SPEED_DATA *sp)
{
	// disable pwm
	hal_pwm_set_output(PWM_TIMER_MODULE, 0, PWM_RANGE + 1); 

	_storage.TotalSteps += sp->s_partial;
	sp->s_partial = 0;
	_storage.ConfigBits = XCPU_CONFIGF_NONE;
	if (_state & XCPU_STATE_MILES) 
		_storage.ConfigBits |= XCPU_CONFIGF_MILES;

	_storage.DriveMode = _drive_mode;

	persist_save(&_storage);

//	_output_state = OUTPUT_NONE;
	_control_state = CONTROL_OFF;
	_state = XCPU_STATE_OFF;
}

static void _run_diag()
{
}