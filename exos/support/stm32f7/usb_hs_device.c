#include "usb_hs_device.h"
#include "cpu.h"
#include <support/usb/device_hal.h>
#include <kernel/panic.h>
#include <string.h>

#define USB_DEV_EP_COUNT 6

static usb_otg_crs_global_t * const otg_global = (usb_otg_crs_global_t *)(USB_OTG_HS_BASE + 0x000);
static usb_otg_crs_device_t * const otg_device = (usb_otg_crs_device_t *)(USB_OTG_HS_BASE + 0x800);
static usb_otg_crs_power_t * const otg_power = (usb_otg_crs_power_t *) (USB_OTG_HS_BASE + 0xe00);
static volatile uint32_t * const otg_fifo[] = {
	 (volatile uint32_t *)(USB_OTG_HS_BASE + 0x1000), (volatile uint32_t *)(USB_OTG_HS_BASE + 0x2000),
	 (volatile uint32_t *)(USB_OTG_HS_BASE + 0x3000), (volatile uint32_t *)(USB_OTG_HS_BASE + 0x4000),
	 (volatile uint32_t *)(USB_OTG_HS_BASE + 0x5000), (volatile uint32_t *)(USB_OTG_HS_BASE + 0x6000) };

#define USB_HS_ENABLE_DMA

static event_t _connected;
static usb_io_buffer_t *_ep_setup_io = nullptr;
static usb_io_buffer_t *_ep_out_io[USB_DEV_EP_COUNT];
static usb_io_buffer_t *_ep_in_io[USB_DEV_EP_COUNT];
static unsigned char _ep_max_length[USB_DEV_EP_COUNT];

// NOTE: total fifo size for OTG_FS is 1024 words (4 Kbytes)
#define FIFO_TOTAL_WORDS	1024
#define FIFO_TX_WORDS		32		// words for each IN EP (min 16)

// NOTE: GD32 requires double packet size of FIFO space for interrupt engine to work

void hal_usbd_initialize()
{
	exos_event_create(&_connected, EXOS_EVENTF_NONE);
	for(unsigned ep_num = 0; ep_num < USB_DEV_EP_COUNT; ep_num++)
		_ep_in_io[ep_num] = _ep_out_io[ep_num] = nullptr;

#ifdef DEBUG
	unsigned usb_clk = cpu_get_pll_qclk();
	ASSERT(usb_clk == 48000000UL, KERNEL_ERROR_KERNEL_PANIC);
#endif

	// core initialization
	usb_otg_hs_initialize();

#ifndef USB_HS_ENABLE_ID
	unsigned gusbcfg = otg_global->GUSBCFG & ~(USB_OTG_GUSBCFG_HNPCAP | USB_OTG_GUSBCFG_SRPCAP);
	otg_global->GUSBCFG = gusbcfg | USB_OTG_GUSBCFG_FDMOD;	// force usb-device mode
#endif
	exos_thread_sleep(10);

		// setup data FIFOs
	// NOTE: we have 4 tx fifos and 4 endpoints so we are going to use each tx fifo for the each in ep (with the same number)
	unsigned word_ptr = 0;
	unsigned rxf_size = FIFO_TOTAL_WORDS - (4 * FIFO_TX_WORDS);
	otg_global->GRXFSIZ = rxf_size << USB_OTG_GRXFSIZ_RXFD_Pos;
	word_ptr = rxf_size;
	for (unsigned i = 0; i < USB_DEV_EP_COUNT; i++)
	{
		unsigned txf_size = FIFO_TX_WORDS;
		unsigned txf_reg = (txf_size << USB_OTG_DIEPTXF_INEPTXFD_Pos) | 
				(word_ptr << USB_OTG_DIEPTXF_INEPTXSA_Pos);
		word_ptr += txf_size;
		switch(i)
		{
			case 0:	otg_global->DIEPTXF0 = txf_reg;	
					otg_global->HPTXFSIZ = txf_reg; break;
			case 1:	otg_global->DIEPTXF1 = txf_reg;	break;
			case 2:	otg_global->DIEPTXF2 = txf_reg;	break;
			case 3:	otg_global->DIEPTXF3 = txf_reg;	break;
#if USB_DEV_EP_COUNT >= 6
			case 4:	otg_global->DIEPTXF4 = txf_reg;	break;
			case 5:	otg_global->DIEPTXF5 = txf_reg;	break;
#endif
#if USB_DEV_EP_COUNT >= 8
			case 6:	otg_global->DIEPTXF6 = txf_reg;	break;
			case 7:	otg_global->DIEPTXF7 = txf_reg;	break;
#endif
			default:	kernel_panic(KERNEL_ERROR_NOT_SUPPORTED);
		}
	}

	otg_global->GRSTCTL = (0x10 << USB_OTG_GRSTCTL_TXFNUM_Pos) | USB_OTG_GRSTCTL_TXFFLSH;
	while(otg_global->GRSTCTL & USB_OTG_GRSTCTL_TXFFLSH);
	for(unsigned volatile i = 0; i < 100; i++);

	otg_global->GRSTCTL = USB_OTG_GRSTCTL_RXFFLSH;
	while(otg_global->GRSTCTL & USB_OTG_GRSTCTL_RXFFLSH);
	for(unsigned volatile i = 0; i < 100; i++);

	otg_device->DCTL = USB_OTG_DCTL_POPRGDNE | USB_OTG_DCTL_SDIS;	// disable until app calls connect()

#ifdef USB_HS_ENABLE_VBUS 
	#ifdef USB_OTG_GCCFG_VBDEN
		otg_global->GCCFG = USB_OTG_GCCFG_PWRDWN | USB_OTG_GCCFG_VBDEN;	
	#else
		otg_global->GCCFG = USB_OTG_GCCFG_PWRDWN | USB_OTG_GCCFG_VBUSBSEN;	
	#endif
#else
	#ifdef USB_OTG_GCCFG_NOVBUSSENS
		otg_global->GCCFG = USB_OTG_GCCFG_PWRDWN | USB_OTG_GCCFG_NOVBUSSENS;	
	#else
		otg_global->GCCFG = USB_OTG_GCCFG_PWRDWN;
	#endif
#endif

#ifdef USB_HS_ENABLE_DMA
	otg_global->GAHBCFG |= USB_OTG_GAHBCFG_DMAEN | (5 << USB_OTG_GAHBCFG_HBSTLEN_Pos);
#endif

#ifdef USB_HOST_ROLE_USES_DEVICE_SERVICE
	usb_otg_hs_notify(USB_HOST_ROLE_DEVICE);
#endif
}

