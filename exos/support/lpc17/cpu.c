// LPC17xx sys/cpu support
// by Miguel Fides

#include "cpu.h"
#include <kernel/panic.h>

#if (__TARGET_PROCESSOR > 1770)

unsigned cpu_pclk(PCLK_PERIPH periph)
{
	// NOTE: periph is ignored
	int div = LPC_SC->PCLKSEL & 0x1F;
	return div == 0 ? 0 : SystemCoreClock / div; 
}

#else

unsigned cpu_pclk(PCLK_PERIPH periph)
{
	int div = (periph < 16) ?
		((LPC_SC->PCLKSEL0 >> (periph << 1)) & 3) :
		((LPC_SC->PCLKSEL1 >> ((periph - 16) << 1)) & 3); 

	switch(div)
	{
		case 0:
			return SystemCoreClock >> 2;
		case 1:
			return SystemCoreClock;
		case 2:
			return SystemCoreClock >> 1;
		case 3:
			return SystemCoreClock >> 3;
	}
}

static unsigned _pllclk0()
{
	unsigned osc;
	switch(LPC_SC->CLKSRCSEL & 0x3)
	{
		case 1:	osc = BOARD_OSC;	break;
		case 2:	osc = 32768;		break;
		default:
			osc = 4000000;	// irc clock
			break;
	}

	unsigned clk = osc;
	unsigned stat = LPC_SC->PLL0STAT;
	if ((stat & 0x3000000) == 0x3000000) // PLLE0_STAT & PLLC0_STAT
	{
		clk *= ((LPC_SC->PLL0STAT & 0x7FFF) + 1) << 1;
		clk /= (((LPC_SC->PLL0STAT >> 16) & 0xFF) + 1);
	}

	return clk / ((LPC_SC->CCLKCFG & 0xFF) + 1);
}

static unsigned _pllclk1()
{
	unsigned clk = BOARD_OSC;
	unsigned stat = LPC_SC->PLL1STAT;
	if ((stat & 0x300) == 0x300) // PLLE1_STAT & PLLC1_STAT
	{
		clk *= (LPC_SC->PLL1STAT & 0x1F) + 1;
		unsigned fcco = (clk << 1) * (((LPC_SC->PLL1STAT >> 5) & 0x3) + 1);	
		ASSERT(fcco >= 156000000UL && fcco <= 320000000UL, KERNEL_ERROR_KERNEL_PANIC);
	}
	return clk;
}

unsigned cpu_usbclk()
{
	unsigned usb_clk;
	if (LPC_SC->PLL1CON & 0x2)
	{
		usb_clk = _pllclk1();
	} 
	else
	{
		unsigned pll0clk = _pllclk0();
		switch(LPC_SC->USBCLKCFG & 0xf)
		{
			case 5:	usb_clk = pll0clk / 6;	break;
			case 7:	usb_clk = pll0clk / 8;	break;
			case 9:	usb_clk = pll0clk / 10;	break;
			default:
				kernel_panic(KERNEL_ERROR_KERNEL_PANIC);
		}
	}
	return usb_clk;
}

#endif



