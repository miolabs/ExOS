#ifndef LPC17_RTC_H
#define LPC17_RTC_H

#include <kernel/datetime.h>

#define RTC_ILR_RTCCIF (1<<0)
#define RTC_ILR_RTCALF (1<<1)
#define RTC_ILR_RTSSF (1<<2)

#define RTC_CCR_CLKEN (1<<0)
#define RTC_CCR_CTCRST (1<<1)
#define RTC_CCR_CCALEN (1<<4)

int rtc_initialize();
int rtc_get_datetime(EXOS_DATETIME *datetime);
int rtc_set_datetime(const EXOS_DATETIME *datetime);

#endif // LPC17_RTC_H