void hal_usbd_connect(bool connect)
{
	if (connect)
	{
		otg_global->GINTSTS = USB_OTG_GINTSTS_USBRST | USB_OTG_GINTSTS_ENUMDNE;
		otg_global->GINTMSK |= USB_OTG_GINTMSK_USBRST | USB_OTG_GINTMSK_ENUMDNEM;

		otg_device->DCTL = 0;	// clear USB_OTG_DCTL_SDIS
		// NOTE: we should receive a reset now, when connected
	}
	else
	{
		otg_device->DCTL = USB_OTG_DCTL_SDIS;
//		_set_connect_status(USB_DEVSTA_DETACHED);
	}
}

bool hal_usbd_is_connected()
{
	//TODO
	return false;
}

bool hal_usbd_disconnected()
{
#ifdef USB_HOST_ROLE_USES_DEVICE_SERVICE
	usb_otg_hs_notify(USB_HOST_ROLE_DEVICE_CLOSING);
	return false;
#else
	return true;
#endif
}

void hal_usbd_suspend()
{
	//TODO
}

void hal_usbd_resume()
{
	//TODO
}

void hal_usbd_set_address(unsigned char addr)
{
	// NOTE: this call is used by the stack AFTER status-in phase then host sends a SetAddr command
	// nothing  
}

void hal_usbd_set_address_early(unsigned char addr)
{
	// NOTE: this call (early) is used by the stack BEFORE setup-in (status-in) when host sends a SetAddr command
	otg_device->DCFG |= addr << USB_OTG_DCFG_DAD_Pos;
	// NOTE: device address read from register afterwards is meaningless (errata)
}

void hal_usbd_configure(bool configure)
{
	//TODO
}



static void _configure_out_ep(unsigned ep_num, usb_transfer_type_t tt, unsigned max_packet_size)
{
	ASSERT(ep_num < USB_DEV_EP_COUNT, KERNEL_ERROR_KERNEL_PANIC);
	ASSERT(max_packet_size < 2048, KERNEL_ERROR_NOT_SUPPORTED);
	unsigned doepctl = (tt << USB_OTG_DOEPCTL_EPTYP_Pos) | USB_OTG_DOEPCTL_USBAEP | (max_packet_size & USB_OTG_DOEPCTL_MPSIZ_Msk);
	otg_device->DOEP[ep_num].CTL = doepctl;
	_ep_max_length[ep_num] = max_packet_size;
}

static void _configure_in_ep(unsigned ep_num, usb_transfer_type_t tt, unsigned max_packet_size)
{
	ASSERT(ep_num < USB_DEV_EP_COUNT, KERNEL_ERROR_KERNEL_PANIC);
	ASSERT(max_packet_size < 2048, KERNEL_ERROR_NOT_SUPPORTED);
	unsigned diepctl = (ep_num << USB_OTG_DIEPCTL_TXFNUM_Pos) | (tt << USB_OTG_DIEPCTL_EPTYP_Pos) | 
		USB_OTG_DIEPCTL_USBAEP | (max_packet_size & USB_OTG_DIEPCTL_MPSIZ_Msk);
	otg_device->DIEP[ep_num].CTL = diepctl;
	_ep_max_length[ep_num] = max_packet_size;
}

void hal_usbd_enable_out_ep(unsigned ep_num, usb_transfer_type_t tt, unsigned max_packet_size)
{
	ASSERT(ep_num < USB_DEV_EP_COUNT, KERNEL_ERROR_KERNEL_PANIC);
	_configure_out_ep(ep_num, tt, max_packet_size);
}

