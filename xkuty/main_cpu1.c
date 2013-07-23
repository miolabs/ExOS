#include <kernel/thread.h>
#include <kernel/port.h>
#include <kernel/timer.h>
#include <support/can_hal.h>
#include <support/pwm_hal.h>
#include "xcpu.h"
#include "xcpu/board.h"
#include "xcpu/speed.h"
#include "xcpu/persist.h"
#include "xcpu/throttle_curves.h"
#include "pid.h"


#define BRAKE_THRESHOLD 30
#define PWM_RANGE 100
#define MOTOR_OFFSET 50
#define MOTOR_RANGE (160 - 50)
#define WHEEL_RATIO_KMH (12.9 / 47)
#define WHEEL_RATIO_MPH (WHEEL_RATIO_KMH / 1.60934)

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

typedef struct 
{
	unsigned short buttons;
	unsigned char  throttle;
	unsigned char  brake_left, brake_right;
	unsigned char  throttle_adj_min, throttle_adj_max;
} CAN_INPUT;

static CAN_INPUT _c_i = {0,0,0,0,0,0};

typedef struct
{
	float dt2, speed, dt, ratio, s_partial;
} SPEED_DATA;

static SPEED_DATA _sp = { 0.0f, 0.0f,  0.0f,  0.0f,  0.0f};

static void _speed_calculation ( SPEED_DATA* sp, int mag_miles, float wheel_ratio_adjust)
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
		sp->speed = sp->dt != 0 ? (int)((space / sp->dt) *  sp->ratio) : 99;
		sp->s_partial += space;
		dt2 = sp->dt;
		sp->dt = 0;
		speed_wait = 0;
	}
	else
	{
		if (speed_wait < 50) 
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
} PUSH_CNT;

static enum
{
        CONTROL_OFF = 0,
        CONTROL_ON,
        CONTROL_CRUISE,
} _control_state;
static XCPU_STATE _state; // comm state to share (with lcd)

static XCPU_PERSIST_DATA _storage;

