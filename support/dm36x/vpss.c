#include "vpss.h"
#include "system.h"
#include <kernel/thread.h>

static char _init = 0;

typedef struct
{ 
	unsigned VPBE_CLK_CTRL;
	unsigned MISR_CTRL;
	unsigned MISR_OUT;
} VPSS_CONTROLLER;

static VPSS_CONTROLLER *_vpss = (VPSS_CONTROLLER *)0x01C70200;


void vpss_init(int hard)
{
	if (!_init || hard)
	{
#ifdef DEBUG
		PSC_MODULE_STATE state;
		state = psc_get_module_state(PSC_MODULE_VPSS_MASTER);
		state = psc_get_module_state(PSC_MODULE_VDAC_CLK);
#endif
		psc_set_module_state(PSC_MODULE_VPSS_MASTER, PSC_MODULE_ENABLE);
		psc_set_module_state(PSC_MODULE_VDAC_CLKREC, PSC_MODULE_ENABLE);
		psc_set_module_state(PSC_MODULE_VDAC_CLK, PSC_MODULE_ENABLE);

		_init = 1;
	}

}