void hal_usbd_enable_in_ep(unsigned ep_num, usb_transfer_type_t tt, unsigned max_packet_size)
{
	ASSERT(ep_num < USB_DEV_EP_COUNT, KERNEL_ERROR_KERNEL_PANIC);
	_configure_in_ep(ep_num, tt, max_packet_size);
}

void hal_usbd_prepare_setup_ep(usb_io_buffer_t *iob)
{
	ASSERT(iob != nullptr, KERNEL_ERROR_NULL_POINTER);
	ASSERT(iob->Event != nullptr, KERNEL_ERROR_NULL_POINTER);

	// NOTE: system will call this just after hal_usbd_initialize() to assign buffers for setup packets
	// hal_usbd_connect() is not called yet
	// NOTE: this is also called after setup-out done, BEFORE data(in/out) or status phase 

	iob->Done = 0;
	iob->Status = USB_IOSTA_OUT_WAIT;

	_ep_setup_io = iob;
#ifdef USB_HS_ENABLE_DMA
	otg_device->DOEP[0].DMA = (unsigned)iob->Data;
#endif
	otg_device->DOEP[0].CTL = USB_OTG_DOEPCTL_EPENA | USB_OTG_DOEPCTL_CNAK;	 // NOTE: size is taken from DIEPCTL0
}


static void _prepare_out_ep(unsigned ep_num, usb_io_buffer_t *iob)
{
	unsigned max_packet_length = _ep_max_length[ep_num];
	unsigned rxlen = iob->Length - iob->Done;
	unsigned packet_count = rxlen / max_packet_length;
	unsigned sp = rxlen - (packet_count * max_packet_length);
	if (sp != 0 || packet_count == 0)
		packet_count++;

	unsigned siz = 0;
	if (ep_num == 0)
	{
		// NOTE: ep0 uses a fixed packet length from DIEP0CTL.MPSIZE
		// NOTE: oep0 XFRSIZ field is limited to 7 bit
		if (rxlen >= 128) rxlen = 128;
		if (packet_count > 1) packet_count = 1;
		siz = (1 << USB_OTG_DOEPTSIZ_STUPCNT_Pos);
	}
	else
	{
		siz = (packet_count << USB_OTG_DOEPTSIZ_PKTCNT_Pos)
			| (rxlen << USB_OTG_DOEPTSIZ_XFRSIZ_Pos);
	}

	otg_device->DOEP[ep_num].TSIZ = siz;
#ifdef USB_HS_ENABLE_DMA
	otg_device->DOEP[ep_num].DMA = (unsigned)(iob->Data + iob->Done);
#endif
	otg_device->DOEP[ep_num].CTL |= USB_OTG_DOEPCTL_EPENA | USB_OTG_DOEPCTL_CNAK;
}

void hal_usbd_prepare_out_ep(unsigned ep_num, usb_io_buffer_t *iob)
{
	ASSERT(ep_num < USB_DEV_EP_COUNT, KERNEL_ERROR_KERNEL_PANIC);
	ASSERT(iob != nullptr, KERNEL_ERROR_NULL_POINTER);
	ASSERT(iob->Event != nullptr, KERNEL_ERROR_NULL_POINTER);

	iob->Done = 0;
	iob->Status = USB_IOSTA_OUT_WAIT;

	_ep_out_io[ep_num] = iob;
	_prepare_out_ep(ep_num, iob);

	// NOTE: pio mode doesn't use out-ep irq handler because all handling is done in rxfifo handler
#ifdef USB_HS_ENABLE_DMA
	if (ep_num != 0)
	{
		unsigned mask  = (1 << USB_OTG_DAINTMSK_OEPM_Pos) << ep_num;
		otg_device->DAINTMSK |= mask;
	}
#endif
}


static void _disable_in_ep(unsigned ep_num)
{
	ASSERT(ep_num < USB_DEV_EP_COUNT, KERNEL_ERROR_KERNEL_PANIC);

	// TODO: flush fifo to ensure room  
	otg_device->DIEP[ep_num].CTL |= USB_OTG_DIEPCTL_EPDIS;
	// NOTE: will trigger an ep disabled interrupt
}


static unsigned char _in_ep_last_packet[USB_DEV_EP_COUNT];
static unsigned short _in_ep_rem_length[USB_DEV_EP_COUNT];

