// LPC17xx RTC Peripheral Support
// by Miguel Fides

#include "rtc.h"
#include "cpu.h"
#include <kernel/datetime.h>

static void _wait(int c);

int rtc_initialize()
{
	// module initialization
	LPC_SC->PCONP |= PCONP_PCRTC;	// power enable module

	LPC_RTC->CCR = RTC_CCR_CTCRST;	// reset / ext_clk
	_wait(100);
	LPC_RTC->CCR = RTC_CCR_CLKEN;
	_wait(100);
}

int rtc_get_datetime(EXOS_DATETIME *datetime)
{
	unsigned long ctime1 = LPC_RTC->CTIME1;
	unsigned long ctime2 = LPC_RTC->CTIME1;

	*datetime = (EXOS_DATETIME) {
		.Hours = (ctime1 >> 16) & 0x1F,
		.Minutes = (ctime1 >> 8) & 0x3F,
		.Seconds = ctime1 & 0x3F,
		.Day = ctime2 & 0x1F,
		.Month = (ctime2 >> 8) & 0xF,
		.Year = (ctime2 >> 16) & 0xFFF };

	return 1;
}

int rtc_set_datetime(const EXOS_DATETIME *datetime)
{
	// stop
	LPC_RTC->CCR &= ~RTC_CCR_CLKEN;
	LPC_RTC->CCR |= RTC_CCR_CTCRST;
	
	LPC_RTC->DOM = datetime->Day;
	LPC_RTC->MONTH = datetime->Month;
	LPC_RTC->YEAR = datetime->Year;
	LPC_RTC->DOW = datetime->DayOfWeek;
	LPC_RTC->DOY = exos_datetime_day_of_year(datetime);
	
	LPC_RTC->HOUR = datetime->Hours;
	LPC_RTC->MIN = datetime->Minutes;
	LPC_RTC->SEC = datetime->Seconds;

	// restart
	LPC_RTC->CCR &= ~RTC_CCR_CTCRST;
	LPC_RTC->CCR |= RTC_CCR_CLKEN;
}

static void _wait(int c)
{
	for (int volatile i = 0; i < c; i++);
}







