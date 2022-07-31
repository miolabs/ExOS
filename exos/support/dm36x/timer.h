#ifndef DM36X_TIMER_H
#define DM36X_TIMER_H

#include "intc.h"

struct __TIMER_TCR
{
	unsigned :6;
	unsigned ENAMODE12:2;
	unsigned CLKSRC12:1;
	unsigned :1;
	unsigned READRSTMODE12:1;
	unsigned CAPMODE12:1;
	unsigned CAPEVTMODE12:2;
	unsigned :2;
	unsigned :6;
	unsigned ENAMODE34:2;
	unsigned CLKSRC34:1;
	unsigned :1;
	unsigned READRSTMODE34:1;
	unsigned CAPMODE34:1;
	unsigned CAPEVTMODE34:2;
	unsigned :2;
};

struct __TIMER_INTCTL
{
	unsigned CMP_INT_EN12:1;
	unsigned CMP_INT_STAT12:1;
	unsigned EVT_INT_EN12:1;
	unsigned EVT_INT_STAT12:1;
	unsigned :10;
	unsigned EVAL12:1;
	unsigned SET12:1;
	unsigned CMP_INT_EN34:1;
	unsigned CMP_INT_STAT34:1;
	unsigned EVT_INT_EN34:1;
	unsigned EVT_INT_STAT34:1;
	unsigned :10;
	unsigned EVAL34:1;
	unsigned SET34:1;
};

typedef volatile struct
{
	unsigned long PID12;
	unsigned long EMUMGT;
	unsigned long Reserved08;
	unsigned long Reserved0C;
	unsigned long TIM12;
	unsigned long TIM34;
	unsigned long PRD12;
	unsigned long PRD34;
	union
	{
		unsigned long TCR;
		struct __TIMER_TCR TCRbits;
	};
	unsigned long TGCR;
	unsigned long WDTCR;
	// NOTE: below registers are only for new
	unsigned long Reserved2C;
	unsigned long Reserved30;
	unsigned long REL12;
	unsigned long REL34;
	unsigned long CAP12;
	unsigned long CAP34;
	union
	{
		unsigned long INTCTL_STAT;
		struct __TIMER_INTCTL INTCTL_STATbits;
	};
} TIMER_MODULE;

typedef enum
{
	TIMER_MODE_DISABLED = 0,
	TIMER_MODE_ONCE = 1,
	TIMER_MODE_CONTINUOUS = 2,
	TIMER_MODE_CONTINUOUS_RELOAD = 3,
} TIMER_MODE;

#define TCR_ENAMODE12_BIT 6
#define TCR_ENAMODE23_BIT 22

typedef enum
{
	TGCR_TIMMODE_64BIT_GP = 0,
	TGCR_TIMMODE_DUAL_UNCHAINED = 1,
	TGCR_TIMMODE_64BIT_WD = 2,
	TGCR_TIMMODE_DUAL_CHAINED = 3,
} TGCR_TIMMODE;

#define TGCR_TIMMODE_BIT 2
#define TGCR_TIM12RS (1<<0)
#define TGCR_TIM34RS (1<<1)

typedef void (*MATCH_FN)(int last, int actual);


// prototypes
TIMER_MODULE *timer_initialize(int timer, int freq, int period, TIMER_MODE mode);
void timer_control(int timer, int enable);
void timer_set_handler(int timer, MATCH_FN fn, INTC_PRI pri);

#endif // DM36X_TIMER_H