static void _write_fifo(unsigned ep_num, usb_io_buffer_t *iob)
{
	static volatile bool busy = false;
	ASSERT(!busy, KERNEL_ERROR_KERNEL_PANIC);
	busy = true;

	ASSERT(ep_num < USB_DEV_EP_COUNT, KERNEL_ERROR_KERNEL_PANIC);
	ASSERT(iob != nullptr, KERNEL_ERROR_NULL_POINTER);
	unsigned txlen = _in_ep_rem_length[ep_num];
	unsigned rem = txlen;
	unsigned char *ptr = (unsigned char *)iob->Data + iob->Done; 

	if (rem != 0)
	{
		unsigned short avail_words = otg_device->DIEP[ep_num].TXFSTS;

		// NOTE: check that fifo is properly initialized
		ASSERT(avail_words <= FIFO_TX_WORDS, KERNEL_ERROR_KERNEL_PANIC);

		volatile uint32_t *fifo = otg_fifo[ep_num];	// FIXME: ep number may NOT be fifo number
		unsigned rem_words = avail_words;
		while(rem > 3 && rem_words != 0)
		{
			*fifo = ptr[0] | (ptr[1] << 8) | (ptr[2] << 16) | (ptr[3] << 24);
			rem -= 4;
			ptr += 4;
			rem_words--;
		}
		if (rem < 4 && rem_words != 0)
		{
			switch(rem)
			{
				case 1:	*fifo = ptr[0];	break;
				case 2: *fifo = ptr[0] | (ptr[1] << 8);	break;
				case 3: *fifo = ptr[0] | (ptr[1] << 8) | (ptr[2] << 16);	break;
			}
			rem = 0;
		}

		unsigned done = txlen - rem;
		ASSERT(done != 0, KERNEL_ERROR_KERNEL_PANIC);
		iob->Done += done;
		ASSERT(iob->Done <= iob->Length, KERNEL_ERROR_KERNEL_PANIC);

		_in_ep_rem_length[ep_num] = rem;
	}

	unsigned ep_mask = (1 << ep_num);
	if (rem == 0)
	{
		otg_device->DIEPEMPMSK &= ~ep_mask;	// NOTE: disable tx-fifo interrupt
		
		iob->Status = USB_IOSTA_IN_COMPLETE;
		// NOTE: core will generate a XFRCM (transfer complete) int later
	}
	busy = false;
}

static void _prepare_in_ep(unsigned ep_num, usb_io_buffer_t *iob)
{
	unsigned max_packet_length = _ep_max_length[ep_num];
	ASSERT(max_packet_length >= 8 && max_packet_length <= 64, KERNEL_ERROR_KERNEL_PANIC);
	unsigned txlen = iob->Length - iob->Done;
	unsigned packet_cnt = txlen / max_packet_length;
	unsigned sp = txlen - (packet_cnt * max_packet_length);

	if (ep_num == 0 && packet_cnt > 3)
	{
		// ep0 can only do 3 packets 
		packet_cnt = 3;
		sp = 0;
	}

	// NOTE: zero-length packet and packet_cnt!=1 is not supported by core!
	if (packet_cnt == 0)
	{
		packet_cnt = 1;
		txlen = sp;
		_in_ep_last_packet[ep_num] = sp;
	}
	else 
	{
		txlen = packet_cnt * max_packet_length;		// FIXME: we are doing full packets first (only?)
		_in_ep_last_packet[ep_num] = max_packet_length;
	}

	_in_ep_rem_length[ep_num] = txlen;
	unsigned dieptsiz = (1 << USB_OTG_DIEPTSIZ_MULCNT_Pos) | (packet_cnt << USB_OTG_DIEPTSIZ_PKTCNT_Pos) 
		| (txlen << USB_OTG_DIEPTSIZ_XFRSIZ_Pos);

	if (txlen == 0)
	{
		iob->Status = USB_IOSTA_IN_COMPLETE;
	}

	if (ep_num == 0)
	{
		ASSERT(packet_cnt < 4, KERNEL_ERROR_NOT_SUPPORTED);
		ASSERT(txlen < 128, KERNEL_ERROR_NOT_SUPPORTED);
	}
	otg_device->DIEP[ep_num].TSIZ = dieptsiz;
#ifdef USB_HS_ENABLE_DMA
	otg_device->DIEP[ep_num].DMA = (unsigned)(iob->Data + iob->Done);
	iob->Done += txlen;
	iob->Status = USB_IOSTA_IN_COMPLETE;
#endif
	unsigned ctl = otg_device->DIEP[ep_num].CTL & ~USB_OTG_DIEPCTL_EPDIS;
	otg_device->DIEP[ep_num].CTL = ctl | USB_OTG_DIEPCTL_EPENA | USB_OTG_DIEPCTL_CNAK;

#ifndef USB_HS_ENABLE_DMA
	if (txlen != 0)
	{
		otg_device->DIEPEMPMSK |= (1 << ep_num);
	}
#endif
}

void hal_usbd_prepare_in_ep(unsigned ep_num, usb_io_buffer_t *iob)
{
	ASSERT(ep_num < USB_DEV_EP_COUNT, KERNEL_ERROR_KERNEL_PANIC);
	ASSERT(iob != nullptr, KERNEL_ERROR_NULL_POINTER);
	ASSERT(iob->Event != nullptr, KERNEL_ERROR_NULL_POINTER);

	if (_ep_in_io[ep_num] != nullptr)
	{
		ASSERT(iob == _ep_in_io[ep_num], KERNEL_ERROR_KERNEL_PANIC);
		_disable_in_ep(ep_num);
		bool done = exos_event_wait(iob->Event, 10);
		ASSERT(done, KERNEL_ERROR_KERNEL_PANIC);
	}

	iob->Done = 0;
	iob->Status = USB_IOSTA_IN_WAIT;

	_ep_in_io[ep_num] = iob;
	_prepare_in_ep(ep_num, iob);

	if (ep_num != 0)
	{
		unsigned mask  = (1 << USB_OTG_DAINTMSK_IEPM_Pos) << ep_num;
		otg_device->DAINTMSK |= mask;
	}
}

