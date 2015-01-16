// Quadrature Encoder module support for LPC15xx
// by Miguel Fides

#include "qei.h"

static QEI_INIT_FLAGS _flags = QEI_INITF_NONE;

void qei_initialize(int ppr, QEI_INIT_FLAGS flags)
{
	LPC_SYSCON->PRESETCTRL1 |= PRESETCTRL1_QEI;
	LPC_SYSCON->SYSAHBCLKCTRL1 |= CLKCTRL1_QEI;
	LPC_SYSCON->PRESETCTRL1 &= ~PRESETCTRL1_QEI;

	unsigned long base_freq = SystemCoreClock;

	LPC_QEI->CONF = 0;
	LPC_QEI->FILTERPHA = LPC_QEI->FILTERPHB = LPC_QEI->FILTERINX = 3;

	LPC_QEI->IEC = 0xFFFF;	// all ints
	LPC_QEI->CLR = 0xFFFF;	// all ints

	LPC_QEI->MAXPOS = ppr -1;
	LPC_QEI->CMPOS0 = 0;
	LPC_QEI->CMPOS1 = ppr -1;
	LPC_QEI->CMPOS2 = ppr;
	LPC_QEI->CONF = 
		((flags & QEI_INITF_HIGH_RES) ? QEI_CONF_CAPMODE : 0) |
		((flags & QEI_INITF_BACKWARDS) ? QEI_CONF_DIRINV : 0);
	LPC_QEI->CON = QEI_CON_RESP | 
		((flags & QEI_INITF_USE_INDEX) ? QEI_CON_RESPI : 0) | 
		QEI_CON_RESV | QEI_CON_RESI;
	_flags = flags;

#ifdef DEBUG
	NVIC_EnableIRQ(QEI_IRQn);
	LPC_QEI->IES = QEI_INTF_INX;
#endif
}

unsigned int qei_read()
{
	return LPC_QEI->POS;
}

void QEI_IRQHandler()
{
	if (_flags & QEI_INITF_USE_INDEX) LPC_QEI->CON = QEI_CON_RESPI;
	LPC_QEI->CLR = QEI_INTF_INX | QEI_INTF_POS0 | QEI_INTF_POS1 | QEI_INTF_POS2;
}


