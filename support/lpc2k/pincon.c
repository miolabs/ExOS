// LPC17 GPIO Support
// by Miguel Fides

#include "pincon.h"

LPC_PINCON_TypeDef *LPC_PINCON = (LPC_PINCON_TypeDef *)0xE002C000;
LPC_GPIO_TypeDef *LPC_GPIO0 = (LPC_GPIO_TypeDef *)0x3FFFC000;
LPC_GPIO_TypeDef *LPC_GPIO1 = (LPC_GPIO_TypeDef *)0x3FFFC020;
LPC_GPIO_TypeDef *LPC_GPIO2 = (LPC_GPIO_TypeDef *)0x3FFFC040;
LPC_GPIO_TypeDef *LPC_GPIO3 = (LPC_GPIO_TypeDef *)0x3FFFC060;
LPC_GPIO_TypeDef *LPC_GPIO4 = (LPC_GPIO_TypeDef *)0x3FFFC080;

// currently nothing