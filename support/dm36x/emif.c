// DDR2 EMIF Controller for TMS320DM36x
// by Miguel Fides

#include "emif.h"
#include "system.h"

static EMIF_CONTROLLER *_emif = (EMIF_CONTROLLER *)0x20000000;

static inline int _max(int a, int b)
{
	return a >= b ? a : b;
}

static inline int _rdiv(int a, int b)
{
	return (a + (b - 1)) / b;
}


void emif_initialize_ddr2(EMIF_SPEC *spec, unsigned int tclk)
{
	psc_set_module_state(PSC_MODULE_DDR_EMIF, PSC_MODULE_ENABLE);
	// VTP initialization
	system_perform_vtpio_calibration();

	// configure DDR PHY
	_emif->DDRPHYCR1 = DDRPHYCR1_CONFIG_EXT_STRBEN // select external DQS strobe gating
		| DDRPHYCR1_CONFIG_PWRDNEN | 4; // read_latency (CL + board round trip) - 1

	// configure PBBPR (section 4.7)
	_emif->PBBPR = 0x7f; // must be different to default value (0xff)

	// register initialization (section 2.14.1)
	_emif->SDCR = SDCR_BOOTUNLOCK;
	_emif->SDCR = SDCR_TIMUNLOCK | 
		(4 << SDCR_CL_BIT) | 
		(SDCR_IBANK_EIGHT << SDCR_IBANK_BIT) | 
		(SDCR_PAGESIZE_1024 << SDCR_PAGESIZE_BIT) |
		SDCR_NM | SDCR_DDR_DDQS | SDCR_DDR2EN | SDCR_DDREN | SDCR_SDRAMEN;
	_emif->SDTIMR = ((_rdiv(spec->trfc, tclk) - 1) << SDTIMR_T_RFC_BIT) |
		((_rdiv(spec->trp, tclk) - 1) << SDTIMR_T_RP_BIT) |
		((_rdiv(spec->trcd, tclk) - 1) << SDTIMR_T_RCD_BIT) |
		((_rdiv(spec->twr, tclk) - 1) << SDTIMR_T_WR_BIT) |
		((_rdiv(spec->tras, tclk) - 1) << SDTIMR_T_RAS_BIT) |
		((_rdiv(spec->trc, tclk) - 1) << SDTIMR_T_RC_BIT) |
		(((((spec->trrd << 2) + (tclk << 1)) / (tclk << 2)) - 1) << SDTIMR_T_RRD_BIT) |
		((_rdiv(spec->twtr, tclk) - 1) << SDTIMR_T_WTR_BIT);
	_emif->SDTIMR2 = (((((spec->trasmax * spec->trfc) + 500000) / 1000000) -1) << SDTIMR2_T_RASMAX_BIT) |
		((_max(spec->txp, spec->tcke) - 1) << SDTIMR2_T_XP_BIT) |
		((_rdiv(spec->txsnr, tclk) - 1) << SDTIMR2_T_XSNR_BIT) |
		((spec->txsrd - 1) << SDTIMR2_T_XSRD_BIT) |
		((_rdiv(spec->trtp, tclk) - 1) << SDTIMR2_T_RTP_BIT) |
		((spec->tcke - 1) << SDTIMR2_T_CKE_BIT);
	_emif->SDCR &= ~SDCR_TIMUNLOCK;
	_emif->SDRCR = _rdiv(spec->tref, tclk);

	// reset and re-enable clocks
	psc_set_module_state(PSC_MODULE_DDR_EMIF, PSC_MODULE_SYNC_RESET);
	psc_set_module_state(PSC_MODULE_DDR_EMIF, PSC_MODULE_ENABLE);
}
