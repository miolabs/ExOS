#include "eth.h"
#include "cpu.h"
#include "gpio.h"
#include <kernel/driver/net/adapter.h>
#include <kernel/machine/hal.h>
#include <kernel/panic.h>

static bool _initialize(net_adapter_t *adapter, unsigned phy_unit, const phy_handler_t *handler);
static void _link_up(net_adapter_t *adapter);
static void _link_down(net_adapter_t *adapter);
static void *_get_input_buffer(net_adapter_t *adapter, unsigned long *plength);
static void _discard_input_buffer(net_adapter_t *adapter, void *buffer);
static void *_get_output_buffer(net_adapter_t *adapter, unsigned long size);
static int _send_output_buffer(net_adapter_t *adapter, net_mbuf_t *mbuf, NET_CALLBACK callback, void *state);
const net_driver_t __stm32_eth_driver = { .Initialize = _initialize,
	.LinkUp = _link_up, .LinkDown = _link_down,
	.GetInputBuffer = _get_input_buffer, .DiscardInputBuffer = _discard_input_buffer,
	.GetOutputBuffer = _get_output_buffer, .SendOutputBuffer = _send_output_buffer }; 

static void _phy_write(unsigned unit, phy_reg_t reg, unsigned short value);
static unsigned short _phy_read(unsigned unit, phy_reg_t reg);
static const phy_driver_t _phy_driver = { .Write = _phy_write, .Read = _phy_read };

static bool _initialize(net_adapter_t *adapter, unsigned phy_unit, const phy_handler_t *handler)
{
	ASSERT(SystemCoreClock >= 25000000UL, KERNEL_ERROR_KERNEL_PANIC);
	
	unsigned mdc_cr;
	if (SystemCoreClock <= 35000000UL) mdc_cr = 2;
	else if (SystemCoreClock <= 60000000UL) mdc_cr = 3;
	else if (SystemCoreClock <= 100000000UL) mdc_cr = 0;
	else if (SystemCoreClock <= 150000000UL) mdc_cr = 1;
	else mdc_cr = 4;

	RCC->AHB1RSTR |= RCC_AHB1RSTR_ETHMACRST;
	// NOTE: This configuration must be done while the MAC is under reset and before enabling the MAC clocks
	SYSCFG->PMC |= SYSCFG_PMC_MII_RMII_SEL;	// set RMII mode

	RCC->AHB1ENR |= RCC_AHB1ENR_ETHMACEN | RCC_AHB1ENR_ETHMACPTPEN;
	RCC->AHB1RSTR ^= RCC_AHB1RSTR_ETHMACRST;

	ETH->MACMIIAR = mdc_cr << ETH_MACMIIAR_CR_Pos;
	phy_create(&adapter->Phy, &_phy_driver, phy_unit, handler);

	return true;
}

static void _phy_write(unsigned unit, phy_reg_t reg, unsigned short value)
{
	ETH->MACMIIDR = value;
	ETH->MACMIIAR = (ETH->MACMIIAR & ETH_MACMIIAR_CR)
		| ((reg & 0x1f) << ETH_MACMIIAR_MR_Pos) | ((unit & 0x1f) << ETH_MACMIIAR_PA_Pos)
		| ETH_MACMIIAR_MW | ETH_MACMIIAR_MB;
	unsigned ar;
	for (unsigned i = 0; i < 1000; i++)
	{
		ar = ETH->MACMIIAR;
		if (!(ar & ETH_MACMIIAR_MB)) 
			break;
	}
	ASSERT(0 == (ar & ETH_MACMIIAR_MB), KERNEL_ERROR_KERNEL_PANIC); 
}

static unsigned short _phy_read(unsigned unit, phy_reg_t reg)
{
	ETH->MACMIIAR = (ETH->MACMIIAR & ETH_MACMIIAR_CR)
		| ((reg & 0x1f) << ETH_MACMIIAR_MR_Pos) | ((unit & 0x1f) << ETH_MACMIIAR_PA_Pos)
		| ETH_MACMIIAR_MB;
	unsigned ar;
	for (unsigned i = 0; i < 1000; i++)
	{
		ar = ETH->MACMIIAR;
		if (!(ar & ETH_MACMIIAR_MB)) 
			return (unsigned short)ETH->MACMIIDR;
	}
	kernel_panic(KERNEL_ERROR_KERNEL_PANIC); 
}

static void _link_up(net_adapter_t *adapter)
{
}

static void _link_down(net_adapter_t *adapter)
{
}

static void *_get_input_buffer(net_adapter_t *adapter, unsigned long *plength)
{
}

static void _discard_input_buffer(net_adapter_t *adapter, void *buffer)
{
}

static void *_get_output_buffer(net_adapter_t *adapter, unsigned long size)
{
}

static int _send_output_buffer(net_adapter_t *adapter, net_mbuf_t *mbuf, NET_CALLBACK callback, void *state)
{
}






