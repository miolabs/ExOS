#ifndef EXOS_DATETIME_H
#define EXOS_DATETIME_H

typedef struct _EXOS_DATETIME
{
	unsigned char Seconds;
	unsigned char Minutes;
	unsigned char Hours;
	unsigned char DayOfWeek;
	unsigned char Day;
	unsigned char Month;
	unsigned short Year;
} EXOS_DATETIME;

typedef enum
{
	EXOS_SUNDAY = 0,
	EXOS_MONDAY,
	EXOS_TUESDAY,
	EXOS_WEDNESDAY,
	EXOS_THURSDAY,
	EXOS_FRIDAY,
	EXOS_SATURDAY,
} EXOS_DAYOFWEEK;


typedef struct 
{
	int Days;
	int Milliseconds;
} EXOS_TIMESPAN;

#define EXOS_SECONDS_IN_DAY (24*60*60)

//prototypes
const char *exos_datetime_day_name(EXOS_DAYOFWEEK day);
const char *exos_datetime_month_name(int month);
int exos_datetime_print(char *s, EXOS_DATETIME *datetime);
void exos_datetime_now(EXOS_DATETIME *datetime);
void exos_datetime_seek(const EXOS_DATETIME *base, EXOS_TIMESPAN *elapsed, EXOS_DATETIME *target);
int exos_datetime_print(char *s, EXOS_DATETIME *datetime);

#endif // EXOS_DATETIME_H
