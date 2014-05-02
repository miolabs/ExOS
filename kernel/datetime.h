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


#endif // EXOS_DATETIME_H
