// LPC17xx Ethernet Support (Static Memory Model)
// by Miguel Fides

#include "emac.h"
#include "emac_mem.h"
#include "cpu.h"
#include <net/adapter.h>

#define ETH_BUFFER_SIZE	1514
typedef unsigned char ETH_BUFFER[ETH_BUFFER_SIZE + 4];

#define __eth __attribute__((section(".dma")))

#define ENET_LPC_RX_DESCRIPTORS (8)   // RX Fragments = RX packet buffers
static ETH_RX_DESC _rx_desc[ENET_LPC_RX_DESCRIPTORS] __eth;
static ETH_RX_STATUS _rx_stat[ENET_LPC_RX_DESCRIPTORS] __eth;
static ETH_BUFFER _rx_buffers[ENET_LPC_RX_DESCRIPTORS] __eth;

#define ENET_LPC_TX_DESCRIPTORS (8)   // TX Fragments
static ETH_TX_DESC _tx_desc[ENET_LPC_TX_DESCRIPTORS] __eth;
static ETH_TX_STATUS _tx_stat[ENET_LPC_TX_DESCRIPTORS] __eth;
static ETH_TX_REQUEST _tx_req[ENET_LPC_TX_DESCRIPTORS];

static unsigned long _tx_free_index, _rx_read_index;

#define ENET_LPC_TX_BUFFERS ((ENET_LPC_TX_DESCRIPTORS + 1) / 2)
static ETH_BUFFER _tx_buffers[ENET_LPC_TX_BUFFERS] __eth;
static ETH_BUFFER *_free_buffers[ENET_LPC_TX_BUFFERS];
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
	for (int i = 0; i < ENET_LPC_TX_BUFFERS; i++)
		_free_buffers[i] = &_tx_buffers[i];
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
		return _free_buffers[index];
	}
	return NULL;
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
		
		// setup fragment
		_tx_desc[index].Data = mbuf->Buffer + mbuf->Offset;
		_tx_desc[index].Control = ((mbuf->Length - 1) & ETH_TX_DESC_CONTROL_SIZE_MASK)
			| ETH_TX_DESC_CONTROL_OVERRIDE
			| ETH_TX_DESC_CONTROL_PAD | ETH_TX_DESC_CONTROL_CRC 
			| ((mbuf->Next == NULL) ? ETH_TX_DESC_CONTROL_LAST | ETH_TX_DESC_CONTROL_INTERRUPT : 0);
		_tx_stat[index].Status = 0;

        ETH_TX_REQUEST *req = &_tx_req[index];
		req->Callback = callback;
		req->CallbackState = state;
		req->Flags = done ? 0 : ETH_TX_REQ_HEADER_BUFFER; // NOTE: first buffer in each mbuf is for header buffer

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
	for(int count = 0; index != LPC_EMAC->TxConsumeIndex; count++)
	{
		ETH_TX_REQUEST *req = &_tx_req[index];
		NET_CALLBACK callback = req->Callback;
		if (callback != NULL) callback(req->CallbackState);

		if (req->Flags & ETH_TX_REQ_HEADER_BUFFER)
		{
			// reclaim header buffer
			void *data = _tx_desc[index].Data;
			unsigned long next = _txbuf_free_index;
			_free_buffers[next++] = data;
			if (next >= ENET_LPC_TX_BUFFERS) next = 0;
			_txbuf_free_index = next;
		}
		if (++index == ENET_LPC_TX_DESCRIPTORS) index = 0;
		_tx_free_index = index;
	}
}

