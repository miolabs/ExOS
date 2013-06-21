
#include "vpss.h"
#include "system.h"

static int init = 0;

#define VPSS_CLK_CTRL  0x01C40044

#define VPSS_MUXSEL_VENC   0
#define VPSS_MUXSEL_EXCTL  2
#define VPSS_MUXSEL_PCLK   3

#define VENC_CLK_SRC_PLLC1SYSCLK6 0
#define VENC_CLK_SRC_PLLC2SYSCLK5 1
#define VENC_CLK_SRC_MXI          2


volatile unsigned long *_vpss_clk_ctrl = (volatile unsigned long *)VPSS_CLK_CTRL;

void vpss_init ( int hard)
{
	if ( !init || hard)
	{
		psc_set_module_state( PSC_MODULE_VPSS_MASTER, PSC_MODULE_ENABLE);

		*_vpss_clk_ctrl = (VPSS_MUXSEL_VENC<<0) | // VPSS clock
						  (0<<2) | // Invert VPFE pixel clock: normal or inverted
						  (0<<3) | // VPBE/Video encoder clock enable
						  (0<<4) | // DAC clock enable
						  (VENC_CLK_SRC_PLLC1SYSCLK6<<5) | // 27/74.25 MHz input source
						  (0<<7); // DMA clock vs. VPSS clock ratio: 1:2 or 1:1

		init = 1;
	}

}


int vpss_enable_clock ( unsigned long mask)
{
	unsigned long flags;
	unsigned long utemp;

	//spin_lock_irqsave(&oper_cfg.vpss_lock, flags);

	*_vpss_clk_ctrl = *_vpss_clk_ctrl |= mask;

	//spin_unlock_irqrestore(&oper_cfg.vpss_lock, flags);
}
