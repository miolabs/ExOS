#ifndef LPC2K_I2C_H
#define LPC2K_I2C_H

typedef struct _I2C_MODULE
{
	volatile unsigned long I2CONSET;
	volatile unsigned long I2STAT;
	volatile unsigned long I2DAT;
	volatile unsigned long I2ADR;
	volatile unsigned long I2SCLH;
	volatile unsigned long I2SCLL;
	volatile unsigned long I2CONCLR;
} I2C_MODULE;

#define I2C_CON_AA		0x04
#define I2C_CON_SI		0x08
#define I2C_CON_STOP	0x10
#define I2C_CON_START	0x20
#define I2C_CON_I2EN	0x40

#endif // LPC2K_I2C_H

