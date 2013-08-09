
#include <kernel/thread.h>
#include "vpss.h"
#include "system.h"

static char init = 0;

typedef struct
{ 
	unsigned VPBE_CLK_CTRL;
	unsigned MISR_CTRL;
	unsigned MISR_OUT;
} VPSS_CONTROLLER;

static VPSS_CONTROLLER *_vpss = (VPSS_CONTROLLER *)0x01C70200;


void vpss_init ( int hard)
{
	if ( !init || hard)
	{
		psc_set_module_state( PSC_MODULE_VPSS_MASTER, PSC_MODULE_ENABLE);
		psc_set_module_state( PSC_MODULE_VDAC_CLKREC, PSC_MODULE_ENABLE);
		psc_set_module_state( PSC_MODULE_VDAC_CLK, PSC_MODULE_ENABLE);

		/*psc_set_module_state( PSC_MODULE_EDMA_CC, PSC_MODULE_ENABLE);
		psc_set_module_state( PSC_MODULE_EDMA_TC0, PSC_MODULE_ENABLE);
		psc_set_module_state( PSC_MODULE_EDMA_TC1, PSC_MODULE_ENABLE);
		psc_set_module_state( PSC_MODULE_EDMA_TC2, PSC_MODULE_ENABLE);
		psc_set_module_state( PSC_MODULE_EDMA_TC3, PSC_MODULE_ENABLE);
		psc_set_module_state( PSC_MODULE_PWM0, PSC_MODULE_ENABLE);
		psc_set_module_state( PSC_MODULE_PWM1, PSC_MODULE_ENABLE);
		psc_set_module_state( PSC_MODULE_PWM2, PSC_MODULE_ENABLE);
		psc_set_module_state( PSC_MODULE_PWM3, PSC_MODULE_ENABLE);*/

		// VDAC & VPSS_CLK_CTRL
		system_video_regs (
			// Video DAC config (encontrado 0x101941DC, recomendado 0x081141CC para SD)
			0x081141CC, /*((0<<0) | // Power down channel A (down/normal)
						(0<<1) | // Power down channel B (down/normal)
						(1<<2) | // Power down channel C (down/normal)
						(1<<3) | // Power down SD video buffer (down/normal)
						(0<<4) | // TVINT circuit enable signal
						(0<<5) | // Select HD DAC mode / SD Video Buffer mode for DAC CH-C (SD/HD)
						(0x907<<6) | // reserved 
						(0<<19) | // Output interrupt signal when TVOUT shorts to ground
						(0x101<<20), // Reserved */
			// VPSS_CLKCTL
			(VPSS_MUXSEL_VENC<<0) | // VPSS clock
			(0<<2) | // Invert VPFE pixel clock: normal or inverted
			(1<<3) | // VPBE/Video encoder clock enable
			(1<<4) | // DAC clock enable
			(VENC_CLK_SRC_PLLC2SYSCLK5<<5) | // 27/74.25 MHz input source
			(0<<7)); // DMA clock vs. VPSS clock ratio: 1:2 or 1:1

		// Codigo xungo? VPSS_VPBE_CLK_CTRL = 0x00000011;   // Select enc_clk*1, turn on VPBE clk
		//   (LDC is some capture device)
сс
		_vpss->VPBE_CLK_CTRL =  (1<<0) |  // OSD,VENC enable
								(0<<2) |  // VENC CLOCK /1 or /2
								(0<<3) |  // LDC clock enable
								(0<<6) |  // OSD Clock sel: OSD or ARM
								(0<<7);   // LDC clock sel: OSD or ARM

		exos_thread_sleep ( 1);

		init = 1;
	}

}


