
#include "vpss.h"
#include "system.h"

static int init = 0;

#define VPSS_VPBE_CLK_CTRL 0x01C70200

volatile unsigned long *_vpss_vpbe_clk_ctrl = (volatile unsigned long *)VPSS_VPBE_CLK_CTRL;

void vpss_init ( int hard)
{
	if ( !init || hard)
	{
		psc_set_module_state( PSC_MODULE_VPSS_MASTER, PSC_MODULE_ENABLE);

		system_video_regs ();	// VDAC & VPS_CLK_CTRL
	
		// Codigo xungo? VPSS_VPBE_CLK_CTRL = 0x00000011;   // Select enc_clk*1, turn on VPBE clk
		*_vpss_vpbe_clk_ctrl =  (1<<0) | // VPBE_CLK_ENABLE (OSD/VENC)
								(0<<2) | // VENC CLOCK /1 or /2
								(0<<3) | // LCD Clock enable
								(0<<6) | // OSD_CLK_SEL
								(0<<7);  // LDC_CLK_SEL
		init = 1;
	}

}



