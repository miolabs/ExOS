#ifndef LPC17_I2C_H
#define LPC17_I2C_H

typedef struct 
{
	volatile unsigned long CONSET;
	volatile unsigned long STAT;
	volatile unsigned long DAT;
	volatile unsigned long ADR;
	volatile unsigned long SCLH;
	volatile unsigned long SCLL;
	volatile unsigned long CONCLR;
} i2c_module_t;

#define I2C_CON_AA		0x04
#define I2C_CON_SI		0x08
#define I2C_CON_STOP	0x10
#define I2C_CON_START	0x20
#define I2C_CON_I2EN	0x40

#endif // LPC17_I2C_H