void hal_usbd_stall_out_ep(unsigned ep_num, bool stall)
{
	ASSERT(ep_num < USB_DEV_EP_COUNT, KERNEL_ERROR_KERNEL_PANIC);
	if (stall)
	{
		otg_device->DOEP[ep_num].CTL |= USB_OTG_DOEPCTL_STALL;
	}
	else
	{
		otg_device->DOEP[ep_num].CTL &= ~USB_OTG_DOEPCTL_STALL;
	}
}

void hal_usbd_stall_in_ep(unsigned ep_num, bool stall)
{
	ASSERT(ep_num < USB_DEV_EP_COUNT, KERNEL_ERROR_KERNEL_PANIC);

	if (stall)
	{
		otg_device->DIEP[ep_num].CTL |= USB_OTG_DIEPCTL_EPDIS | USB_OTG_DIEPCTL_STALL;	
	}
	else if (ep_num != 0)
	{
		// NOTE: ep0 stall can only be cleared by core, 
		// it is cleared when when a setup packet is received 

		usb_io_buffer_t *iob = _ep_in_io[ep_num];
		if (iob != nullptr)
		{
			iob->Done = 0;
			iob->Status = USB_IOSTA_IN_STALL;
		}
		unsigned ctl = otg_device->DIEP[ep_num].CTL & ~USB_OTG_DIEPCTL_STALL;
		switch ((ctl & USB_OTG_DIEPCTL_EPTYP) >> USB_OTG_DIEPCTL_EPTYP_Pos)
		{
			case USB_TT_BULK:	ctl |= USB_OTG_DIEPCTL_SD0PID_SEVNFRM;	break;
			default:	kernel_panic(KERNEL_ERROR_NOT_IMPLEMENTED);
		}
		otg_device->DIEP[ep_num].CTL = ctl | USB_OTG_DIEPCTL_SNAK;
	}
}

bool hal_usbd_is_halted(unsigned ep_num)
{
	if (ep_num & 0x80)	// in_ep
	{
		ASSERT((ep_num & 0xf) < USB_DEV_EP_COUNT, KERNEL_ERROR_KERNEL_PANIC);
		unsigned diepctl = otg_device->DIEP[ep_num & 0x7].CTL;
		return diepctl & USB_OTG_DIEPCTL_STALL;
	}
	else
	{
		ASSERT((ep_num & 0xf) < USB_DEV_EP_COUNT, KERNEL_ERROR_KERNEL_PANIC);
		unsigned doepctl = otg_device->DOEP[ep_num & 0x7].CTL;
		return doepctl & USB_OTG_DOEPCTL_STALL;
	}
}



static void _read_fifo(unsigned ep_num, unsigned char *ptr, unsigned bcnt)
{
	unsigned dataw;
	unsigned rem = bcnt;
	while (rem >= 4)
	{
		dataw = *otg_fifo[ep_num];
		*ptr++ = (unsigned char)dataw;
		*ptr++ = (unsigned char)(dataw >> 8);
		*ptr++ = (unsigned char)(dataw >> 16);
		*ptr++ = (unsigned char)(dataw >> 24);
		rem -= 4;
	}
	if (rem != 0)
	{
		dataw = *otg_fifo[ep_num];
		switch(rem)
		{
			case 1:	*ptr++ = (unsigned char)dataw;
					break;
			case 2:	*ptr++ = (unsigned char)dataw;
					*ptr++ = (unsigned char)(dataw >> 8);
					break;
			case 3:	*ptr++ = (unsigned char)dataw;
					*ptr++ = (unsigned char)(dataw >> 8);
					*ptr++ = (unsigned char)(dataw >> 16);
					break;
		}
	}
}

