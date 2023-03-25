#include "eth.h"
#include "cpu.h"
#include "gpio.h"
#include <kernel/driver/net/adapter.h>
#include <kernel/machine/hal.h>
#include <kernel/panic.h>
#include <kernel/verbose.h>
#include <string.h>

#ifdef STM32_ETH_DEBUG
#define _verbose(level, ...) verbose(level, "eth", __VA_ARGS__)
#else
#define _verbose(level, ...)	{ /* nothing */ }
#endif

#ifndef TX_DESCRIPTORS
#define TX_DESCRIPTORS 16
#endif

#ifndef RX_DESCRIPTORS
#define RX_DESCRIPTORS 4
#endif

static struct _rx_buffer { unsigned char data[ETH_MAX_FRAME_SIZE]; } _rx_buffers[RX_DESCRIPTORS];
static rx_edesc_t _rx_desc[RX_DESCRIPTORS] __aligned(32);
static tx_edesc_t _tx_desc[TX_DESCRIPTORS] __aligned(32);

static rx_edesc_t * volatile _rx_desc_ptr;
static tx_edesc_t * volatile _tx_desc_ptr1;
static tx_edesc_t * volatile _tx_desc_ptr2;

static bool _initialize(net_adapter_t *adapter, unsigned phy_unit, const phy_handler_t *handler);
static void _eth_start(net_adapter_t *adapter, dispatcher_context_t *context);
static void _link_up(net_adapter_t *adapter);
static void _link_down(net_adapter_t *adapter);
static net_buffer_t *_get_input_buffer(net_adapter_t *adapter);
static bool _send_output_buffer(net_adapter_t *adapter, net_buffer_t *buf);
static void _flush(net_adapter_t *adapter);
const net_driver_t __stm32_eth_driver = { .Initialize = _initialize, .Start = _eth_start,
	.LinkUp = _link_up, .LinkDown = _link_down,
	.GetInputBuffer = _get_input_buffer, .SendOutputBuffer = _send_output_buffer,
	.Flush = _flush }; 

static void _phy_write(unsigned unit, phy_reg_t reg, unsigned short value);
static unsigned short _phy_read(unsigned unit, phy_reg_t reg);
static const phy_driver_t _phy_driver = { .Write = _phy_write, .Read = _phy_read };

static void _output_callback(dispatcher_context_t *context, dispatcher_t *dispatcher);

static net_adapter_t *_adapter = NULL;
static event_t _output_event;
static dispatcher_t _output_dispatcher;

static bool _initialize(net_adapter_t *adapter, unsigned phy_unit, const phy_handler_t *handler)
{
	ASSERT(SystemCoreClock >= 25000000UL, KERNEL_ERROR_KERNEL_PANIC);
	RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;

	ASSERT(_adapter == NULL, KERNEL_ERROR_KERNEL_PANIC);
	_adapter = adapter;
	adapter->Name = "eth";
	adapter->MaxFrameSize = ETH_MAX_FRAME_SIZE;

	exos_event_create(&_output_event, EXOS_EVENTF_AUTORESET);
	exos_dispatcher_create(&_output_dispatcher, &_output_event, _output_callback, adapter);

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

	for(unsigned i = 0; i < TX_DESCRIPTORS; i++)
	{
		unsigned nexti = (i != TX_DESCRIPTORS - 1) ? i + 1 : 0;
		tx_edesc_t *next = &_tx_desc[nexti];
		_tx_desc[i] = (tx_edesc_t) { .tdes[3] = (unsigned)next };
	}
	_tx_desc_ptr1 = _tx_desc_ptr2 = &_tx_desc[0];
	
	for(unsigned i = 0; i < RX_DESCRIPTORS; i++)
	{
		unsigned nexti = (i != RX_DESCRIPTORS - 1) ? i + 1 : 0;
		rx_edesc_t *next = &_rx_desc[nexti];
		_rx_desc[i] = (rx_edesc_t) { .rdes[0] = RDES0_OWN,
			.rdes[1] = RDES1_RCH | ETH_MAX_FRAME_SIZE,
			.rdes[2] = (unsigned)&_rx_buffers[i],
			.rdes[3] = (unsigned)next };
	}
	_rx_desc_ptr = &_rx_desc[0];

	ETH->DMABMR = 0		// TTC <- TxFIFO mode = Store-and-forward
		| ETH_DMABMR_FB	// FB Fixed Burst
		| ETH_DMABMR_PBL_4Beat	// Programmable Burst Length
		| ETH_DMABMR_EDE		// Enhanced Descriptor format Enable
		| (0 << ETH_DMABMR_DSL_Pos)	// Descriptor Skip Length
		| ETH_DMABMR_DA			// DMA Arbitration (Rx has priority over Tx)
		;

	ETH->DMAOMR = 0
		| ETH_DMAOMR_DTCEFD	// DTCEFD Do not drop TCP Checksum Error Frames <<<<<<<<<<< FIXME: Remove 
		| ETH_DMAOMR_TSF	// TSF Transmit Store-and-forward
		; 
	// You must make sure the Transmit FIFO is deep enough to store a complete frame before that frame is transferred 
	// to the MAC Core transmitter. If the FIFO depth is less than the input Ethernet frame size, the payload 
	// (TCP/UDP/ICMP) checksum insertion function is bypassed and only the frame's IPv4 Header checksum is modified, 
	// even in Store-and-forward mode.

	ETH->DMATDLAR = (unsigned)_tx_desc;	// address of first transmit descriptor
	ETH->DMARDLAR = (unsigned)_rx_desc;	// address of first receive descriptor
	ETH->DMAOMR |= ETH_DMAOMR_SR | ETH_DMAOMR_ST;	// Start Tx / Start Rx
	ETH->DMAIER = ETH_DMAIER_RIE | ETH_DMAIER_TIE	// Receive, Transmit
		| ETH_DMAIER_NISE	// Normal Int Summary Enable
		| ETH_DMAIER_AISE;	 // TODO: Abnormal Int Summary Enable

	ETH->MACFCR;	// BPA <- Back Pressure Active?
	ETH->MACFFR = ETH_MACFFR_RA 	// Receive All
		| ETH_MACFFR_PM;
	ETH->MACCR = 0		// IFG <- interframe gap?
		| ETH_MACCR_FES	// Fast Ethernet Speed (phy negotiated)
		| ETH_MACCR_DM	// full Duplex Mode enable (phy negotiated)
		| ETH_MACCR_TE | ETH_MACCR_RE;	// Tx, Rx Enable

	NVIC_EnableIRQ(ETH_IRQn);
	return true;
}

