#include <support/board_hal.h>
#include <support/dm36x/gpio.h>
#include <support/dm36x/system.h>
#include <support/dm36x/emif.h>
#include <support/dm36x/aemif.h>
#include <support/onfi.h>

extern SYSTEM_CONTROLLER *_system;

// Leopardboard 365 leds are at GIO58, GIO57, active high
#define LED1 58	// GIO58
#define LED2 57 // GIO57

static void _fl_cmd(unsigned char cmd, unsigned addr_len, unsigned char *addr);
static int _fl_write(void *data, unsigned length);
static int _fl_read(void *data, unsigned length);
static const ONFI_DRIVER _fl_driver = {
	.Cmd = _fl_cmd, .Write = _fl_write, .Read = _fl_read };

static ONFI_STATUS _fl_status;

volatile int i_am_here () { return 666; }

void hal_board_initialize()
{
	gpio_initialize();
   	gpio_setup(LED1, GPIO_DIR_OUTPUT, 0);
	gpio_setup(LED2, GPIO_DIR_OUTPUT, 0);
	system_select_armss_clock(PLLC1);

	// PLL freq = 432 MHz
	int ratios_pllc2[] = { 
		9,	// SYSCLK1 = 48 MHz (USB Ref)
		2,	// SYSCLK2 = 216 MHz (ARM9)
		3,	// SYSCLK3 = 144.33 MHz (-DDR2)
		27,	// SYSCLK4 = 16 MHz (VoiceCodec)
		16,	// SYSCLK5 = 27 MHz (VENC)
		}; 
//	pllc_setup(PLLC2, 0, 9, 0, ratios_pllc2, 5);
	pllc_setup(PLLC2, 0, 27, 2, ratios_pllc2, 5);

	system_select_armss_clock(PLLC2);
    system_select_ddr2_clock(PLLC2);
//	gpio_setup(35, GPIO_DIR_OUTPUT, 1);	// CLKOUT1
//	gpio_setup(31, GPIO_DIR_OUTPUT, 1);	// CLKOUT2 (PLLC1_SYSCLK9 * DIV)
//	system_select_pinmux(31, 3);	// for testing purposes only

		// PLL freq = 500 MHz
		int ratios_pllc1[] = {
			10,	// SYSCLK1 = 50 MHz (-USB Ref)
			3,	// SYSCLK2 = 166.66 MHz (HDVICP+ARM9Core)
			3,	// SYSCLK3 = 166.66 MHz (HDVICP IF+MJCP IF)
			6,	// SYSCLK4 = 83.33 MHz (CFGBuf, VCLK, Peripheral)
			3,	// SYSCLK5 = 166.66 MHz (VPSS)
			10, // SYSCLK6 = (-VENC)
			3,	// SYSCLK7 = 166.66 MHz (DDR2)
			20, // SYSCLK8 = 25MHz (MMC/SD0)
			10, // SYSCLK9 = (DIV * CLKOUT2)
			}; 
		pllc_setup(PLLC1, 5, 125, 1, ratios_pllc1, 9);
		system_select_ddr2_clock(PLLC1);

	// Initialize SDRAM if the program is not already working there
	unsigned int code = (unsigned int)&i_am_here;
	if ((code & 0xf0000000) != 0x80000000)
	{
		EMIF_SPEC spec = (EMIF_SPEC) {
			.tref = 7800, // 7.8 us)
			.trfc = 128, // 127.5 ns
			.trp = 20,
			.trcd = 20,
			.twr = 15,
			.tras = 45,
			.trc = 65,
			.trrd = 10,
			.twtr = 10,
			.trasmax = 70000, // 70us
			.txp = 2, // cycles
			.txsnr = 138,	// 137.5 ns
			.txsrd = 200, // cycles
			.trtp = 8,	// 7.5 ns
			.tcke = 3, // cycles
			};
		emif_initialize_ddr2(&spec, 6); // 6ns clk (166MHz)
	}

#ifdef DM36X_ENABLE_VENC
	_system->VDAC_CONFIG = 0x8001c; // 0x101941DC;
	_system->VPSS_CLK_CTRLbits.VENC_CLK_SRC = VENC_CLK_SRC_PLLC2SYSCLK5;
	_system->VPSS_CLK_CTRLbits.VPSS_MUXSEL = VPSS_MUXSEL_PLLC_CLK;
	_system->VPSS_CLK_CTRLbits.VENCLKEN = 1;
	_system->VPSS_CLK_CTRLbits.DACCLKEN = 1;
#endif	

	AEMIF_SPEC spec2 = (AEMIF_SPEC) {
		.trp = 20,
		.trea_max = 25, 
		.tcea_max = 25, .tchz_max = 30,
		.trc = 35,
		.trhz_max = 100,
		.twp = 15,
        .tcs = 0, .tch = 10, 
		.tcls = 10, .tclh = 5,
        .tals = 10, .talh = 5,
		.tds = 10, .tdh = 10,
		.twc = 25,
		};
	aemif_initialize_nand(&spec2, 0, 12); // 12ns clk (83 MHz)
	int fl_ok = onfi_initialize(&_fl_driver, &_fl_status);
}


void hal_led_set(HAL_LED led, int state)
{
	switch(led)
	{
		case 0:
			gpio_set(LED1, state);
			break;
		case 1:
			gpio_set(LED2, state);
			break;
	}
}

static void _fl_cmd(unsigned char cmd, unsigned addr_len, unsigned char *addr)
{
	AEMIF_NAND_CLE8 = cmd;
   	for(int i = 0; i < addr_len; i++)
		AEMIF_NAND_ALE8 = addr[i];
}

static int _fl_write(void *data, unsigned length)
{
	for(int i = 0; i < length; i++)
		AEMIF_NAND_DATA8 = ((unsigned char *)data)[i];
	return length;
}

static int _fl_read(void *data, unsigned length)
{
	if (data != NULL)
	{
		for(int i = 0; i < length;i ++)
			((unsigned char *)data)[i] = AEMIF_NAND_DATA8;
	}
	else
	{
		unsigned char dummy;
		for(int i = 0; i < length;i ++)
			dummy = AEMIF_NAND_DATA8;
	}
	return length;
}