#ifndef USB_HS_ENABLE_DMA
static void _handle_rxf()
{
	static unsigned _fail = 0;

	unsigned stsp = otg_global->GRXSTSP;
	unsigned pktsts = (stsp & USB_OTG_GRXSTSP_PKTSTS_Msk) >> USB_OTG_GRXSTSP_PKTSTS_Pos;
	unsigned bcnt = (stsp & USB_OTG_GRXSTSP_BCNT_Msk) >> USB_OTG_GRXSTSP_BCNT_Pos;
	unsigned ep_num = (stsp & USB_OTG_GRXSTSP_EPNUM_Msk) >> USB_OTG_GRXSTSP_EPNUM_Pos;
	ASSERT(ep_num < USB_DEV_EP_COUNT, KERNEL_ERROR_KERNEL_PANIC);

	usb_io_buffer_t *iob = nullptr;
	switch(pktsts)
	{
		case 0b0100:		// SETUP-OUT complete
			// NOTE: core sets this code when a setup-out is completed and PKTCNT = 0
			ASSERT(ep_num == 0, KERNEL_ERROR_KERNEL_PANIC);
			ASSERT(bcnt == 0, KERNEL_ERROR_KERNEL_PANIC);
			iob = _ep_setup_io;
			if (iob->Status == USB_IOSTA_OUT_WAIT)
			{
				iob->Status = USB_IOSTA_DONE;
				exos_event_set(iob->Event);
			}
			// NOTE: as bcnt == 0, no fifo read following
			break;
		case 0b0110:		// SETUP-OUT
			ASSERT(ep_num == 0, KERNEL_ERROR_KERNEL_PANIC);
			// FIXME: maybe we should use a private buffer for back-to-back setup packets 
			// in order to receive more than one packet (3, per spec and core support) 
			// and get last one later, when core reports setup-out complete
			iob = _ep_setup_io;
			break;
		case 0b0011:		// DATA-OUT complete
			iob = _ep_out_io[ep_num];
			if (iob != nullptr)
			{
				ASSERT(iob->Status == USB_IOSTA_OUT_WAIT, KERNEL_ERROR_KERNEL_PANIC);
				iob->Status = USB_IOSTA_DONE;
				exos_event_set(iob->Event);
			}
			break;
		case 0b0010:		// OUT data_packet
			iob = _ep_out_io[ep_num];
			if (iob != nullptr)
			{
				// nothing 
			}
			else if (ep_num == 0)
			{
				iob = _ep_in_io[0];	// ongoing setup data_in?
				if (iob != nullptr)
				{
					// cancel data_in phase
					otg_device->DIEP[0].CTL |= USB_OTG_DIEPCTL_EPDIS;	// will trigger an IEP_DIS interrupt
				}
			}
#ifdef DEBUG
			// NOTE: unhandled non-control data out
			else kernel_panic(KERNEL_ERROR_KERNEL_PANIC);
#endif
			break;
		default:
			kernel_panic(KERNEL_ERROR_NOT_IMPLEMENTED);
			break;
	}

	if (iob != nullptr)
	{
		if (bcnt != 0)
		{
			_read_fifo(ep_num, (unsigned char *)iob->Data + iob->Done, bcnt);
			iob->Done += bcnt;
		}
	}
	else 
	{
		ASSERT(bcnt == 0, KERNEL_ERROR_NULL_POINTER);
	}
}
#endif

static void _ep_out_handler(unsigned ep)
{
	ASSERT(ep < USB_DEV_EP_COUNT, KERNEL_ERROR_KERNEL_PANIC);

	unsigned intreq = otg_device->DOEP[ep].INT;
	intreq &= otg_device->DOEPMSK;

	usb_io_buffer_t *iob = nullptr;
	unsigned int_handled = 0;
	if (intreq & USB_OTG_DOEPINT_XFRC)
	{
#ifdef USB_HS_ENABLE_DMA
		iob = _ep_out_io[ep];
		if (iob != nullptr)
		{
			ASSERT(iob->Status == USB_IOSTA_OUT_WAIT, KERNEL_ERROR_KERNEL_PANIC);
			unsigned xfrsiz = otg_device->DOEP[ep].TSIZ & USB_OTG_DOEPTSIZ_XFRSIZ;
			iob->Done = iob->Length - xfrsiz;
			iob->Status = USB_IOSTA_DONE;
			exos_event_set(iob->Event);
		}
		else
		{
			ASSERT(ep == 0, KERNEL_ERROR_NOT_IMPLEMENTED);
		}
#endif
		int_handled = USB_OTG_DOEPINT_XFRC;
	}
	else if (intreq & USB_OTG_DOEPINT_STUP)	// setup(out) complete
	{
#ifdef USB_HS_ENABLE_DMA
		iob = _ep_setup_io;
		if (iob->Status == USB_IOSTA_OUT_WAIT)
		{
			iob->Status = USB_IOSTA_DONE;
			exos_event_set(iob->Event);
		}
#endif
		int_handled = USB_OTG_DOEPINT_STUP;
	}

	ASSERT(int_handled != 0, KERNEL_ERROR_KERNEL_PANIC);
	// clear int
	otg_device->DOEP[ep].INT = int_handled;
}

