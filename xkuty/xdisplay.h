#ifndef XDISPLAY_H
#define XDISPLAY_H

#include "xcpu.h"

typedef enum
{
	ST_INTRO_SECTION = 1,
    ST_LOGO_IN,
    ST_LOGO_SHOW,
    ST_LOGO_OUT,
    ST_EXOS_IN,
    ST_EXOS_SHOW,
    ST_EXOS_OUT,
	ST_INTRO_SECTION_END,
	//
    ST_DASH,
	ST_DEBUG_INPUT,
	ST_FACTORY_MENU,	// Disabled
    ST_ADJUST_WHEEL_DIA,
	ST_ADJUST_THROTTLE_MAX,
	ST_ADJUST_THROTTLE_MIN,
	ST_USER_MENU,
	ST_ADJUST_MAX_SPEED,
	ST_SHOW_PHONES,
	ST_ADD_PHONE,
} DISPLAY_STATE;

typedef struct
{
	char active;
	char name[17];
} PHONE_REG;

typedef struct 
{
	int CpuStatus;
	int Speed;
	unsigned long Distance;
	int battery_level_fx8;
	int SpeedAdjust;
	XCPU_DRIVE_MODE DriveMode;
	unsigned char ThrottleMin;
	unsigned char ThrottleMax;
	unsigned char CustomCurve[7];
	PHONE_REG PhoneList[6]; // Max 5 phones. The 6 th one is the new candidate
	int CurrentMenuOption;
	//int AppliedThrottle;
	int MaxSpeed;
} DASH_DATA;

void xdisplay_initialize();
void xdisplay_clean_screen();
void xdisplay_dump();
void xdisplay_intro(DISPLAY_STATE *state, int *st_time_base, int time, int intro_next_state);
void xdisplay_runtime_screens(DISPLAY_STATE state, DASH_DATA *dash);

#endif // XDISPLAY_H