static void _eth_start(net_adapter_t *adapter, dispatcher_context_t *context)
{
	exos_dispatcher_add(context, &_output_dispatcher, EXOS_TIMEOUT_NEVER);
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
	_verbose(VERBOSE_DEBUG, "link_up() ignored");
}

static void _link_down(net_adapter_t *adapter)
{
	kernel_panic(KERNEL_ERROR_NOT_IMPLEMENTED);
}

static net_buffer_t *_get_input_buffer(net_adapter_t *adapter)
{
	ASSERT(adapter != NULL, KERNEL_ERROR_NULL_POINTER);

	net_buffer_t *buf = NULL;

	rx_edesc_t *desc = _rx_desc_ptr;
	unsigned rdes0;
	while (rdes0 = desc->rdes[0], (rdes0 & (RDES0_OWN | RDES0_FS)) == RDES0_FS)
	{
		// NOTE: our receive buffers should hold a complete frame
		if (rdes0 & RDES0_LS)	// last desc for this frame
		{
			buf = net_adapter_alloc_buffer(adapter);
			if (buf != NULL)
			{
				unsigned fl = (desc->rdes[0] & RDES0_FL_MASK) >> RDES0_FL_BIT;
				ASSERT(fl <= buf->Root.Length, KERNEL_ERROR_KERNEL_PANIC);
				memcpy(buf->Root.Buffer, (void *)desc->rdes[2], fl);
				buf->Root.Length = fl;
			}
		}
		else 
		{
			_verbose(VERBOSE_ERROR, "rx frame didn't fit in a single desc");
			do
			{
				desc->rdes[0] = RDES0_OWN;
				desc = (rx_edesc_t *)desc->rdes[3];	// next
				rdes0 = desc->rdes[0];
			} while((rdes0 & RDES0_LS) == 0);
		}

		desc->rdes[0] = RDES0_OWN;
		desc = (rx_edesc_t *)desc->rdes[3];	// next

		if (buf != NULL)
			break;
	}
	_rx_desc_ptr = desc;
	return buf;
}

