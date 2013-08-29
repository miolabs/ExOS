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
	ST_FACTORY_MENU,
    ST_ADJUST_WHEEL_DIA,
	ST_ADJUST_THROTTLE_MAX,
	ST_ADJUST_THROTTLE_MIN,
	ST_ADJUST_DRIVE_MODE,
} DISPLAY_STATE;

typedef struct {
	int SpeedAdjust;
	XCPU_DRIVE_MODE DriveMode;
	unsigned char ThrottleMin;
	unsigned char ThrottleMax;
} DASH_CONFIG;

typedef struct {
	int CpuStatus;
	int Speed;
	unsigned long Distance;
	int battery_level_fx8;
	DASH_CONFIG ActiveConfig;
	DASH_CONFIG CurrentConfig;
	int CurrentMenuOption;
} DASH_DATA;

void xdisplay_initialize();
void xdisplay_clean_screen();
void xdisplay_dump();
void xdisplay_intro(DISPLAY_STATE *state, int *st_time_base, int time);
void xdisplay_runtime_screens(DISPLAY_STATE state, DASH_DATA *dash);

#endif // XDISPLAY_H

