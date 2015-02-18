#ifndef LPC2K_PINCON_H
#define LPC2K_PINCON_H

#include <support/lpc2k/cpu.h>

typedef struct
{
  __IO unsigned long PINSEL0;
  __IO unsigned long PINSEL1;
  __IO unsigned long PINSEL2;
  __IO unsigned long PINSEL3;
  __IO unsigned long PINSEL4;
  __IO unsigned long PINSEL5;
  __IO unsigned long PINSEL6;
  __IO unsigned long PINSEL7;
  __IO unsigned long PINSEL8;
  __IO unsigned long PINSEL9;
  __IO unsigned long PINSEL10;
       unsigned long RESERVED0[5];
  __IO unsigned long PINMODE0;
  __IO unsigned long PINMODE1;
  __IO unsigned long PINMODE2;
  __IO unsigned long PINMODE3;
  __IO unsigned long PINMODE4;
  __IO unsigned long PINMODE5;
  __IO unsigned long PINMODE6;
  __IO unsigned long PINMODE7;
  __IO unsigned long PINMODE8;
  __IO unsigned long PINMODE9;
  __IO unsigned long PINMODE_OD0;
  __IO unsigned long PINMODE_OD1;
  __IO unsigned long PINMODE_OD2;
  __IO unsigned long PINMODE_OD3;
  __IO unsigned long PINMODE_OD4;
  __IO unsigned long I2CPADCFG;
} LPC_PINCON_TypeDef;

#define LPC_PINCON  ((LPC_PINCON_TypeDef *)0xE002C000)

typedef struct
{
    __IO unsigned long FIODIR;
	unsigned long RESERVED0[3];
    __IO unsigned long FIOMASK;
    __IO unsigned long FIOPIN;
    __IO unsigned long FIOSET;
    __O  unsigned long FIOCLR;
} LPC_GPIO_TypeDef;

#define LPC_GPIO0 ((LPC_GPIO_TypeDef *)0x3FFFC000)
#define LPC_GPIO1 ((LPC_GPIO_TypeDef *)0x3FFFC020)
#define LPC_GPIO2 ((LPC_GPIO_TypeDef *)0x3FFFC040)
#define LPC_GPIO3 ((LPC_GPIO_TypeDef *)0x3FFFC060)
#define LPC_GPIO4 ((LPC_GPIO_TypeDef *)0x3FFFC080)

#define PINMODE_PULLUP 0
#define PINMODE_REPEATER 1
#define PINMODE_FLOAT 2
#define PINMODE_PULLDOWN 3

// prototypes
void pincon_setfunc(int port, int pin, int func, int mode);

#endif // LPC2K_PINCON_H
