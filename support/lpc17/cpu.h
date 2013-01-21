#ifndef LPC17_CPU_H
#define LPC17_CPU_H

#include <CMSIS/LPC17xx.h>

#define LPC1769 1769

#define PCONP_PCTIM0	0x00000002
#define PCONP_PCTIM1	0x00000004
#define PCONP_PCUART0	0x00000008
#define PCONP_PCUART1	0x00000010
#define PCONP_PCPWM0	0x00000020
#define PCONP_PCPWM1	0x00000040
#define PCONP_PCI2C0	0x00000080
#define PCONP_PCSPI		0x00000100
#define PCONP_PCRTC		0x00000200
#define PCONP_PCSSP1	0x00000400
#define PCONP_PCEMC		0x00000800
#define PCONP_PCADC		0x00001000
#define PCONP_PCAN1		0x00002000
#define PCONP_PCAN2		0x00004000
#define PCONP_PCAN3		0x00008000
#define PCONP_PCAN4		0x00010000
#define PCONP_PCI2C1	0x00080000
#define PCONP_PCSSP0	0x00200000
#define PCONP_PCTIM2	0x00400000
#define PCONP_PCTIM3	0x00800000
#define PCONP_PCUART2	0x01000000
#define PCONP_PCUART3	0x02000000
#define PCONP_PCI2C2	0x04000000
#define PCONP_PCI2CS	0x08000000
#define PCONP_PCSDC		0x10000000
#define PCONP_PCGPDMA	0x20000000
#define PCONP_PCENET	0x40000000
#define PCONP_PUSB		0x80000000

#define CCLKCFG_CCLKSEL_MASK 0x000000FF

typedef struct
{
	unsigned PCLK_WDT:2;
	unsigned PCLK_TIMER0:2;
	unsigned PCLK_TIMER1:2; 
	unsigned PCLK_UART0:2;

	unsigned PCLK_UART1:2;
	unsigned :2;
	unsigned PCLK_PWM1:2;
	unsigned PCLK_I2C0:2;

	unsigned PCLK_SPI:2;
	unsigned :2;
	unsigned PCLK_SSP1:2;
	unsigned PCLK_DAC:2;

	unsigned PCLK_ADC:2;
	unsigned PCLK_CAN1:2;
	unsigned PCLK_CAN2:2;
	unsigned PCLK_ACF:2;
} _PCLKSEL0;

#define PCLKSEL0bits (*(_PCLKSEL0 *) &LPC_SC->PCLKSEL0)

typedef struct
{
	unsigned PCLK_QEI:2;
	unsigned PCLK_GPIOINT:2;
	unsigned PCLK_PCB:2; 
	unsigned PCLK_I2C1:2;

	unsigned :2;
	unsigned PCLK_SSP0:2;
	unsigned PCLK_TIMER2:2;
	unsigned PCLK_TIMER3:2;

	unsigned PCLK_UART2:2;
	unsigned PCLK_UART3:2;
	unsigned PCLK_I2C2:2;
	unsigned PCLK_I2S:2;
	
	unsigned :2;
	unsigned PCLK_RIT:2;
	unsigned PCLK_SYSCON:2;
	unsigned PCLK_MC:2;
} _PCLKSEL1;

#define PCLKSEL1bits (*(_PCLKSEL1 *) &LPC_SC->PCLKSEL1)

// prototypes
///////////////////////////////

int cpu_cclk() __attribute__((deprecated)); 
int cpu_pclk(int cclk, int setting);
int cpu_trylock(unsigned char *lock, unsigned char value) __attribute__((always_inline));
void cpu_unlock(unsigned char *lock) __attribute__((always_inline));

#endif // LPC17_CPU_H
