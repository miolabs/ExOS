// AEMIF Controller for TMS320DM36x
// by Miguel Fides

#include "aemif.h"
#include "system.h"

static AEMIF_CONTROLLER *_aemif = (AEMIF_CONTROLLER *)0x01D10000;
volatile unsigned char *__aemif_nand_data8 = (void *)0x2000000;
volatile unsigned char *__aemif_nand_cle8 = (void *)0x2000010;
volatile unsigned char *__aemif_nand_ale8 = (void *)0x200000B;
volatile unsigned long *__aemif_nand_data = (void *)0x2000000;
volatile unsigned long *__aemif_nand_cle = (void *)0x2000010;
volatile unsigned long *__aemif_nand_ale = (void *)0x200000B;

static inline int _max(int a, int b)
{
	return a >= b ? a : b;
}

static inline int _max4(int a, int b, int c, int d)
{
	return _max(_max(a, b), _max(c, d));
}

static inline int _rdiv(int a, int b)
{
	return a >= b ? (a + (b - 1)) / b : 1;
}

void aemif_initialize_nand(AEMIF_SPEC *spec, int cs, unsigned int tclk)
{
	psc_set_module_state(PSC_MODULE_AEMIF, PSC_MODULE_ENABLE);
	//NOTE: default pinmux is correct for aemif nand flash
	
	unsigned tsu = 5;	// ns data valid before EM_OE rising
	unsigned th = 0;	// ns data valid after EM_OE rising 
	
	unsigned r_strobe = _max(spec->trea_max + tsu, spec->trp);
	unsigned r_setup = _max(spec->tclr, (spec->tcea_max + tsu) - r_strobe); 
	unsigned r_hold = _max(th - spec->tchz_max, spec->trc - (r_setup + r_strobe));
	unsigned ta = _max(spec->tchz_max, spec->trhz_max - r_hold);

	unsigned w_strobe = spec->twp;
	unsigned w_setup = _max(0, spec->tds - w_strobe);
	unsigned w_hold = _max(spec->twc - (w_strobe + w_setup),
		_max4(spec->tclh, spec->talh, spec->tch, spec->tdh));

	unsigned long cr = CR_ASIZE_8BIT | CR_EW;
	cr |= (_rdiv(ta, tclk) - 1) << CR_TA_BIT;
	cr |= (_rdiv(r_hold, tclk) - 1) << CR_RHOLD_BIT;
	cr |= (_rdiv(r_strobe, tclk) - 1) << CR_RSTROBE_BIT;
	cr |= (_rdiv(r_setup, tclk) - 1) << CR_RSETUP_BIT;
	cr |= (_rdiv(w_hold, tclk) - 1) << CR_WHOLD_BIT;
	cr |= (_rdiv(w_strobe, tclk) - 1) << CR_WSTROBE_BIT;
	cr |= (_rdiv(w_setup, tclk) - 1) << CR_WSETUP_BIT;

	_aemif->AWCCR = 0x80;	// clear WP0 (wait polarity low)
	switch(cs)
	{
		case 0:	
			_aemif->A1CR = cr;
			_aemif->NANDFCR |= NANDFCR_CS2NAND;
			break;
		case 1: 
			_aemif->A2CR = cr;	
			_aemif->NANDFCR |= NANDFCR_CS3NAND;
			break;
	}


}
