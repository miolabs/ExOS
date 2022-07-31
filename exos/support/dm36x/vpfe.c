// Video Processing Front End (VPFE) for TMS320DM36x

#include <assert.h>
#include <kernel/thread.h>

#include "vpfe.h"
#include "vpss.h"
#include "system.h"

static ISIF_CONTROLLER *_isif = (ISIF_CONTROLLER *)0x01C71000;
static IPIPE_CONTROLLER *_ipipe = (IPIPE_CONTROLLER *)0x01C70800;
static IPIPEIF_CONTROLLER *_ipieif = (IPIPEIF_CONTROLLER *)0x01C71200;
static RESIZER_CONTROLLER *_resizer = (RESIZER_CONTROLLER *)0x01C70400;
static H3A_CONTROLLER *_h3a = (H3A_CONTROLLER *)0x01C71400;

static void _vpfe_config_isif()
{
	_isif->MODESET = (0<<0) | // HDVDD
					 (0<<1) | // FIDD
					 (0<<2) | // VDPOL
					 (0<<3) | // HDPOL
					 (0<<4) | // FIPOL
					 (0<<5) | // SWEN 
					 (0<<7) | // CCDMD
					 (0<<12);  // INPMOD

//	_isif->CCDFG =	 (0<<0) | // VDLC
//					 (0<<0) | // EXTRG
//					 (0<<0); // YCINSWP
//
//	_isif->REC565IF = (1<<) | //R656ON
//
//	_isif->SYNCEN = ;
}


void vpfe_initialize_simple(VPFE_SIMPLE_SPEC *spec)
{
	// Reset VPFE
	vpss_init(0);

   	//int venc_clk = system_get_sysclk(PLLC2, PLLC_SYSCLK5);
	exos_thread_sleep(1);

	_vpfe_config_isif();

	//_vpfe_config_ipipeif();

	//_vpfe_config_ipipe();

	//_vpfe_config_resizer();
	//_vpfe_config_h3a();

}


#if 0





#endif