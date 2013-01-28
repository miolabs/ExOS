// EMAC Controller for TMS320DM36x
// by Miguel Fides

#include "emac.h"
#include "emac_mdio.h"
#include "system.h"
#include "intc.h"
#include <kernel/panic.h>
#include <net/mbuf.h>

#define EMAC_INT_PRI INTC_PRI_IRQ_MEDHIGH

static EMAC_MODULE *_emac = (EMAC_MODULE *)0x01D07000;
static EMAC_MODULE_POINTERS *_emac_ptrs = (EMAC_MODULE_POINTERS *)0x01D07600;
static EMAC_CONTROL_MODULE *_emac_control = (EMAC_CONTROL_MODULE *)0x01D0A000;
static unsigned long *_emac_stats = (unsigned long *)0x01D07200;

#define EMAC_RX_BUFFERS 16
#define EMAC_TX_BUFFERS 32
#define EMAC_TOTAL_DESCRIPTORS (EMAC_TX_BUFFERS + EMAC_RX_BUFFERS)

static const unsigned char _pad_buffer[] = "padpadpadpadpadpadpadpadpadpadpadpadpadpad";

typedef struct
{
	volatile EMAC_Desc *Head;
	volatile EMAC_Desc *Tail;
} EMAC_Chain;

static EMAC_Desc _desc_array[EMAC_TOTAL_DESCRIPTORS] __attribute__((__section__(".emac")));
static int _desc_allocated = 0;
static EMAC_PKT_BUFFER _rx_buffers[EMAC_RX_BUFFERS];
static EMAC_PKT_BUFFER _tx_buffers[EMAC_TX_BUFFERS];
static EMAC_Chain _rx_chain;
static EMAC_Desc *_rx_ready_head;
static EMAC_Chain _tx_chain;
static EMAC_Desc *_tx_last_load;
static EMAC_Chain _tx_free_chain;
static int _tx_index = 0;

static void _buffers_allocate(EMAC_Chain *chain, int count);
static void _set_mac(int ch, unsigned char *mac_addr);

void emac_initialize(unsigned char *mac_addr)
{
	_desc_allocated = 0;

	system_select_pinmux(17, 1);	// EMAC_TX_EN
	system_select_pinmux(16, 1);	// EMAC_TX_CLK
	system_select_pinmux(15, 1);	// EMAC_COL
	system_select_pinmux(14, 1);	// EMAC_TXD3
	system_select_pinmux(13, 1);	// EMAC_TXD2
	system_select_pinmux(12, 1);	// EMAC_TXD1
	system_select_pinmux(11, 1);	// EMAC_TXD0
	system_select_pinmux(10, 1);	// EMAC_RXD3
	system_select_pinmux(9, 1);		// EMAC_RXD2
	system_select_pinmux(8, 1);		// EMAC_RXD1
	system_select_pinmux(7, 1);		// EMAC_RXD0
	system_select_pinmux(6, 1);		// EMAC_RX_CLK
	system_select_pinmux(5, 1);		// EMAC_RX_DV
	system_select_pinmux(4, 1);		// EMAC_RX_ER
	system_select_pinmux(3, 1);		// EMAC_CRS
	system_select_pinmux(2, 1);		// EMAC_MDIO
	system_select_pinmux(1, 1);		// EMAC_MDCLK

	system_select_intmux(52, 1);	// INT52 = EMACRXTHRESH
	system_select_intmux(53, 1);	// INT53 = EMACRXPULSE
	system_select_intmux(54, 1);	// INT54 = EMACTXPULSE
	system_select_intmux(55, 1);	// INT55 = EMACMISCPULSE

	psc_set_module_state(PSC_MODULE_EMAC, PSC_MODULE_ENABLE);
	int emac_clk = system_get_sysclk(PLLC1, PLLC_SYSCLK4);

	_emac->SOFTRESET = 1;
	while(_emac->SOFTRESET);

	_emac_control->CMSOFTRESET = 1;
	
	// disable interrupts
	_emac_control->CMRXINTEN = 0;
	_emac_control->CMTXINTEN = 0;
	_emac_control->CMTHRESHINTEN = 0;
	_emac_control->CMMISCINTEN = 0;

	// Full duplex enable bit set when auto negotiation happens
	_emac->MACCONTROL = EMAC_MACCONTROL_TXPTYPE // fixed-priority
		| EMAC_MACCONTROL_FULLDUPLEX;

	_emac->RXMBPENABLE = EMAC_RXMBP_RXBROADEN 
		| (0 << EMAC_RXMBP_RXBROADCH_BIT);
	_emac->RXMAXLEN = EMAC_MAX_ETHERNET_PKT_SIZE;

	_emac->RXBUFFEROFFSET = 0;
	_emac->RXFILTERLOWTHRESH = 0;
	_emac->RXUNICASTCLEAR = 0xFF;
	for(int i = 0; i < 8; i++)
	{
		_emac_ptrs->TXHDP[i] = NULL;
		_emac_ptrs->RXHDP[i] = NULL;
	}

	_emac->MACSRCADDRHI =
		(mac_addr[3] << 24) |
		(mac_addr[2] << 16) |
		(mac_addr[1] << 8) |
		mac_addr[0];
	_emac->MACSRCADDRLO =
		(mac_addr[5] << 8) |
		mac_addr[4];

	_set_mac(0, mac_addr);
	_emac->RXINTMASKSET = (1<<0);	// enable ch int
	_emac->TXINTMASKSET = (1<<0);	// enable ch int

   	_buffers_allocate(&_rx_chain, EMAC_RX_BUFFERS);
	EMAC_PKT_BUFFER *rx_pkt = _rx_buffers;
	for(EMAC_Desc *rx_desc = (EMAC_Desc *)_rx_chain.Head; 
		rx_desc != NULL; rx_desc = rx_desc->Next)
	{
		rx_desc->BufOffLen = EMAC_MAX_ETHERNET_PKT_SIZE;
		rx_desc->PktFlgLen = EMAC_DSC_FLAG_OWNER;
		rx_desc->Buffer = (unsigned char *)rx_pkt++;
	}
	_rx_ready_head = (EMAC_Desc *)_rx_chain.Head;
	_emac_ptrs->RXHDP[0] = _rx_ready_head;
	_emac->RXUNICASTSET = (1<<0);

   	_buffers_allocate(&_tx_free_chain, EMAC_TX_BUFFERS);
	_tx_chain.Head = _tx_chain.Tail = NULL;
	_emac_ptrs->TXHDP[0] = _tx_last_load = NULL;

	// clear stats
	for(int i = 0; i < 36; i++) _emac_stats[i] = 0; 

	_emac->TXCONTROL |= 1;
	_emac->RXCONTROL |= 1;
	_emac->MACINTMASKSET = EMAC_MACINTMASK_HOST;

	_emac->MACCONTROL |= EMAC_MACCONTROL_MIIEN;
	
	_emac_control->CMRXINTEN = 0xFF;
	_emac_control->CMTXINTEN = 0xFF;
	_emac_control->CMTHRESHINTEN = 0xFF;
	_emac_control->CMMISCINTEN = 0xFF;

	emac_mdio_reset(emac_clk);

    intc_set_priority(52, 1, EMAC_INT_PRI);
    intc_set_priority(53, 1, EMAC_INT_PRI);
    intc_set_priority(54, 1, EMAC_INT_PRI);
    intc_set_priority(55, 1, EMAC_INT_PRI);
}

