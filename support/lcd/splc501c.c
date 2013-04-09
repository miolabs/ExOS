#include "splc501c.h"
#include <kernel/thread.h>
#include <support/ssp_hal.h>

static void _write_cmd(unsigned char cmd);
static void _write_cmd2(unsigned char cmd1, unsigned char cmd2);
static void _write_data(unsigned char data[], int length);

void lcdcon_initialize(LCD_PROPERTIES *lcd)
{
	lcd->Buffer = (LCD_BUFFER_PROPERTIES) {
		.Depth = 1, .BufferType = LCD_BUFFER_STRIPE };
	lcd->Width = 128;
	lcd->Height = 64;

	hal_ssp_initialize(LCD_SSP_MODULE, 2000000, HAL_SSP_MODE_SPI, HAL_SSP_CLK_IDLE_HIGH);
	lcdcon_gpo_initialize();

	lcdcon_gpo(LCDCON_GPO_CS);
	lcdcon_gpo(LCDCON_GPO_CS | LCDCON_GPO_RESET);
	exos_thread_sleep(2);
	lcdcon_gpo(LCDCON_GPO_CS);
	exos_thread_sleep(20);

	_write_cmd(0xa2);	// set 1/9 bias
	_write_cmd(0xa0);	// adc select, normal
	_write_cmd(0xc8);	// com output reverse
	_write_cmd(0xa4);	// display all points normal
	_write_cmd(0x40);	// display start line set
	_write_cmd(0x25);	// internal resistor ratio
	_write_cmd(0x81);	// electronic volume mode set
	_write_cmd(0x13);	// electronic volume
	_write_cmd(0x2f);	// power controller set
	_write_cmd(0xaf);	// display on

	for(int page = 0; page < 8; page++)
	{
		_write_cmd(0xb0 | page); // select page
		_write_cmd2(0x10, 0x00);

		for(int i = 0; i < 48; i++)
		{
			unsigned char data[] = { 0x55, 0xAA, 0x55, 0xAA }; // {0xCC, 0xCC, 0x33, 0x33 };
			_write_data(data, sizeof(data));
		}
	}

	lcdcon_gpo(LCDCON_GPO_IDLE);
}

void splc501c_set_offset(int offset)
{
	_write_cmd(0x40 | (offset & 0x3F));
}

static void _write_cmd(unsigned char cmd)
{
	lcdcon_gpo(LCDCON_GPO_CS | LCDCON_GPO_A0);
	hal_ssp_transmit(LCD_SSP_MODULE, &cmd, 0, 1);
	lcdcon_gpo(LCDCON_GPO_IDLE);
}

static void _write_cmd2(unsigned char cmd1, unsigned char cmd2)
{
	unsigned char _buffer[] = { cmd1, cmd2 };
	lcdcon_gpo(LCDCON_GPO_CS | LCDCON_GPO_A0);
	hal_ssp_transmit(LCD_SSP_MODULE, _buffer, 0, 2);
	lcdcon_gpo(LCDCON_GPO_IDLE);
}

static void _write_data(unsigned char data[], int length)
{
	lcdcon_gpo(LCDCON_GPO_CS);
	hal_ssp_transmit(LCD_SSP_MODULE, data, 0, length);
	lcdcon_gpo(LCDCON_GPO_IDLE);
}