static void _ep_in_handler(unsigned ep)
{
	static unsigned _debug1 = 0;

	ASSERT(ep < USB_DEV_EP_COUNT, KERNEL_ERROR_KERNEL_PANIC);
	usb_io_buffer_t *iob = _ep_in_io[ep];
	unsigned ep_mask = 1 << ep;

	unsigned intreq = otg_device->DIEP[ep].INT;
	unsigned intmsk = otg_device->DIEPMSK;
	if (otg_device->DIEPEMPMSK & ep_mask) intmsk |= USB_OTG_DIEPINT_TXFE;
	intreq &= intmsk;

	unsigned int_handled = 0;
	if (intreq & USB_OTG_DIEPINT_TXFE)	// handle tx-fifo
	{
		ASSERT(iob != nullptr, KERNEL_ERROR_NULL_POINTER);
		ASSERT(iob->Status != USB_IOSTA_DONE, KERNEL_ERROR_KERNEL_PANIC);
		_write_fifo(ep, iob);

		int_handled = USB_OTG_DIEPINT_TXFE; // NOTE: this is not needed, DIEPINTx won't clear flag
	}
	else if (intreq & USB_OTG_DIEPINT_XFRC)	// transfer complete
	{
		// NOTE: if data-in was canceled (in-ep disabled), we should not get here
		ASSERT(iob != nullptr, KERNEL_ERROR_NULL_POINTER);
		ASSERT(_ep_max_length[ep] != 0, KERNEL_ERROR_KERNEL_PANIC);

		if (_in_ep_last_packet[ep] == _ep_max_length[ep] && 
			((iob->Flags & USB_IOF_SHORT_PACKET_END) || iob->Done < iob->Length))
		{
			if (ep != 0)
				_debug1++;

			iob->Status = USB_IOSTA_IN_WAIT;
			_prepare_in_ep(ep, iob);
		}
		else
		{
			if (iob->Status == USB_IOSTA_IN_COMPLETE)
			{
				ASSERT(iob->Done == iob->Length, KERNEL_ERROR_KERNEL_PANIC);
				iob->Status = USB_IOSTA_DONE;
				exos_event_set(iob->Event);
			}
			else
			{
				// caller may have changed iob->Status to UNKNOWN to cancel response
				ASSERT(iob->Status == USB_IOSTA_UNKNOWN, KERNEL_ERROR_KERNEL_PANIC);
				// FIXME: allow caller to cancel the iob properly
			}
			_ep_in_io[ep] = nullptr;
		}

        int_handled = USB_OTG_DIEPINT_XFRC;
	}
	else if (intreq & USB_OTG_DIEPINT_EPDISD)	// ep disabled
	{
		if (iob != nullptr)
		{
			ASSERT(iob->Status != USB_IOSTA_DONE, KERNEL_ERROR_KERNEL_PANIC);

			// flush TX fifo
			otg_global->GRSTCTL = (ep << USB_OTG_GRSTCTL_TXFNUM_Pos) | USB_OTG_GRSTCTL_TXFFLSH;
			while(otg_global->GRSTCTL & USB_OTG_GRSTCTL_TXFFLSH);

			if (iob->Status == USB_IOSTA_IN_STALL)
			{
				_prepare_in_ep(ep, iob);
			}
			else
			{
				ASSERT(ep == 0, KERNEL_ERROR_NOT_IMPLEMENTED);
				iob->Status = USB_IOSTA_DONE;
				exos_event_set(iob->Event);
			
				_ep_in_io[ep] = nullptr;
			}
		}
		else
		{
			ASSERT(ep == 0, KERNEL_ERROR_NULL_POINTER);
		}

		int_handled = USB_OTG_DIEPINT_EPDISD;
	}

	ASSERT(int_handled != 0, KERNEL_ERROR_KERNEL_PANIC);
	// clear int
	otg_device->DIEP[ep].INT = int_handled;
} 


