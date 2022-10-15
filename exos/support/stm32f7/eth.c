#include "eth.h"
#include "cpu.h"
#include "gpio.h"
#include <kernel/driver/net/adapter.h>
#include <kernel/machine/hal.h>
#include <kernel/panic.h>
#include <kernel/verbose.h>

#define _verbose(level, ...) verbose(level, "eth", __VA_ARGS__)

#ifndef TX_BUFFERS
#define TX_BUFFERS 32
#endif

#ifndef RX_BUFFERS
#define RX_BUFFERS 4
#endif

static struct _rx_buffer { unsigned char data[ETH_MAX_FRAME_SIZE]; } _rx_buffers[RX_BUFFERS];
static rx_edesc_t _rx_desc[RX_BUFFERS] __aligned(32);
static tx_edesc_t _tx_desc[TX_BUFFERS] __aligned(32);

static rx_edesc_t * volatile _rx_desc_ptr1;
static rx_edesc_t * volatile _rx_desc_ptr2;

static bool _initialize(net_adapter_t *adapter, unsigned phy_unit, const phy_handler_t *handler);
static void _link_up(net_adapter_t *adapter);
static void _link_down(net_adapter_t *adapter);
static void *_get_input_buffer(net_adapter_t *adapter, unsigned *plength);
static void _discard_input_buffer(net_adapter_t *adapter, void *buffer);
static void *_get_output_buffer(net_adapter_t *adapter, unsigned size);
static int _send_output_buffer(net_adapter_t *adapter, net_mbuf_t *mbuf, NET_CALLBACK callback, void *state);
const net_driver_t __stm32_eth_driver = { .Initialize = _initialize,
	.LinkUp = _link_up, .LinkDown = _link_down,
	.GetInputBuffer = _get_input_buffer, .DiscardInputBuffer = _discard_input_buffer,
	.GetOutputBuffer = _get_output_buffer, .SendOutputBuffer = _send_output_buffer }; 

static void _phy_write(unsigned unit, phy_reg_t reg, unsigned short value);
static unsigned short _phy_read(unsigned unit, phy_reg_t reg);
static const phy_driver_t _phy_driver = { .Write = _phy_write, .Read = _phy_read };

static net_adapter_t *_adapter = NULL;

