#include "datetime.h"
#include "timer.h"
#include "panic.h"
#include <stdio.h>
#include <support/services/debug.h>

const EXOS_TIMESPAN EXOS_TIMESPAN_ZERO = { /* all zero */ };

static EXOS_DATETIME _boot = { .Day = 1, .Month = 1, .Year = 2000, .DayOfWeek = EXOS_SATURDAY };
static int _seconds = 0;
static const char *_days[] = { "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };
static const char *_month[] = { "January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December" };

const char *exos_datetime_day_name(EXOS_DAYOFWEEK day)
{
	return (day < 7) ? _days[day] : "Undefined";
}

const char *exos_datetime_month_name(int month)
{
	return (month > 0 && month <= 12) ? _month[month - 1] : "Undefined";
}

int exos_datetime_print(char *s, EXOS_DATETIME *datetime)
{
#ifdef DEBUG
	if (s == NULL || datetime == NULL)
		kernel_panic(KERNEL_ERROR_NULL_POINTER);
#endif
	return sprintf(s, "%s, %s %d, %d %02d:%02d:%02d", 
		exos_datetime_day_name(datetime->DayOfWeek),
		exos_datetime_month_name(datetime->Month), datetime->Day, datetime->Year,
		datetime->Hours, datetime->Minutes, datetime->Seconds);
}

static inline int _biy(int year)
{
	return (year & 3) ? 1 : ((year % 100) ? 1 : !(year % 400));
}

int exos_datetime_day_of_year(const EXOS_DATETIME *datetime)
{
	int days = 0;
	switch(datetime->Month)
	{
		case 12: days += 30;
		case 11:	days += 31;
		case 10: days += 30;
		case 9:	days += 31;
		case 8: days += 31;
		case 7: days += 30;
		case 6: days += 31;
		case 5:	days += 30;
		case 4: days += 31;
		case 3:	days += _biy(datetime->Year) ? 29 : 28;
		case 2:	days += 31;
	}
	days += datetime->Day - 1;
	return days;
}

static int _date2days(const EXOS_DATETIME *datetime)
{
	int years = datetime->Year - 1; 
	int ry = years;
	int qc = ry / 400; ry %= 400;
	int c = ry / 100; ry %= 100;
	int by = ry / 4; ry %= 4;
	int days = ((97 * qc) + (24 * c) + by) + (years * 365);
	
#if 0
	debug_printf("year %d: qc=%d, c=%d, by=%d, ry=%d, days=%d\r\n", 
		datetime->Year, qc, c, by, ry, days);
#endif

	days += exos_datetime_day_of_year(datetime);
	return days;
}

static void _days2date(EXOS_DATETIME *datetime, int days)
{
	datetime->DayOfWeek = (days + 1) % 7;
	datetime->Year = 1;
	datetime->Year += (days / 146097) * 400; days %= 146097;
	datetime->Year += (days / 36524) * 100; days %= 36524;
	datetime->Year += (days / 1461) * 4; days %= 1461;
	datetime->Year += (days / 365); days %= 365;
	int month = 1;
	if (days >= 31) { month++; days -= 31;
		int feb_days = _biy(datetime->Year) ? 29 : 28;
		if (days >= feb_days) { month++; days -= feb_days;
			if (days >= 31) { month++; days-= 31;
				if (days >= 30) { month++; days-= 30;
			if (days >= 31) { month++; days-= 31;
				if (days >= 30) { month++; days-= 30;
			if (days >= 31) { month++; days-= 31;
				if (days >= 30) { month++; days-= 31;
			if (days >= 30) { month++; days-= 30;
				if (days >= 30) { month++; days-= 31;
			if (days >= 30) { month++; days-= 30; 
		}}}}}}}}}}}
	datetime->Month = month;
	datetime->Day = days + 1;
}

static inline int _divrem(signed *pd, unsigned m)
{
	int d = *pd;

	if (d < 0)
	{
		int q = d / (signed)m;
		int rem = d % (signed)m;
		if (rem != 0) q--, rem += m;
		*pd = q;
		return rem;
	}
	else if (d >= (signed)m)
	{
		*pd = d / (signed)m;
		return (d % (signed)m);
	}
	*pd = 0;
	return d;
}

static void _seek(const EXOS_DATETIME *date1, int seconds, EXOS_DATETIME *date2)
{
	int delta = date1->Seconds + seconds;
	date2->Seconds = _divrem(&delta, 60);
	delta += date1->Minutes;
	date2->Minutes = _divrem(&delta, 60);
	delta += date1->Hours;
	date2->Hours = _divrem(&delta, 24);

	delta += _date2days(date1);
	_days2date(date2, delta);
}

void exos_datetime_seek(const EXOS_DATETIME *base, const EXOS_TIMESPAN *elapsed, EXOS_DATETIME *target)
{
#ifdef DEBUG
	if (base == NULL || elapsed == NULL || target == NULL)
		kernel_panic(KERNEL_ERROR_NULL_POINTER);
#endif

	int seconds =  (elapsed->Milliseconds / 1000) + (elapsed->Days * EXOS_SECONDS_IN_DAY);
	_seek(base, seconds, target); 
}

void exos_datetime_boot_setup(const EXOS_DATETIME *datetime)
{
#ifdef DEBUG
	if (datetime == NULL)
		kernel_panic(KERNEL_ERROR_NULL_POINTER);
#endif
	_seek(datetime, -_seconds, &_boot);
}

void exos_datetime_now(EXOS_DATETIME *datetime)
{
#ifdef DEBUG
	if (datetime == NULL)
		kernel_panic(KERNEL_ERROR_NULL_POINTER);
#endif
	_seek(&_boot, _seconds, datetime);
}

void __timer_second_tick()
{
	_seconds++;
}