static void _buffers_allocate(EMAC_Chain *chain, int count)
{
	EMAC_Desc *head = &_desc_array[_desc_allocated++];
	EMAC_Desc *desc = head;
	for (int i = 1; i < count; i++)
	{
		EMAC_Desc *next = &_desc_array[_desc_allocated++];
		desc->Next = next;
		desc = next;
	}
	desc->Next = (EMAC_Desc *)0;

	chain->Head = head;
	chain->Tail = desc;
}

// EMACRXTHRESH
void INT52_Handler()
{
}

// EMACRXPULSE
void INT53_Handler()
{
	int valid_count = 0;
	EMAC_Desc *rx_desc = (EMAC_Desc *)_rx_chain.Head;
	while (rx_desc != NULL)
	{
		unsigned long flags = rx_desc->PktFlgLen;
		if ((flags & EMAC_DSC_FLAG_OWNER) == 0)
		{
			if (flags & EMAC_DSC_FLAG_EOP)
				_emac_ptrs->RXCP[0] = rx_desc;
			rx_desc = rx_desc->Next;
			valid_count++;
		}
		else break; 
	}
	_rx_chain.Head = rx_desc;

	if (valid_count != 0)
	{
		// notify reception
		emac_dm36x_rx_handler();
	}
    _emac->MACEOIVECTOR = EMAC_EOI_RXPULSE;
}

// EMACTXPULSE
void INT54_Handler()
{
	EMAC_Desc *tx_desc = (EMAC_Desc *)_tx_chain.Head;
	if (tx_desc == NULL) tx_desc = _tx_last_load;
	
	while (tx_desc != NULL)
	{
		unsigned long flags = tx_desc->PktFlgLen;
		if ((flags & (EMAC_DSC_FLAG_SOP | EMAC_DSC_FLAG_OWNER)) == (EMAC_DSC_FLAG_SOP | EMAC_DSC_FLAG_OWNER)) 
			break;
		
		if (flags & EMAC_DSC_FLAG_EOP)
			_emac_ptrs->TXCP[0] = tx_desc;
		
		if (tx_desc->Callback != NULL)
			tx_desc->Callback(tx_desc->CallbackState);

		_tx_free_chain.Tail = _tx_free_chain.Tail->Next = tx_desc;
		tx_desc = tx_desc->Next;
		_tx_free_chain.Tail->Next = NULL;
		
	}
	_tx_chain.Head = tx_desc;
	_emac->MACEOIVECTOR = EMAC_EOI_TXPULSE;
}

// EMACMISCPULSE
void INT55_Handler()
{
}