static void _reset(unsigned speed)
{
	if (speed == 0)	// reset received
	{
		otg_global->GINTMSK &= ~(USB_OTG_GINTMSK_RXFLVLM | USB_OTG_GINTMSK_NPTXFEM);
		for(unsigned ep = 0; ep < USB_DEV_EP_COUNT; ep++)
			otg_device->DOEP[ep].CTL |= USB_OTG_DOEPCTL_SNAK;

		// unmask ints for control 0 in/out
		otg_device->DAINTMSK = (1 << USB_OTG_DAINTMSK_OEPM_Pos) | (1 << USB_OTG_DAINTMSK_IEPM_Pos); 
		otg_device->DOEPMSK = USB_OTG_DOEPMSK_STUPM | USB_OTG_DOEPMSK_XFRCM;
		otg_device->DIEPMSK = USB_OTG_DIEPMSK_XFRCM | USB_OTG_DIEPMSK_EPDM /*| USB_OTG_DIEPMSK_TOM*/;
		// NOTE: IEPx enable bits for TXFE are in a separate register (DIEPEMPMSK)
		otg_device->DIEPEMPMSK = 0;

		// NOTE: docs recommend to configure 3 back-to-back setup-packets, but we are using just 1 for now
		otg_device->DOEP[0].TSIZ = (1 << USB_OTG_DOEPTSIZ_STUPCNT_Pos) 
			| (64 << USB_OTG_DOEPTSIZ_XFRSIZ_Pos);
		// NOTE: max packet size is configured later, after speed is known (ENUMDNE int)
	}
	else
	{
		unsigned cfg;

		// configure g(lobal)usbcfg and max packet size for control ep according to speed
		switch(speed)
		{
			case 3:	// full-speed
				// NOTE: usb turnaround time is 5 for ahbclk = 48 Mhz
				cfg = otg_global->GUSBCFG & ~USB_OTG_GUSBCFG_TRDT;
				otg_global->GUSBCFG = cfg | USB_OTG_GUSBCFG_PHYSEL | (5 << USB_OTG_GUSBCFG_TRDT_Pos);

				// NOTE: initial value for full-speed is 64 bytes
				otg_device->DIEP[0].CTL = (0 << USB_OTG_DIEPCTL_MPSIZ_Pos); // DIEPCTL0.MPSIZ[1:0] 0 = 64 bytes
				otg_device->DOEP[0].CTL = USB_OTG_DOEPCTL_EPENA | USB_OTG_DOEPCTL_CNAK;	 // NOTE: size is taken from DIEPCTL0
				_ep_max_length[0] = 64;
				break;
			case 1:	// hs phy in full-speed
				// NOTE: usb turnaround time is 9 for ahbclk = 48 Mhz
				cfg = otg_global->GUSBCFG & ~USB_OTG_GUSBCFG_TRDT;
				otg_global->GUSBCFG = cfg | (9 << USB_OTG_GUSBCFG_TRDT_Pos);

				// NOTE: initial value for full-speed is 64 bytes
				otg_device->DIEP[0].CTL = (0 << USB_OTG_DIEPCTL_MPSIZ_Pos); // DIEPCTL0.MPSIZ[1:0] 0 = 64 bytes
				otg_device->DOEP[0].CTL = USB_OTG_DOEPCTL_EPENA | USB_OTG_DOEPCTL_CNAK;	 // NOTE: size is taken from DIEPCTL0
				_ep_max_length[0] = 64;
				break;
			default:	kernel_panic(KERNEL_ERROR_NOT_IMPLEMENTED);
		}

        otg_global->GINTMSK |= USB_OTG_GINTMSK_RXFLVLM
			| USB_OTG_GINTMSK_IEPINT | USB_OTG_GINTMSK_OEPINT;
	}
}


void __usb_otg_hs_device_irq_handler()
{
	static unsigned _rst_count = 0;
	static unsigned _edn_count = 0;
	static unsigned _rxf_count = 0;
	static unsigned _iep_count = 0;
	static unsigned _oep_count = 0;

	static bool _reset_state = false;
	unsigned intreq = otg_global->GINTSTS & otg_global->GINTMSK;
	unsigned daint = otg_device->DAINT & otg_device->DAINTMSK;

	if (intreq & USB_OTG_GINTSTS_RXFLVL)	// rx-fifo
	{
#ifdef USB_HS_ENABLE_DMA
		kernel_panic(KERNEL_ERROR_KERNEL_PANIC);
#else
		_handle_rxf();
#endif	
		_rxf_count++;
	}
	else if (intreq & USB_OTG_GINTSTS_IEPINT)
	{
		for (unsigned ep = 0; ep < USB_DEV_EP_COUNT; ep++)
		{
			unsigned mask = 1 << ep;
			if (daint & mask)
				_ep_in_handler(ep);
		}

		_iep_count++;
	}
	else if (intreq & USB_OTG_GINTSTS_OEPINT)
	{
		for (unsigned ep = 0; ep < USB_DEV_EP_COUNT; ep++)
		{
			unsigned mask = 0x10000 << ep;
			if (daint & mask)
				_ep_out_handler(ep);
		}

		_oep_count++;
	}
	else if (intreq & USB_OTG_GINTSTS_USBRST)
	{
		otg_global->GINTSTS = USB_OTG_GINTSTS_USBRST;	// clear int
		_reset(0);
		if (!_reset_state)
		{
			_reset_state = true;
//			_set_connect_status(USB_DEVSTA_DETACHED);
		}
		otg_device->DCFG = otg_device->DCFG & ~(USB_OTG_DCFG_PFIVL_Msk | USB_OTG_DCFG_DAD_Msk | USB_OTG_DCFG_NZLSOHSK_Msk)
//			| USB_OTG_DCFG_NZLSOHSK	// auto-stall out status stage
#ifdef STM32_USB_HS_ULPI
			| (1 << USB_OTG_DCFG_DSPD_Pos); // hs phy in full speed
#else
			| (3 << USB_OTG_DCFG_DSPD_Pos); // full speed
#endif
		_rst_count++;
	}
	else if (intreq & USB_OTG_GINTSTS_ENUMDNE)
	{
		// read connected speed
   		unsigned speed = (otg_device->DSTS & USB_OTG_DSTS_ENUMSPD_Msk) >> USB_OTG_DSTS_ENUMSPD_Pos;
		if (_reset_state)
		{
			_reset(speed);
            otg_global->GINTSTS = USB_OTG_GINTSTS_USBRST;	// clear int
			_reset_state = false;

//			_set_connect_status(USB_DEVSTA_ATTACHED);
		}

		otg_global->GINTSTS = USB_OTG_GINTSTS_ENUMDNE;	// clear int

		_edn_count++;
	}
	else 
	{
		ASSERT(intreq == 0, KERNEL_ERROR_KERNEL_PANIC);
	}
}

