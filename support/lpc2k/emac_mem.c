// LPC23xx/24xx Ethernet Support (Static Memory Model)
// by Miguel Fides

#include "emac.h"
#include "emac_mem.h"
#include <net/adapter.h>

#define ETH_BUFFER_SIZE	1514
typedef unsigned char ETH_BUFFER[ETH_BUFFER_SIZE + 4];

#define __eth __attribute__((section(".eth")))

#define ENET_LPC_RX_DESCRIPTORS (4)   // RX Fragments = RX packet buffers
static ETH_RX_DESC _rx_desc[ENET_LPC_RX_DESCRIPTORS] __eth;
static ETH_RX_STATUS _rx_stat[ENET_LPC_RX_DESCRIPTORS] __eth;
static ETH_BUFFER _rx_buffers[ENET_LPC_RX_DESCRIPTORS] __eth;

#define ENET_LPC_TX_DESCRIPTORS (4)   // TX Fragments
static ETH_TX_DESC _tx_desc[ENET_LPC_TX_DESCRIPTORS] __eth;
static ETH_TX_STATUS _tx_stat[ENET_LPC_TX_DESCRIPTORS] __eth;
static NET_CALLBACK _tx_callbacks[ENET_LPC_TX_DESCRIPTORS];
static void * _tx_callback_states[ENET_LPC_TX_DESCRIPTORS];

#define ENET_LPC_TX_BUFFERS ((ENET_LPC_TX_DESCRIPTORS + 1) / 2)
static ETH_BUFFER _tx_buffers[ENET_LPC_TX_BUFFERS] __eth;
static unsigned long _tx_free_index, _rx_read_index;
static unsigned long _txbuf_alloc_index, _txbuf_free_index;
static int _errors = 0;

void emac_mem_initialize()
{
	// init rx descriptors and buffers
	for (int i = 0; i < ENET_LPC_RX_DESCRIPTORS; i++)
	{
		_rx_desc[i].Data = _rx_buffers[i];
		_rx_desc[i].ControlBits.Size = ETH_BUFFER_SIZE - 1;
		_rx_desc[i].ControlBits.Interrupt = 1;
		_rx_stat[i].Status = 0;
	}

	LPC_EMAC->RxDescriptor = (unsigned long)_rx_desc;
	LPC_EMAC->RxStatus = (unsigned long)_rx_stat;
	LPC_EMAC->RxDescriptorNumber = ENET_LPC_RX_DESCRIPTORS - 1;
	
	_rx_read_index = LPC_EMAC->RxConsumeIndex = LPC_EMAC->RxProduceIndex;

	// init tx descriptors
	LPC_EMAC->TxDescriptor = (unsigned long)_tx_desc;
	LPC_EMAC->TxStatus = (unsigned long)_tx_stat;
	LPC_EMAC->TxDescriptorNumber = ENET_LPC_TX_DESCRIPTORS - 1;
	
	_tx_free_index = LPC_EMAC->TxProduceIndex = LPC_EMAC->TxConsumeIndex;
    _txbuf_alloc_index = _txbuf_free_index = 0;
}

void *emac_get_input_buffer(unsigned long *psize)
{
	unsigned long index = _rx_read_index;
	if (index == LPC_EMAC->RxProduceIndex) return (void*)0;

	*psize = _rx_stat[index].StatusBits.Size + 1; 
	void *buffer = _rx_desc[index].Data;

	if (++index == ENET_LPC_RX_DESCRIPTORS) index = 0;
	_rx_read_index = index;
	return buffer;
}

void emac_discard_input(void *data)
{
	unsigned long index = LPC_EMAC->RxConsumeIndex;
	if (index != _rx_read_index)
	{
		_rx_desc[index].Data = data;
		if (++index == ENET_LPC_RX_DESCRIPTORS) index = 0;
		LPC_EMAC->RxConsumeIndex = index;
	}
}

void *emac_get_output_buffer(unsigned long size)
{
	unsigned long next = _txbuf_alloc_index;
	unsigned long index = next++;
	if (next >= ENET_LPC_TX_BUFFERS) next = 0;
	if (next != _txbuf_free_index)
	{
		_txbuf_alloc_index = next;

		_tx_callbacks[index] = NULL;
		return &_tx_buffers[index];
	}
	return NULL;
}

static inline void _setup_fragment(unsigned long index, void *data, unsigned long length, int last, NET_CALLBACK callback, void *state)
{
	_tx_desc[index].Data = data;
	_tx_desc[index].Control = ((length - 1) & ETH_TX_DESC_CONTROL_SIZE_MASK)
		| ETH_TX_DESC_CONTROL_OVERRIDE
		| ETH_TX_DESC_CONTROL_PAD | ETH_TX_DESC_CONTROL_CRC 
		| (last ? ETH_TX_DESC_CONTROL_LAST | ETH_TX_DESC_CONTROL_INTERRUPT : 0);
	_tx_stat[index].Status = 0;
	_tx_callbacks[index] = callback;
	_tx_callback_states[index] = state;
}

int emac_send_output(NET_MBUF *mbuf, NET_CALLBACK callback, void *state)
{
	int done = 0;
	unsigned long next = LPC_EMAC->TxProduceIndex;
	while(mbuf != NULL)
	{
		unsigned long index = next++;
		if (next == ENET_LPC_TX_DESCRIPTORS) next = 0;
		
		if (next == LPC_EMAC->TxConsumeIndex)
			break;
		
		_setup_fragment(index, mbuf->Buffer + mbuf->Offset, mbuf->Length, 
			(mbuf->Next == NULL), callback, state);
		done = 1;
		callback = NULL;

		mbuf = mbuf->Next;
	}
	if (done) LPC_EMAC->TxProduceIndex = next;
	return done;
}

void emac_mem_tx_handler()
{
	unsigned long index = _tx_free_index;
	while(index != LPC_EMAC->TxConsumeIndex)
	{
		NET_CALLBACK callback = _tx_callbacks[index];
		if (callback != NULL) callback(_tx_callback_states[index]);

		if (_tx_desc[index].Data == &_tx_buffers[_txbuf_free_index])
		{
			_txbuf_free_index++;
			if (_txbuf_free_index == ENET_LPC_TX_BUFFERS)
				_txbuf_free_index = 0;
		}
		if (++index == ENET_LPC_TX_DESCRIPTORS) index = 0;
		_tx_free_index = index;
	}
}

