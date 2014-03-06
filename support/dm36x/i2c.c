// DM36 I2C Peripheral Support

#include "cpu.h"
#include <support/i2c_hal.h>
#include <support/board_hal.h>

typedef volatile struct
{
	unsigned long ICOAR, // 0h I2C Own Address 
	unsigned long ICIMR, // 4h I2C Interrupt Mask 
	unsigned long ICSTR, // 8h I2C Interrupt Status 
	unsigned long ICCLKL, // Ch I2C Clock Low-Time Divider 
	unsigned long ICCLKH, // 10h I2C Clock High-Time Divider 
	unsigned long ICCNT, // 14h I2C Data Count 
	unsigned long ICDRR, // 18h I2C Data Receive 
	unsigned long ICSAR, // 1Ch I2C Slave Address
	unsigned long ICDXR, // 20h I2C Data Transmit
	unsigned long ICMDR, // 24h I2C Mode
	unsigned long ICIVR, // 28h I2C Interrupt Vector
	unsigned long ICEMDR, // 2Ch I2C Extended Mode
	unsigned long ICPSC, // 30h I2C Prescaler
	unsigned long REVID1, // 34h I2C Revision ID Register
	unsigned long REVID2, // 38h I2C Revision ID Register
	unsigned long reserved1, // 3Ch 
   	unsigned long reserved2, // 40h 
    unsigned long reserved3, // 44h 
	unsigned long ICPFUNC, // 48h I2C Pin Function
	unsigned long ICPDIR, // 4Ch I2C Pin Direction
	unsigned long ICPDIN, // 50h I2C Pin Data In
	unsigned long ICPDOUT, // 54h I2C Pin Data Out
	unsigned long ICPDSET, // 58h I2C Pin Data Set
	unsigned long ICPDCLR, // 5ch I2C Pin Data Clear register
} I2C_CONTROLLER;

static I2C_CONTROLLER *_i2c = (ISIF_CONTROLLER *)0x01C21000; 

void hal_i2c_initialize(int module, int bitrate)
{
	//Enable I2C clock from the Power and Sleep Controller if it is driven by Power and Sleep Controller (see the SoC Reference Guide ).
	psc_set_module_state(PSC_MODULE_I2C, PSC_MODULE_ENABLE)

	hal_board_init_pinmux(HAL_RESOURCE_I2C, module);

	//Place I2C in reset (clear IRS = 0 in ICMDR).
	_i2c->ICMDR &= ~(1<<5);

	//Configure ICMDR:
	//(a) Configure I2C as Master (MST = 1).
	//(b) Indicate the I2C configuration to be used; for example, Data Receiver (TRX = 0).
	//(c) Indicate 7-bit addressing is to be used (XA = 0).
	//(i) Disable repeat mode (RM = 0).
	//(d) Disable loopback mode (DLB = 0).
	//(i) Disable free data format (FDF = 0).
	//(e) Optional: Disable start byte mode if addressing a fully fledged I2C device (STB = 0).
	//(f) Set number of bits to transfer to be 8 bits (BC = 0).
	_i2c->ICMDR = (0<<0) |  // BC = 8 bits
				  (0<<3) |  // FDF = 0 
				  (0<<4) |	// STB = 0
				  (0<<5) |  // IRS = 0 
				  (0<<6) |  // DLB = 0 
				  (0<<7) |  // RM = 0 
				  (0<<8) |  // XA = 0 
				  (0<<9) |  // TRX = 0 Receiver mode 
				  (1<<10);	// MST = 1 Master

	//Configure Slave Address: the I2C device this I2C master would be addressing (ICSAR = 7BIT ADDRESS).
	_i2c->ICSAR = ;

	//Configure the peripheral clock operation frequency (ICPSC). This value should be selected in such a way that the frequency is between 7 and 12 MHZ.
	int pclk = cpu_pclk(SystemCoreClock, 1);
	int icpsc = (pclk / 12000000) - 1;
	_i2c->ICPSC = icpsc;	
	int d = 7;
	if (icpsc == 1)
		d = 6;
	else if (icpsc > 1)
		d = 5;
	int prescaler_clk =  pclk / (icpsc + 1);

	//Configure I2C master clock frequency:
	//(a) Configure the low-time divider value (ICCLKL). 
	//   Clock low-time divide-down value of 1-65536. The period of the module clock is 
	//   multiplied by (ICCL + d) to produce the low-time duration of the I2C serial on the (I2C_SCL) pin.
    int third_divider = prescaler_clk / (bitrate * 3);
	_i2c->ICCLKL = (third_divider * 2) - d;
	//(b) Configure the high-time divider value (ICCLKH).
   //     Clock high-time divide-down value of 1-65536. The period of the module clock is 
   //     multiplied by (ICCH + d) to produce the high-time duration of the I2C serial on the (I2C_SCL) pin.
	_i2c->ICCLKH = third_divider - d; // ICCLKH must be configured while the I2C is still in reset

	//Make sure the interrupt status register (ICSTR) is cleared:
	//(a) Read ICSTR and write it back (write 1 to clear) ICSTR = ICSTR. 
	//(b) Read ICIVR until it is zero.
	
	// Take I2 Ccontroller out of reset: enable I2C controller (set IRS bit = 1 in ICMDR).
	_i2c->ICMDR |= (1<<5);
}

static int _wait_not_busy()
{
	int result = 0;
	for(int loops = 0; loops < 1000; loops++)
	{
		// Check if bus busy bit is cleared (BB = 0 in ICSTR)
		if ((_i2c->ICSTR & (1<<12)) == 0) 
		{ 
			result = 1; 
			break;
		}
	}
	return result;
}

