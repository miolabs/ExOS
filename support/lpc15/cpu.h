#ifndef LPC15_CPU_H
#define LPC15_CPU_H

#include <CMSIS/LPC15xx.h>

#define CLKCTRL0_SYS (1<<0)
#define CLKCTRL0_ROM (1<<1)
#define CLKCTRL0_SRAM1 (1<<3)
#define CLKCTRL0_SRAM2 (1<<4)
#define CLKCTRL0_FLASH (1<<7)
#define CLKCTRL0_EEPROM (1<<9)
#define CLKCTRL0_MUX (1<<11)
#define CLKCTRL0_SWM (1<<12)
#define CLKCTRL0_IOCON (1<<13)
#define CLKCTRL0_GPIO0 (1<<14)
#define CLKCTRL0_GPIO1 (1<<15)
#define CLKCTRL0_GPIO2 (1<<16)
#define CLKCTRL0_PINT (1<<18)
#define CLKCTRL0_GINT (1<<19)
#define CLKCTRL0_DMA (1<<20)
#define CLKCTRL0_CRC (1<<21)
#define CLKCTRL0_WWDT (1<<22)
#define CLKCTRL0_RTC (1<<23)
#define CLKCTRL0_ADC0 (1<<27)
#define CLKCTRL0_ADC1 (1<<28)
#define CLKCTRL0_DAC (1<<29)
#define CLKCTRL0_ACPM (1<<30)

#define CLKCTRL1_MRT (1<<0)
#define CLKCTRL1_RIT (1<<1)
#define CLKCTRL1_SCT0 (1<<2)
#define CLKCTRL1_SCT1 (1<<3)
#define CLKCTRL1_SCT2 (1<<4)
#define CLKCTRL1_SCT3 (1<<5)
#define CLKCTRL1_SCTIPU (1<<6)
#define CLKCTRL1_CCAN (1<<7)
#define CLKCTRL1_SPI0 (1<<9)
#define CLKCTRL1_SPI1 (1<<10)
#define CLKCTRL1_I2C0 (1<<13)
#define CLKCTRL1_UART0 (1<<17)
#define CLKCTRL1_UART1 (1<<18)
#define CLKCTRL1_UART2 (1<<19)
#define CLKCTRL1_QEI (1<<21)
#define CLKCTRL1_USB (1<<23)

#endif // LPC15_CPU_H

