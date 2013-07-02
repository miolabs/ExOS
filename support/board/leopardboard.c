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

	// PLL freq = 432 MHz
	int ratios_pllc2[] = { 
		9,	// SYSCLK1 = 48 MHz (USB Ref)
		2,	// SYSCLK2 = 216 MHz (ARM9)
		18,	// SYSCLK3 = 24 MHz (-DDR2)
		27,	// SYSCLK4 = 16 MHz (VoiceCodec)
		16,	// SYSCLK5 = 27 MHz (VENC)
		}; 
	pllc_setup(PLLC2, 0, 9, 0, ratios_pllc2, 5);

	system_select_armss_clock(PLLC2);
//	gpio_setup(35, GPIO_DIR_OUTPUT, 1);	// CLKOUT1
//	gpio_setup(31, GPIO_DIR_OUTPUT, 1);	// CLKOUT2 (PLLC1_SYSCLK9 * DIV)
//	system_select_pinmux(31, 3);	// for testing purposes only

	// Initialize SDRAM if the program is not already working there
	unsigned int code = (unsigned int)&i_am_here;
	if (( code & 0xf0000000) != 0x80000000)
	{
		// PLL freq = 500 MHz
		int ratios_pllc1[] = {
			10,	// SYSCLK1 = 50 MHz (-USB Ref)
			3,	// SYSCLK2 = 166.66 MHz (HDVICP+ARM9Core)
			3,	// SYSCLK3 = 166.66 MHz (HDVICP IF+MJCP IF)
			6,	// SYSCLK4 = 83.33 MHz (CFGBuf, VCLK, Peripheral)
			4,	// SYSCLK5 = 125 MHz (VPSS)
			10, // SYSCLK6 = (-VENC)
			3,	// SYSCLK7 = 166.66 MHz (DDR2)
			20, // SYSCLK8 = 25MHz (MMC/SD0)
			10, // SYSCLK9 = (DIV * CLKOUT2)
			}; 
		pllc_setup(PLLC1, 5, 125, 1, ratios_pllc1, 9);

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
		system_select_ddr2_clock(PLLC1);
		emif_initialize_ddr2(&spec, 6); // 6ns clk (166MHz)
	}

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

int _setup_tvout ()
{
	//PINMUX1 = 0x00145555;  //  evmdm365.gel Video Cout, EXTCLK, FIELD
	// 0001 0100 0101 0101 0101

	//PINMUX1 &= ~0x00400000;  // Enable VCLK for PINMUX 
	//PINMUX1 |= 0XAA00; // Enable COUT[0],COUT[1],COUT[2],COUT[3]

    _system->PINMUX1bits.COUT0 = 1; // Enable COUT instead of GIO
    _system->PINMUX1bits.COUT1 = 1; // Enable COUT instead of GIO
    _system->PINMUX1bits.COUT2 = 1; // Enable COUT instead of GIO
    _system->PINMUX1bits.COUT3 = 1; // Enable COUT instead of GIO
    _system->PINMUX1bits.COUT4 = 1; // Enable COUT instead of GIO
    _system->PINMUX1bits.COUT5 = 1; // Enable COUT instead of GIO
    _system->PINMUX1bits.COUT6 = 1; // Enable COUT instead of GIO
    _system->PINMUX1bits.COUT7 = 1; // Enable COUT instead of GIO
    _system->PINMUX1bits.FIELD = 1; // ?
    _system->PINMUX1bits.VCLK = 0; // Enable VCLK for PINMUX 
	return 1;
}



int hal_board_init_pinmux(HAL_RESOURCE res, int unit)
{
	switch(res)
	{
		case HAL_RESOURCE_TVOUT: return _setup_tvout();
		/*case HAL_RESOURCE_I2C:		return _setup_i2c(unit);
		case HAL_RESOURCE_SSP:		return _setup_ssp(unit);
		case HAL_RESOURCE_USBHOST:	return _setup_usbhost(unit);
        case HAL_RESOURCE_USBDEV:	return _setup_usbdev(unit);
		case HAL_RESOURCE_PWM:		return _setup_pwm(unit);
		case HAL_RESOURCE_CAP:		return _setup_cap(unit);
		case HAL_RESOURCE_MAT:		return _setup_mat(unit);
		case HAL_RESOURCE_CAN:		return _setup_can(unit);
		case HAL_RESOURCE_UART:		return _setup_uart(unit);
		case HAL_RESOURCE_ADC:		return _setup_adc(unit);*/
	}
	return 0;
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