void *emac_get_input_buffer(unsigned long *psize)
{
	EMAC_Desc *rx_desc = (EMAC_Desc *)_rx_ready_head;
	if (rx_desc != NULL && rx_desc != _rx_chain.Head)
	{
		*psize = rx_desc->PktFlgLen & 0xFFFF;
		return rx_desc->Buffer;
	}
	else
	{
		return NULL;
	}
}

void emac_discard_input_buffer(void *buffer)
{
	EMAC_Desc *rx_desc = _rx_ready_head;
#ifdef DEBUG
	if (rx_desc == NULL) kernel_panic(KERNEL_ERROR_UNKNOWN);
#endif
	_rx_ready_head = rx_desc->Next;
	if (rx_desc != NULL)
	{
		rx_desc->Buffer = buffer;

		// Recycle packet descriptor/buffer
		rx_desc->Next = NULL;
		rx_desc->BufOffLen = EMAC_MAX_ETHERNET_PKT_SIZE;
		rx_desc->PktFlgLen = EMAC_DSC_FLAG_OWNER;

		EMAC_Desc *rx_tail = (EMAC_Desc *)_rx_chain.Tail;
		_rx_chain.Tail = rx_tail->Next = rx_desc; 
		if (rx_tail->PktFlgLen & EMAC_DSC_FLAG_EOQ)
		{
			_rx_chain.Head = rx_desc;
			_emac_ptrs->RXHDP[0] = rx_desc;
			if (_rx_ready_head == NULL)
			{
				// this should never happen 
				// unless we are recycling the one and only descriptor
				_rx_ready_head = rx_desc;
			}
		}
	}
}

void *emac_get_output_buffer(unsigned long size)
{
	int index = _tx_index;
	EMAC_PKT_BUFFER *tx_pkt = &_tx_buffers[index];
	if (++index >= EMAC_TX_BUFFERS) index = 0;
	_tx_index = index;
	return tx_pkt;
}

int emac_send_output_buffer(NET_MBUF *mbuf, NET_CALLBACK callback, void *state)
{
	int complete = 0;
	if (mbuf != NULL)
	{
		NET_MBUF padding;
		int packet_length = net_mbuf_length(mbuf);
		if (packet_length < 60) // min eth payload length
		{
			net_mbuf_init(&padding, (void *)_pad_buffer, 0, 60 - packet_length);
			net_mbuf_append(mbuf, &padding);
			packet_length = 60;
		}

		if (packet_length <= EMAC_MAX_ETHERNET_PKT_SIZE)
		{
			EMAC_Desc *tx_desc = (EMAC_Desc *)_tx_free_chain.Head;
			for(int count = 0; tx_desc != NULL; count++)
			{
				tx_desc->Callback = (count == 0) ? callback : NULL;
				tx_desc->CallbackState = state; 

				tx_desc->Buffer = mbuf->Buffer + mbuf->Offset;
				tx_desc->BufOffLen = mbuf->Length;
				if (mbuf->Next == NULL)
				{
					tx_desc->PktFlgLen = (count != 0) ? EMAC_DSC_FLAG_EOP :
						packet_length | EMAC_DSC_FLAG_SOP | EMAC_DSC_FLAG_OWNER | EMAC_DSC_FLAG_EOP;

					// remove descriptors from free chain
					EMAC_Desc *tx_head = (EMAC_Desc *)_tx_free_chain.Head;
					_tx_free_chain.Head = tx_desc->Next;
					tx_desc->Next = NULL;

					// enqueue for tx
					if (_tx_chain.Tail == NULL)
					{
						_emac_ptrs->TXHDP[0] = _tx_last_load = tx_head;
					}
					else
					{
						_tx_chain.Tail->Next = tx_head;
						if (_tx_chain.Tail->PktFlgLen & EMAC_DSC_FLAG_EOQ)
						{                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     
							_emac_ptrs->TXHDP[0] = _tx_last_load = tx_head;
						}
					}
					_tx_chain.Tail = tx_desc;
					
					complete = 1;
					break;
				}

				tx_desc->PktFlgLen = (count != 0) ? 0 :
					packet_length | EMAC_DSC_FLAG_SOP | EMAC_DSC_FLAG_OWNER;
				tx_desc = tx_desc->Next;
				mbuf = mbuf->Next;
			}
		}
	}
	if (!complete) kernel_panic(KERNEL_ERROR_UNKNOWN);
	return complete;
}

static void _set_mac(int ch, unsigned char *mac_addr)
{
	_emac->MACINDEX = ch;
	_emac->MACADDRHI =
		(mac_addr[3] << 24) |
		(mac_addr[2] << 16) |
		(mac_addr[1] << 8) |
		(mac_addr[0]);
	_emac->MACADDRLO =
		(mac_addr[5] << 8) |
		(mac_addr[4]) |
		(ch << EMAC_MACADDRLO_CHANNEL_BIT) |
		EMAC_MACADDRLO_MATCHFILT | EMAC_MACADDRLO_VALID;
}