static bool _initialize(net_adapter_t *adapter, unsigned phy_unit, const phy_handler_t *handler)
{
	ASSERT(SystemCoreClock >= 25000000UL, KERNEL_ERROR_KERNEL_PANIC);
	RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;

	ASSERT(_adapter == NULL, KERNEL_ERROR_KERNEL_PANIC);
	_adapter = adapter;
		
	unsigned mdc_cr;
	if (SystemCoreClock <= 35000000UL) mdc_cr = 2;
	else if (SystemCoreClock <= 60000000UL) mdc_cr = 3;
	else if (SystemCoreClock <= 100000000UL) mdc_cr = 0;
	else if (SystemCoreClock <= 150000000UL) mdc_cr = 1;
	else mdc_cr = 4;

	RCC->AHB1RSTR |= RCC_AHB1RSTR_ETHMACRST;
	// NOTE: This configuration must be done while the MAC is under reset and before enabling the MAC clocks
	SYSCFG->PMC |= SYSCFG_PMC_MII_RMII_SEL;	// set RMII mode

	RCC->AHB1ENR |= RCC_AHB1ENR_ETHMACEN | RCC_AHB1ENR_ETHMACRXEN | RCC_AHB1ENR_ETHMACTXEN /*| RCC_AHB1ENR_ETHMACPTPEN*/;
	RCC->AHB1RSTR ^= RCC_AHB1RSTR_ETHMACRST;

	ETH->MACMIIAR = mdc_cr << ETH_MACMIIAR_CR_Pos;
	phy_create(&adapter->Phy, &_phy_driver, phy_unit, handler);

	for(unsigned i = 0; i < TX_BUFFERS; i++)
	{
		unsigned nexti = (i != TX_BUFFERS - 1) ? i + 1 : 0;
		tx_edesc_t *next = &_tx_desc[nexti];
		_tx_desc[i] = (tx_edesc_t) { .tdes[3] = (unsigned)next };
	}
	
	for(unsigned i = 0; i < RX_BUFFERS; i++)
	{
		unsigned nexti = (i != RX_BUFFERS - 1) ? i + 1 : 0;
		rx_edesc_t *next = &_rx_desc[nexti];
		_rx_desc[i] = (rx_edesc_t) { .rdes[0] = RDES0_OWN,
			.rdes[1] = RDES1_RCH | ETH_MAX_FRAME_SIZE,
			.rdes[2] = (unsigned)&_rx_buffers[i],
			.rdes[3] = (unsigned)next };
	}
	_rx_desc_ptr1 = _rx_desc_ptr2 = &_rx_desc[0];

	ETH->DMABMR = 0		// TTC <- TxFIFO mode = Store-and-forward
		| ETH_DMABMR_FB	// FB Fixed Burst
		| ETH_DMABMR_PBL_4Beat	// Programmable Burst Length
		| ETH_DMABMR_EDE		// Enhanced Descriptor format Enable
		| (0 << ETH_DMABMR_DSL_Pos)	// Descriptor Skip Length
		| ETH_DMABMR_DA			// DMA Arbitration (Rx has priority over Tx)
		;

	ETH->DMAOMR = 0
		| ETH_DMAOMR_DTCEFD	// DTCEFD Do not drop TCP Checksum Error Frames <- FIXME: Remove 
		| ETH_DMAOMR_TSF	// TSF Transmit Store-and-forward
		; 
	// You must make sure the Transmit FIFO is deep enough to store a complete frame before that frame is transferred 
	// to the MAC Core transmitter. If the FIFO depth is less than the input Ethernet frame size, the payload 
	// (TCP/UDP/ICMP) checksum insertion function is bypassed and only the frame’s IPv4 Header checksum is modified, 
	// even in Store-and-forward mode.

	ETH->DMATDLAR = (unsigned)_tx_desc;	// address of first transmit descriptor
	ETH->DMARDLAR = (unsigned)_rx_desc;	// address of first receive descriptor
	ETH->DMAOMR |= ETH_DMAOMR_ST | ETH_DMAOMR_SR;	// Start Tx / Start Rx
	ETH->DMAIER = ETH_DMAIER_RIE | ETH_DMAIER_NISE	// Normal Int Summary Enable
		| ETH_DMAIER_AISE;	 // TODO: Abnormal Int Summary Enable

	ETH->MACFCR;	// BPA <- Back Pressure Active?
	ETH->MACFFR = ETH_MACFFR_RA 	// Receive All
		| ETH_MACFFR_PM;
	ETH->MACCR = 0		// IFG <- interframe gap?
		| ETH_MACCR_FES	// Fast Ethernet Speed (phy negotiated)
		| ETH_MACCR_DM	// full Duplex Mode enable (phy negotiated)
		| ETH_MACCR_RE;	// Rx Enable

	NVIC_EnableIRQ(ETH_IRQn);
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

static void *_get_input_buffer(net_adapter_t *adapter, unsigned *plength)
{
	bool done = false;
	rx_edesc_t *desc = _rx_desc_ptr1;
	unsigned rdes0 = desc->rdes[0];
	if (0 == (rdes0 & RDES0_OWN))
	{
		if (rdes0 & RDES0_LS)	// last desc for this frame
		{
			unsigned length = (rdes0 & RDES0_FL_MASK) >> RDES0_FL_BIT;
			*plength = length;
			done = true;
		 } else _verbose(VERBOSE_ERROR, "rx frame didn't fit in a single desc");

		 _rx_desc_ptr1 = (rx_edesc_t *)desc->rdes[3];
	}
	return done ? (void *)desc->rdes[2] : NULL;
}

static void _discard_input_buffer(net_adapter_t *adapter, void *buffer)
{
	ASSERT(buffer != NULL, KERNEL_ERROR_NULL_POINTER);

	// TODO: reorder descriptors if needed

	rx_edesc_t *desc = _rx_desc_ptr2;
	ASSERT(buffer == (void *)desc->rdes[2], KERNEL_ERROR_KERNEL_PANIC);
	_rx_desc_ptr2 = (rx_edesc_t *)desc->rdes[3];	// next
	desc->rdes[0] = RDES0_OWN;
}

static void *_get_output_buffer(net_adapter_t *adapter, unsigned size)
{
}

static int _send_output_buffer(net_adapter_t *adapter, net_mbuf_t *mbuf, NET_CALLBACK callback, void *state)
{
	// write TransmitPollDemandRegister to wake up the descriptor parsing
	ETH->DMATPDR = 0;
}






void ETH_IRQHandler()
{
	ASSERT(_adapter != NULL, KERNEL_ERROR_NULL_POINTER);
	unsigned dma_sr = ETH->DMASR;
	if (dma_sr & ETH_DMASR_RS)	// Receive Status
	{
		exos_event_set(&_adapter->InputEvent);
		ETH->DMASR = ETH_DMASR_RS | ETH_DMASR_NIS;	// clear
	}

}

void ETH_WKUP_IRQHandler()
{
	kernel_panic(KERNEL_ERROR_NOT_IMPLEMENTED);
}



