// System Block Controller for TMS320DM36x
// by Miguel Fides

#include "system.h"

static SYSTEM_CONTROLLER *_system = (SYSTEM_CONTROLLER *)0x01C40000;

static PSC_CONTROLLER *_psc = (PSC_CONTROLLER *)0x01C41000;

// leopard board 365 uses 24MHz crystal on MXI1-MXO1 pins
static PLL_CONTROLLER *_pllc[] = { (PLL_CONTROLLER *)0x01C40800, (PLL_CONTROLLER *)0x01C40C00 };

void system_select_armss_clock(PLLC_INDEX plli)
{
	_system->PERI_CLKCTLbits.ARMCLKS = (int)plli;
}

void system_select_ddr2_clock(PLLC_INDEX plli)
{
	// select between PLLC1 (SYSCLK7) or PLLC2 (SYSCLK3)
	_system->PERI_CLKCTLbits.DDRCLKS = (int)plli;
}

void system_perform_vtpio_calibration()
{
	_system->VTPIOCR &= ~(VTPIO_CLRZ | VTPIO_LOCK | VTPIO_IOPWRDN | VTPIO_PWRDN);
	for(int volatile i = 0; i < 100; i++);
	_system->VTPIOCR |= VTPIO_CLRZ;
	while(_system->VTPIOCR & VTPIO_READY == 0);
	_system->VTPIOCR |= VTPIO_LOCK;
	_system->VTPIOCR |= (VTPIO_IOPWRDN | VTPIO_PWRDN);
}

void system_select_pinmux(int gio, int func)
{
	switch(gio)
	{
		case 1:		_system->PINMUX3bits.GIO1 = func; break;
		case 2:		_system->PINMUX3bits.GIO2 = func; break;
		case 3:		_system->PINMUX3bits.GIO3 = func; break;
		case 4:		_system->PINMUX3bits.GIO4 = func; break;
		case 5:		_system->PINMUX3bits.GIO5 = func; break;
		case 6:		_system->PINMUX3bits.GIO6 = func; break;
		case 7:		_system->PINMUX3bits.GIO7 = func; break;
		case 8:		_system->PINMUX3bits.GIO8 = func; break;
		case 9:		_system->PINMUX3bits.GIO9 = func; break;
		case 10:	_system->PINMUX3bits.GIO10 = func; break;
		case 11:	_system->PINMUX3bits.GIO11 = func; break;
		case 12:	_system->PINMUX3bits.GIO12 = func; break;
		case 13:	_system->PINMUX3bits.GIO13 = func; break;
		case 14:	_system->PINMUX3bits.GIO14 = func; break;
		case 15:	_system->PINMUX3bits.GIO15 = func; break;
		case 16:	_system->PINMUX3bits.GIO16 = func; break;
		case 17:	_system->PINMUX3bits.GIO17 = func; break;
		case 18:	_system->PINMUX3bits.GIO18 = func; break;
		case 19:	_system->PINMUX3bits.GIO19 = func; break;
		case 20:	_system->PINMUX3bits.GIO20 = func; break;
		case 21:	_system->PINMUX3bits.GIO21 = func; break;
		case 22:	_system->PINMUX3bits.GIO22 = func; break;
		case 23:	_system->PINMUX3bits.GIO23 = func; break;
		case 24:	_system->PINMUX3bits.GIO24 = func; break;
		case 25:	_system->PINMUX3bits.GIO25 = func; break;
		case 26:	_system->PINMUX3bits.GIO26 = func; break;
		case 27:	_system->PINMUX4bits.GIO27 = func;	break;
		case 28:	_system->PINMUX4bits.GIO28 = func;	break;
		case 29:	_system->PINMUX4bits.GIO29 = func;	break;
		case 30:	_system->PINMUX4bits.GIO30 = func;	break;
		case 31:	_system->PINMUX4bits.GIO31 = func;	break;
		case 32:	_system->PINMUX4bits.GIO32 = func;	break;
		case 33:	_system->PINMUX4bits.GIO33 = func;	break;
		case 34:	_system->PINMUX4bits.GIO34 = func;	break;
		case 35:	_system->PINMUX4bits.GIO35 = func;	break;
		case 36:	_system->PINMUX4bits.GIO36 = func;	break;
		case 37:	_system->PINMUX4bits.GIO37 = func;	break;
		case 38:	_system->PINMUX4bits.GIO38 = func;	break;
		case 39:	_system->PINMUX4bits.GIO39 = func;	break;
		case 40:	_system->PINMUX4bits.GIO40 = func;	break;
		case 41:	_system->PINMUX4bits.GIO41 = func;	break;
		case 42:	_system->PINMUX4bits.GIO42 = func;	break;
	}
}