static bool _send_output_buffer(net_adapter_t *adapter, net_buffer_t *buf)
{
	ASSERT(adapter != NULL, KERNEL_ERROR_NULL_POINTER);

	tx_edesc_t *desc = _tx_desc_ptr1;
	net_mbuf_t *mbuf = &buf->Root;
	unsigned count = 0;
	unsigned tdes0 = TDES0_FS | TDES0_TCH | TDES0_IC;	// First Segment, TDES3 is desc CHain ptr, Int. on Complete
	while(mbuf->Buffer != NULL && mbuf->Length != 0)
	{
		if ((desc->tdes[0] & TDES0_OWN) != 0)	// buffer is still owned by dma
		{
			count = 0;
			break;
		}
		else 
		{
			if (mbuf->Next == NULL) tdes0 |= TDES0_LS;
			else if (mbuf->Next->Buffer == NULL || mbuf->Next->Length == 0) tdes0 |= TDES0_LS;
			desc->tdes[0] = tdes0;
			desc->tdes[1] = mbuf->Length;
			desc->tdes[2] = (unsigned)mbuf->Buffer + mbuf->Offset;
			desc->tdes[4] = (count == 0) ? (unsigned)buf : 0;
#ifdef DEBUG
			desc->tdes[5] = (unsigned)buf;
#endif
			desc = (tx_edesc_t *)desc->tdes[3];
			count++;
		}

		if (tdes0 & TDES0_LS)
			break;

		if (desc == _tx_desc_ptr2)
		{
			count = 0;
			break;
		}

		tdes0 &= ~TDES0_FS;
		mbuf = mbuf->Next;
	}

	if (count != 0)
	{
		desc = _tx_desc_ptr1;
		for (unsigned i = 0; i < count; i++)
		{
			// FIXME: should we set OWN on all segments after the first one? <<<<<<<<<<<<
			desc->tdes[0] |= TDES0_OWN;
			desc = (tx_edesc_t *)desc->tdes[3];
		}
		_tx_desc_ptr1 = desc;

		// write TransmitPollDemandRegister to wake up the descriptor parsing
		ETH->DMATPDR = 0;
		
		_verbose(VERBOSE_COMMENT, "send_output() queued buffer @$%x", (unsigned)buf & 0xffff);
		return true;
	}
	_verbose(VERBOSE_ERROR, "send_output() failed!");
	return false;
}

static net_buffer_t *_reclaim_tx_buffer(bool flush)
{
	tx_edesc_t *desc = _tx_desc_ptr2;
	unsigned tdes0 = desc->tdes[0];
	net_buffer_t *buf = (net_buffer_t *)desc->tdes[4];
	if (buf != NULL && 
		(!(tdes0 & TDES0_OWN) || flush))
	{
		ASSERT(buf == (net_buffer_t *)desc->tdes[5], KERNEL_ERROR_KERNEL_PANIC);
		ASSERT(buf->Root.Buffer == (void *)desc->tdes[2], KERNEL_ERROR_KERNEL_PANIC);
		ASSERT((desc->tdes[0] & TDES0_FS) != 0, KERNEL_ERROR_KERNEL_PANIC);
		
		desc->tdes[0] = 0;
		desc->tdes[4] = (unsigned)NULL;	// clear buffer reference

		tx_edesc_t *next = (tx_edesc_t *)desc->tdes[3];
		ASSERT(next != NULL, KERNEL_ERROR_NULL_POINTER);
		while((tdes0 & TDES0_LS) == 0)
		{
			desc = next;
			tdes0 = desc->tdes[0];
			desc->tdes[0] = 0;
			desc->tdes[4] = (unsigned)NULL;	// clear buffer reference

			tx_edesc_t *next = (tx_edesc_t *)desc->tdes[3];
			ASSERT(next != NULL, KERNEL_ERROR_NULL_POINTER);
		}
		desc = next;
		_tx_desc_ptr2 = desc;
		return buf;
	}
	return NULL;
}

static void _output_callback(dispatcher_context_t *context, dispatcher_t *dispatcher)
{
	net_adapter_t *adapter = (net_adapter_t *)dispatcher->CallbackState;
	ASSERT(adapter != NULL, KERNEL_ERROR_NULL_POINTER);

	net_buffer_t *buf;
	while(buf = _reclaim_tx_buffer(false), buf != NULL)
	{
		_verbose(VERBOSE_COMMENT, "freed buf @$%x", (unsigned)buf & 0xffff);
		net_adapter_free_buffer(buf);
	}

	exos_dispatcher_add(context, dispatcher, EXOS_TIMEOUT_NEVER);
}

static void _flush(net_adapter_t *adapter)
{
	net_buffer_t *buf;
	while(buf = _reclaim_tx_buffer(true), buf != NULL)
	{
		_verbose(VERBOSE_DEBUG, "flushed buf @$%x", (unsigned)buf & 0xffff);
		net_adapter_free_buffer(buf);
	}
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
	if (dma_sr & ETH_DMASR_TS)	// Transmit Status
	{
		exos_event_set(&_output_event);
		ETH->DMASR = ETH_DMASR_TS | ETH_DMASR_NIS;	// clear 
	}
}

void ETH_WKUP_IRQHandler()
{
	kernel_panic(KERNEL_ERROR_NOT_IMPLEMENTED);
}