int hal_i2c_master_frame(int module, unsigned char slave, 
	unsigned char *buffer, int write_len, int read_len)
{
	if (module == 0)
		return 1;	// Error codes?

	// Servicing Receive Data via CPU
	// ------------------------------

	// Wait until busy bit is cleared 
	int error = 0;
	if (!_wait_not_busy()) 
		error = 1;

	// Generate a START event, followed by Slave Address, etc. (set STT = 1 in ICMDR)
	_i2c->ICMDR |= (1<<13);

	// Wait until data is received (ICRRDY = 1 in ICSTR)

	// Read data:
	// (a) If ICRRDY = 1 in ICSTR, then read ICDRR.
	// (b) Perform the previous two steps until receiving one byte short of the entire byte expecting to receive.

	// Configure the I2C controller not to generate an ACK on the next/final byte reception: set NACKMOD bit for the I2C to generate a NACK on the last byte received (set NACKMOD = 1 in ICMDR).

	// End transfer/release bus when transfer is done. Generate a STOP event (set STP = 1 in ICMDR).


	if (write_len != 0 && error == 0)
	{
	}

	if (read_len != 0 && error == 0)
	{
	}

}




/*
#define I2C_MODULE_COUNT 3

static I2C_MODULE *_modules[] = { 
	(I2C_MODULE *)LPC_I2C0_BASE, 
	(I2C_MODULE *)LPC_I2C1_BASE, 
	(I2C_MODULE *)LPC_I2C2_BASE };

static inline I2C_MODULE *_get_module(int module)
{
	return (module < I2C_MODULE_COUNT) ? _modules[module] : (void *)0;
}

void hal_i2c_initialize(int module, int bitrate)
{
	I2C_MODULE *i2c = _get_module(module);
	switch(module)
	{
		case 0:
			LPC_SC->PCONP |= PCONP_PCI2C0;
			break;
		case 1:
			LPC_SC->PCONP |= PCONP_PCI2C1;
			break;
		case 2:
			LPC_SC->PCONP |= PCONP_PCI2C2;
			break;
	}

	if (i2c)
	{
		hal_board_init_pinmux(HAL_RESOURCE_I2C, module);
		i2c->I2CONCLR = 0xFF;
		int pclk = cpu_pclk(SystemCoreClock, 1);
		int third_divider = pclk / (bitrate * 3);
		i2c->I2SCLH = third_divider;
		i2c->I2SCLL = third_divider * 2;
		i2c->I2CONSET = I2C_CON_I2EN;	// enable module
	}
}

static int _wait_si(I2C_MODULE *i2c)
{
	int result = 0;
	for(int loops = 0; loops < 1000; loops++)
	{
		if (i2c->I2CONSET & I2C_CON_SI) 
		{ 
			result = 1; 
			break;
		}
	}
	return result;
}

int hal_i2c_master_frame(int module, unsigned char slave, 
	unsigned char *buffer, int write_len, int read_len)
{
	I2C_MODULE *i2c = _get_module(module);
	if (i2c == (I2C_MODULE *)0) return -1;

	i2c->I2CONCLR = I2C_CON_SI;	
	int error = 0;
	int pos = 0;

	// send Start condition
	i2c->I2CONSET = I2C_CON_START;
	if (!_wait_si(i2c)) error = 1;
	else if ((i2c->I2STAT != 0x08) && (i2c->I2STAT != 0x10)) error = 2;

	if (write_len != 0 && error == 0)
	{
		i2c->I2DAT = slave << 1;	// SLA + W (low bit = 0)
		i2c->I2CONSET = I2C_CON_AA;
		i2c->I2CONCLR = I2C_CON_START | I2C_CON_SI;
		if (!_wait_si(i2c)) error = 1;
		else if (i2c->I2STAT != 0x18) error = 3;
		
		while(write_len-- && error == 0)
		{
			i2c->I2DAT = buffer[pos++];
            i2c->I2CONCLR = I2C_CON_SI;	
			if (!_wait_si(i2c)) error = 1;
			else if (i2c->I2STAT != 0x28) error = 4;
		}

		if (read_len != 0 && error == 0)
		{
			// send Repeated Start to switch to continue
			i2c->I2CONSET = I2C_CON_START;
            i2c->I2CONCLR = I2C_CON_SI;	
			if (!_wait_si(i2c)) error = 1;
			else if (i2c->I2STAT != 0x10) error = 5;
		}
	}

	if (read_len != 0 && error == 0)
	{
		i2c->I2DAT = (slave << 1) | 1 ;	// SLA + R (low bit = 1)
		i2c->I2CONSET = I2C_CON_AA;
		i2c->I2CONCLR = I2C_CON_START | I2C_CON_SI;
		if (!_wait_si(i2c)) error = 1;
		else if (i2c->I2STAT != 0x40) error = 6;

		while (read_len-- && error == 0)
		{
			if (read_len != 0)
			{
				i2c->I2CONCLR = I2C_CON_SI;
				if (!_wait_si(i2c)) error = 1;
				else if (i2c->I2STAT != 0x50) error = 7;
				else buffer[pos++] = i2c->I2DAT;
			}
			else
			{
				i2c->I2CONCLR = I2C_CON_SI | I2C_CON_AA;
				if (!_wait_si(i2c)) error = 1;
				else if (i2c->I2STAT != 0x58) error = 8;
				else buffer[pos++] = i2c->I2DAT;
			}
		}
	}

	if (error) 
	{
		i2c->I2CONCLR = I2C_CON_START;
	}
	i2c->I2CONSET = I2C_CON_STOP;
    i2c->I2CONCLR = I2C_CON_SI;
	return error;
}
*/