void system_select_intmux(int number, int func)
{
	switch(number)
	{
		case 0:		_system->ARM_INTMUXbits.INT0 = func; break;
		case 7:		_system->ARM_INTMUXbits.INT7 = func; break;
		case 8:		_system->ARM_INTMUXbits.INT8 = func; break;
		case 10:	_system->ARM_INTMUXbits.INT10 = func; break;
		case 13:	_system->ARM_INTMUXbits.INT13 = func; break;
		case 17:	_system->ARM_INTMUXbits.INT17 = func; break;
		case 18:	_system->ARM_INTMUXbits.INT18 = func; break;
		case 19:	_system->ARM_INTMUXbits.INT19 = func; break;
		case 20:	_system->ARM_INTMUXbits.INT20 = func; break;
		case 24:	_system->ARM_INTMUXbits.INT24 = func; break;
		case 26:	_system->ARM_INTMUXbits.INT26 = func; break;
		case 28:	_system->ARM_INTMUXbits.INT28 = func; break;
		case 29:	_system->ARM_INTMUXbits.INT29 = func; break;
		case 30:	_system->ARM_INTMUXbits.INT30 = func; break;
		case 38:	_system->ARM_INTMUXbits.INT38 = func; break;
		case 43:	_system->ARM_INTMUXbits.INT43 = func; break;
		case 52:	_system->ARM_INTMUXbits.INT52 = func; break;
		case 53:	_system->ARM_INTMUXbits.INT53 = func; break;
		case 54:	_system->ARM_INTMUXbits.INT54 = func; break;
		case 55:	_system->ARM_INTMUXbits.INT55 = func; break;
		case 56:	_system->ARM_INTMUXbits.INT56 = func; break;
		case 57:	_system->ARM_INTMUXbits.INT57 = func; break;
		case 58:	_system->ARM_INTMUXbits.INT58 = func; break;
		case 59:	_system->ARM_INTMUXbits.INT59 = func; break;
		case 61:	_system->ARM_INTMUXbits.INT61 = func; break;
		case 62:	_system->ARM_INTMUXbits.INT62 = func; break;
	}
}


//The procedure for module state transitions is as follows (‘x’ corresponds to the module):
//• Wait for the GOSTATx bit in PTSTAT to clear to 0x0. You must wait for any previously initiated
//transitions to finish before initiating a new transition.
//• Set the NEXT bit in MDCTL[x] to SwResetDisable (0x0), SyncReset (0x1), Disable (0x2), or Enable
//(0x3).
//Note: You may set transitions in multiple NEXT bits in MDCTL[x] in this step.
//• Set the GOx bit in PTCMD to 0x1 to initiate the transition(s).
//• Wait for the GOSTATx bit in PTSTAT to clear to 0x0. The module is in the new state after the
//GOSTATx bit in PTSTAT clears to 0x0.

void psc_set_module_state(PSC_MODULE module, PSC_MODULE_STATE state)
{
	while(_psc->PTSTAT & PTSTAT_GOSTAT);
	_psc->MDCTL[module] = MDCTL_LRST | (state & 0x3);
	_psc->PTCMD |= PTCMD_GOSET;
	while(_psc->PTSTAT & PTSTAT_GOSTAT);
}

static void _wait(int count)
{
	for (int volatile i = 0; i < count; i++);
}

static void _set_div_ratio(PLL_CONTROLLER *pll, int index, int ratio)
{
	unsigned long value = PLLDIV_EN | ((ratio - 1) & PLLDIV_RATIO_MASK);
	switch(index)
	{
		case PLLC_SYSCLK1: pll->PLLDIV1 = value; break;
		case PLLC_SYSCLK2: pll->PLLDIV2 = value; break;
		case PLLC_SYSCLK3: pll->PLLDIV3 = value; break;
		case PLLC_SYSCLK4: pll->PLLDIV4 = value; break;
		case PLLC_SYSCLK5: pll->PLLDIV5 = value; break;
		case PLLC_SYSCLK6: pll->PLLDIV6 = value; break;
		case PLLC_SYSCLK7: pll->PLLDIV7 = value; break;
		case PLLC_SYSCLK8: pll->PLLDIV8 = value; break;
		case PLLC_SYSCLK9: pll->PLLDIV9 = value; break;
	}
}

