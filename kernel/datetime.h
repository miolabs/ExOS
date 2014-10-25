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

typedef struct 
{
	unsigned long Days;
	unsigned long MilliSeconds;
} EXOS_TIMESPAN;

#define EXOS_SECONDS_IN_DAY (24*60*60)

//prototypes
void exos_datetime_now(EXOS_DATETIME *datetime);

#endif // EXOS_DATETIME_H
