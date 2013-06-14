// VPBE Des-Controller for TMS320DM36x
// by anonymous

#include "vpbe.h"
#include "vpss.h"
#include "system.h"

static VENC_CONTROLLER *_venc = (VENC_CONTROLLER *)0x01C71E00;
static OSD_CONTROLLER  *_osd  = (OSD_CONTROLLER *) 0x01C71C00;

void vpbe_initialize_simple  (VPBE_SIMPLE_SPEC *spec)
{
	// Reset VPBE
	vpss_init (0);

	// Configure OSD

	// OSD enable

    // Configure VENC

	// VENC enable

}