void pllc_setup(PLLC_INDEX plli, int pre_div, int multiplier, int post_div, int sysclkdiv[], int sysclkdiv_count)
{
	PLL_CONTROLLER *pll = _pllc[plli];
	pll->PLLCTLbits.PLLPWRDN = 0;	// power up
	pll->PLLCTLbits.PLLENSRC = 0;	// enable PLLEN
	pll->PLLCTLbits.PLLEN = 0;	// bypass
	_wait(50);
	pll->PLLCTLbits.PLLRST = 1;	// assert reset
	_wait(50);
	pll->PLLCTLbits.PLLRST = 0;	// de-assert reset
	pll->PREDIV = /*PLLDIV_EN |*/ pre_div;
	pll->POSTDIV = PLLDIV_EN | post_div;
	pll->PLLM = multiplier;

	// The following sequence is required in PLLSECCTL for the multiplier and pre-divider values to take effect
	pll->PLLSECCTL |= (PLLSECCTL_TENABLE | PLLSECCTL_TENABLEDIV | PLLSECCTL_TINITZ);
	pll->PLLSECCTL &= ~(PLLSECCTL_TINITZ);
	pll->PLLSECCTL &= ~(PLLSECCTL_TENABLE | PLLSECCTL_TENABLEDIV);
	pll->PLLSECCTL |= PLLSECCTL_TINITZ;

	if (sysclkdiv != NULL)
	{
		for(int i = 0; i < sysclkdiv_count; i++)
		{
			_set_div_ratio(pll, i, sysclkdiv[i]);
		}
        	// NOTE: DM365 requires all clocks to remain synchronized
		pll->ALNCTL = 0x1FF;
		pll->PLLCMD |= PLLCMD_GOSET;
        while(pll->PLLSTAT & PLLSTAT_GOSTAT);	
	}

	// wait for pll to lock
	switch(plli)
	{
		case PLLC1:
			while((_system->PLLC1_CONFIG & PLLC_CONFIG_LOCK123_MASK) != (0x7 << PLLC_CONFIG_LOCK123_BIT));
			break;
		case PLLC2:
			while((_system->PLLC2_CONFIG & PLLC_CONFIG_LOCK123_MASK) != (0x7 << PLLC_CONFIG_LOCK123_BIT));
			break;
	}
	
	pll->PLLCTLbits.PLLEN = 1;
}

//To modify the PLLDIVn ratios, perform the following steps:
//1. Check that the GOSTAT bit in PLLSTAT is cleared to 0 to show that no GO operation is currently in
//progress.
//2. Program the RATIO field in PLLDIVn to the desired new divide-down rate. If the RATIO field changes,
//the PLL controller will flag the change in the corresponding bit of DCHANGE.
//3. Set the respective ALNn bits in ALNCTL to 1 to align any SYSCLKs after the GO operation.
//4. Set the GOSET bit in PLLCMD to 1 to initiate the GO operation to change the divide values, and to
//align the SYSCLKs as programmed.
//5. Read the GOSTAT bit in PLLSTAT to make sure the bit goes back to 0 to indicate that the GO
//operation has completed.

void pllc_set_divider(PLLC_INDEX plli, PLLC_SYSCLK_INDEX index, int ratio)
{
	PLL_CONTROLLER *pll = _pllc[plli];
	while(pll->PLLSTAT & PLLSTAT_GOSTAT);
	_set_div_ratio(pll, index, ratio);

	// NOTE: DM365 requires all clocks to remain synchronized
	pll->ALNCTL = 0x1FF;
	pll->PLLCMD |= PLLCMD_GOSET;
	while(pll->PLLSTAT & PLLSTAT_GOSTAT);	
}

int system_get_sysclk(PLLC_INDEX plli, PLLC_SYSCLK_INDEX sysi)
{
	int osc = OSCILLATOR_CLOCK_FREQUENCY;
	PLL_CONTROLLER *pll = _pllc[plli];
	int postdiv = pll->POSTDIV & 0x1F;
	int pllout = pll->PLLCTLbits.PLLEN ?
		((osc / (pll->PREDIV + 1)) * (pll->PLLM * 2) / (postdiv + 1)) : osc;
	int plldiv;
	switch(sysi)
	{
		case PLLC_SYSCLK1: plldiv = pll->PLLDIV1 & 0x1F; break;
		case PLLC_SYSCLK2: plldiv = pll->PLLDIV2 & 0x1F; break;
		case PLLC_SYSCLK3: plldiv = pll->PLLDIV3 & 0x1F; break;
		case PLLC_SYSCLK4: plldiv = pll->PLLDIV4 & 0x1F; break;
		case PLLC_SYSCLK5: plldiv = pll->PLLDIV5 & 0x1F; break;
		case PLLC_SYSCLK6: plldiv = pll->PLLDIV6 & 0x1F; break;
		case PLLC_SYSCLK7: plldiv = pll->PLLDIV7 & 0x1F; break;
		case PLLC_SYSCLK8: plldiv = pll->PLLDIV8 & 0x1F; break;
		case PLLC_SYSCLK9: plldiv = pll->PLLDIV9 & 0x1F; break;
		default: return osc;
	}
	return pllout / (plldiv + 1);
}




