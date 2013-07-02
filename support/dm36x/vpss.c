
#include "vpss.h"
#include "system.h"

static int init = 0;

void vpss_init ( int hard)
{
	if ( !init || hard)
	{
		psc_set_module_state( PSC_MODULE_VPSS_MASTER, PSC_MODULE_ENABLE);

		system_video_regs ();	// VDAC & VPS_CLK_CTRL

		init = 1;
	}

}