void main()
{
	int result;
	hal_pwm_initialize(PWM_TIMER_MODULE, PWM_RANGE - 1, 200000);
	hal_pwm_set_output(PWM_TIMER_MODULE, 0, 1025);

	PUSH_CNT      push = { 0,0,0,0,0,0,0,0};

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

	CURVE_MODE    drive_mode = CURVE_SOFT;

	unsigned char led = 0;
        
	if (!persist_load(&_storage))
	{
		_storage = (XCPU_PERSIST_DATA) { .Magic = XCPU_PERSIST_MAGIC,
				.TotalSteps = 0,
				.ConfigBits = XCPU_CONFIGF_NONE,
				.WheelRatioAdj = 0,
				.ThrottleAdjMin = 66, 
				.ThrottleAdjMax = 166};
	}

	_control_state = CONTROL_OFF;
	_output_state = OUTPUT_NONE;
	_state = XCPU_STATE_OFF;

	int throttle_target = 0;
	while(1)
	{
		exos_timer_wait(&_timer);

		XCPU_MSG *xmsg;
		while(NULL != (xmsg = (XCPU_MSG *)exos_port_get_message(&_can_rx_port, 0)))
		{
			if (xmsg->CanMsg.EP.Id == 0x200)
			{
				CAN_BUFFER *data = &xmsg->CanMsg.Data;
				_c_i.buttons = data->u8[0] | (data->u8[7] << 8);
				unsigned int throttle_raw = data->u8[1] | ( data->u8[2] << 8);
				_c_i.brake_left = data->u8[3];
				_c_i.brake_right = data->u8[4];
				_c_i.throttle_adj_min = data->u8[5];
				_c_i.throttle_adj_max = data->u8[6];

				_c_i.throttle = get_curve_value ( throttle_raw, drive_mode ) >> 4;
			}

			exos_fifo_queue(&_can_free_msgs, (EXOS_NODE *)xmsg);
		}

		// read sensors
		_speed_calculation ( &_sp, (_state & XCPU_STATE_MILES) ? 1:0, (float)_storage.WheelRatioAdj);

		int space = speed_read(&_sp.dt2);

		switch(_control_state)
		{
			case CONTROL_OFF:
				_output_state = OUTPUT_NONE;
				if (_push_delay(xcpu_board_input(INPUT_BUTTON_START), &push.start, 10))
				{
					_control_state = CONTROL_ON;
					_state = (_storage.ConfigBits & XCPU_CONFIGF_MILES) ? XCPU_STATE_ON | XCPU_STATE_MILES : XCPU_STATE_ON;
					_state |= XCPU_STATE_NEUTRAL;
					drive_mode = ( _storage.ConfigBits & XCPU_CONFIGG_DRIVE_MODE) >> XCPU_CONFIGG_DRIVE_MODE_SHIFT;
					_run_diag();
				}
				break;
			case CONTROL_CRUISE:
				{
					int input_throttle = _c_i.throttle;
					_c_i.throttle = pid(&_pid, _sp.speed, &_pid_k, 0.05F);
					if (_c_i.throttle > 250)
					{
						_c_i.throttle--;
					}
					int release = ( input_throttle - _c_i.throttle) > 20;
					if ((_output_state & OUTPUT_BRAKEL) || release)
					{
						_control_state = CONTROL_ON;
						_state &= ~XCPU_STATE_CRUISE_ON;
					}
				}
			case CONTROL_ON:
				_output_state = _default_output_state;
				if (_c_i.brake_left > BRAKE_THRESHOLD || _c_i.brake_right > BRAKE_THRESHOLD)
				{
					_output_state |= (OUTPUT_BRAKEL | OUTPUT_EBRAKE);
					_c_i.throttle = 0;
				}
				if ( _c_i.buttons & XCPU_BUTTON_LIGHTS_OFF)
					_default_output_state = OUTPUT_NONE;
				if (_c_i.buttons & XCPU_BUTTON_HORN)
					_output_state |= OUTPUT_HORN;

				if (_push_delay(_c_i.buttons & XCPU_BUTTON_ADJUST_UP, &push.up, 5) &&
					_storage.WheelRatioAdj < 10)
				{
					_storage.WheelRatioAdj++;
				}
				if (_push_delay(_c_i.buttons & XCPU_BUTTON_ADJUST_DOWN, &push.down, 5) &&
					_storage.WheelRatioAdj > -10)
				{
					_storage.WheelRatioAdj--;
				}
				if (_push_delay(_c_i.buttons & XCPU_BUTTON_SWITCH_UNITS, &push.swtch, 5))
				{
					_state ^= XCPU_STATE_MILES;
				}

				if (_push_delay( _c_i.buttons & XCPU_BUTTON_ADJ_DRIVE_MODE, &push.mode, 5))
				{
					drive_mode = (_c_i.buttons & XCPU_BUTTON_ADJ_DRIVE_MODE) >> XCPU_BUTTON_ADJ_DRIVE_MODE_SHIFT;
				}

				if (_push_delay( _c_i.buttons & XCPU_BUTTON_ADJ_THROTTLE, &push.adj, 5))
				{
					_storage.ThrottleAdjMin = _c_i.throttle_adj_min;
					_storage.ThrottleAdjMax = _c_i.throttle_adj_max;
				}

				if (_push_delay(_c_i.buttons & XCPU_BUTTON_CRUISE, &push.cruise, 5))
				{
					if ( _sp.speed == 0)
					{
						_state ^= XCPU_STATE_NEUTRAL;
					}
					else
					{
						if ( _sp.speed > 0)
						{
							_pid.SetPoint = _sp.speed;
							_pid.Integral = _c_i.throttle / _pid_k.I;

							_state ^= XCPU_STATE_CRUISE_ON;
							_control_state = _state & XCPU_STATE_CRUISE_ON ? CONTROL_CRUISE : CONTROL_ON;
						}
					}
				}

				if (_push_delay(xcpu_board_input(INPUT_BUTTON_START), &push.start, 10) ||
					_push_delay(_c_i.buttons & XCPU_BUTTON_CRUISE, &push.off, 100))
				{
					if (_sp.speed == 0)
					{
						hal_pwm_set_output(PWM_TIMER_MODULE, 0, PWM_RANGE + 1); //      disable pwm

						_storage.TotalSteps += _sp.s_partial;
						_sp.s_partial = 0;
						_storage.ConfigBits = XCPU_CONFIGF_NONE;
						if (_state & XCPU_STATE_MILES) 
							_storage.ConfigBits |= XCPU_CONFIGF_MILES;

						_storage.ConfigBits |= drive_mode << XCPU_CONFIGG_DRIVE_MODE_SHIFT;

						persist_save(&_storage);

						_output_state = OUTPUT_NONE;
						_control_state = CONTROL_OFF;
						_state = XCPU_STATE_OFF;
					}
					else 
						_state |= XCPU_STATE_WARNING;
				}

				// TODO: check battery and engine

				// update throttle out
				if (_control_state != CONTROL_OFF)
				{
					unsigned char th_lim = (_state & XCPU_STATE_NEUTRAL) ? 0 
							: MOTOR_OFFSET + (( _c_i.throttle * MOTOR_RANGE) >> 8);
					hal_pwm_set_output(PWM_TIMER_MODULE, 0, PWM_RANGE - ((th_lim * PWM_RANGE) >> 8));
				}
				break;                          
			}

			xcpu_board_led(led = !led);     // toggle led

			CAN_BUFFER buf;

			buf.u32[0] = 0;
			buf.u32[1] = 0;
			buf.u8[0]  = _storage.ThrottleAdjMin;
			buf.u8[1]  = _storage.ThrottleAdjMax;
			buf.u16[1] = drive_mode;
			hal_can_send((CAN_EP) { .Id = 0x301 }, &buf, 8, CANF_NONE);

			exos_thread_sleep (5);  // Temporary fix; packets get overwritten

			buf.u8[0] = _sp.speed;
			buf.u8[1] = _c_i.throttle; // batt
			buf.u8[2] = _state;
			buf.u8[3] = _storage.WheelRatioAdj;
			buf.u32[1] = (unsigned long)(((_storage.TotalSteps + _sp.s_partial) * ( _sp.ratio / 3.6f)) / 100);
			hal_can_send((CAN_EP) { .Id = 0x300 }, &buf, 8, CANF_NONE);

			exos_thread_sleep (5);  // IDEM

			xcpu_board_output(_output_state);
        }
}

void hal_can_received_handler(int index, CAN_MSG *msg)
{
        XCPU_MSG *xmsg;
        switch(msg->EP.Id)
        {
                case 0x200:
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
                case 0x201:
                        // TODO
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