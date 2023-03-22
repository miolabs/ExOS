#include "usb_fs_device.h"
#include "cpu.h"
#include <support/usb/device_hal.h>
#include <kernel/panic.h>
#include <string.h>

#if defined STM32F469xx
#define USB_DEV_EP_COUNT 5	// FIXME: maybe 6!
#else
#define USB_DEV_EP_COUNT 4
#endif

static usb_otg_crs_global_t * const otg_global = (usb_otg_crs_global_t *)(USB_OTG_FS_BASE + 0x000);
static usb_otg_crs_device_t * const otg_device = (usb_otg_crs_device_t *)(USB_OTG_FS_BASE + 0x800);
static usb_otg_crs_power_t * const otg_power = (usb_otg_crs_power_t *) (USB_OTG_FS_BASE + 0xe00);
static volatile uint32_t * const otg_fifo[USB_DEV_EP_COUNT] = {
	 (volatile uint32_t *)(USB_OTG_FS_BASE + 0x1000), (volatile uint32_t *)(USB_OTG_FS_BASE + 0x2000), 
	 (volatile uint32_t *)(USB_OTG_FS_BASE + 0x3000), (volatile uint32_t *)(USB_OTG_FS_BASE + 0x4000) }; 


static event_t _connected;
static usb_io_buffer_t *_ep_setup_io = nullptr;
static usb_io_buffer_t *_ep_out_io[USB_DEV_EP_COUNT];
static usb_io_buffer_t *_ep_in_io[USB_DEV_EP_COUNT];
static unsigned char _ep_max_length[USB_DEV_EP_COUNT];
static unsigned char _in_ep_last_packet[USB_DEV_EP_COUNT];

// NOTE: total fifo size for OTG_FS is 320 words (1280 bytes)
#define FIFO_RX_WORDS	240		// words shared for all OUT EP
#define FIFO_TX_WORDS	16		// words for each IN EP (min 16) 

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
	usb_otg_fs_initialize();

#ifndef USB_FS_ENABLE_ID
	otg_global->GUSBCFG &= ~(USB_OTG_GUSBCFG_HNPCAP | USB_OTG_GUSBCFG_SRPCAP);
	otg_global->GUSBCFG |= USB_OTG_GUSBCFG_FDMOD;	// force usb-device mode
#endif

	otg_device->DCTL = USB_OTG_DCTL_SDIS;	// disable until app calls connect()

#ifdef USB_FS_ENABLE_VBUS 
	#ifdef USB_OTG_GCCFG_VBDEN
		otg_global->GCCFG = USB_OTG_GCCFG_PWRDWN | USB_OTG_GCCFG_VBDEN;	
	#else
		otg_global->GCCFG = USB_OTG_GCCFG_PWRDWN | USB_OTG_GCCFG_VBUSBSEN;	
	#endif
#else
	#ifdef USB_OTG_GCCFG_NOVBUSSENS
		otg_global->GCCFG = USB_OTG_GCCFG_PWRDWN | USB_OTG_GCCFG_NOVBUSSENS;	
	#else
		#warning "USB_OTG_GCCFG_NOVBUSSENS not defined"
		otg_global->GCCFG = USB_OTG_GCCFG_PWRDWN | (1<<21);
	#endif
#endif

	usb_otg_fs_notify(USB_HOST_ROLE_DEVICE);
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
		// TODO
		kernel_panic(KERNEL_ERROR_NOT_IMPLEMENTED);
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
	usb_otg_fs_notify(USB_HOST_ROLE_DEVICE_CLOSING);
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
	switch(ep_num)
	{
		case 0:	otg_device->DOEPCTL0 = doepctl;	break;
		case 1:	otg_device->DOEPCTL1 = doepctl;	break;
		case 2:	otg_device->DOEPCTL2 = doepctl;	break;
		case 3:	otg_device->DOEPCTL3 = doepctl;	break;
		case 4:	otg_device->DOEPCTL4 = doepctl;	break;
		default:	
			kernel_panic(KERNEL_ERROR_NOT_IMPLEMENTED);
	}
	_ep_max_length[ep_num] = max_packet_size;
}

static void _configure_in_ep(unsigned ep_num, usb_transfer_type_t tt, unsigned max_packet_size)
{
	ASSERT(ep_num < USB_DEV_EP_COUNT, KERNEL_ERROR_KERNEL_PANIC);
	ASSERT(max_packet_size < 2048, KERNEL_ERROR_NOT_SUPPORTED);
	unsigned diepctl = (ep_num << USB_OTG_DIEPCTL_TXFNUM_Pos) | (tt << USB_OTG_DIEPCTL_EPTYP_Pos) | 
		USB_OTG_DIEPCTL_USBAEP | (max_packet_size & USB_OTG_DIEPCTL_MPSIZ_Msk);
	switch(ep_num)
	{
		case 0:	otg_device->DIEPCTL0 = diepctl;	break;
		case 1:	otg_device->DIEPCTL1 = diepctl;	break;
		case 2:	otg_device->DIEPCTL2 = diepctl;	break;
		case 3:	otg_device->DIEPCTL3 = diepctl;	break;
		case 4:	otg_device->DIEPCTL4 = diepctl;	break;
		default:	
			kernel_panic(KERNEL_ERROR_NOT_IMPLEMENTED);
	}
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


void hal_usbd_stall_out_ep(unsigned ep_num, bool stall)
{
	ASSERT(ep_num < USB_DEV_EP_COUNT, KERNEL_ERROR_KERNEL_PANIC);
	if (stall)
	{
		switch(ep_num)
		{
			case 0:	otg_device->DOEPCTL0 |= USB_OTG_DOEPCTL_STALL;	break;
			case 1:	otg_device->DIEPCTL1 |= USB_OTG_DOEPCTL_STALL;	break;
			case 2:	otg_device->DIEPCTL2 |= USB_OTG_DOEPCTL_STALL;	break;
			case 3:	otg_device->DIEPCTL3 |= USB_OTG_DOEPCTL_STALL;	break;
			case 4:	otg_device->DIEPCTL4 |= USB_OTG_DOEPCTL_STALL;	break;
		}
	}
	else
	{
		kernel_panic(KERNEL_ERROR_NOT_IMPLEMENTED);
	}
}

void hal_usbd_stall_in_ep(unsigned ep_num, bool stall)
{
	ASSERT(ep_num < USB_DEV_EP_COUNT, KERNEL_ERROR_KERNEL_PANIC);
	switch(ep_num)
	{
		case 0:	
			if (stall)
				otg_device->DIEPCTL0 |= USB_OTG_DIEPCTL_STALL;	
			else
			{
				// NOTE: ep0 stall can only be cleared by core, 
				// it is cleared when when a setup packet is received 
			}
			break;
		case 1:	
			if (stall)
				otg_device->DIEPCTL1 |= USB_OTG_DIEPCTL_STALL;
			else
			{
				otg_device->DIEPCTL1 &= ~USB_OTG_DIEPCTL_STALL;
				otg_device->DIEPCTL1 |= USB_OTG_DIEPCTL_SD0PID_SEVNFRM | USB_OTG_DIEPCTL_SNAK;
			}
			break;
		case 2:	
			if (stall)
				otg_device->DIEPCTL2 |= USB_OTG_DIEPCTL_STALL;
			else
			{
				otg_device->DIEPCTL2 &= ~USB_OTG_DIEPCTL_STALL;
				otg_device->DIEPCTL2 |= USB_OTG_DIEPCTL_SD0PID_SEVNFRM | USB_OTG_DIEPCTL_SNAK;
			}
			break;
		case 3:	
			if (stall)
				otg_device->DIEPCTL3 |= USB_OTG_DIEPCTL_STALL;
			else
			{
				otg_device->DIEPCTL3 &= ~USB_OTG_DIEPCTL_STALL;
				otg_device->DIEPCTL3 |= USB_OTG_DIEPCTL_SD0PID_SEVNFRM | USB_OTG_DIEPCTL_SNAK;
			}
			break;
		case 4:	
			if (stall)
				otg_device->DIEPCTL4 |= USB_OTG_DIEPCTL_STALL;
			else
			{
				otg_device->DIEPCTL4 &= ~USB_OTG_DIEPCTL_STALL;
				otg_device->DIEPCTL4 |= USB_OTG_DIEPCTL_SD0PID_SEVNFRM | USB_OTG_DIEPCTL_SNAK;
			}
			break;
		default:
			kernel_panic(KERNEL_ERROR_NOT_IMPLEMENTED);
	}
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
	otg_device->DOEPCTL0 = USB_OTG_DOEPCTL_EPENA | USB_OTG_DOEPCTL_CNAK;	 // NOTE: size is taken from DIEPCTL0
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

	switch(ep_num)
	{
		case 0:	otg_device->DOEPTSIZ0 = siz;
				otg_device->DOEPCTL0 |= USB_OTG_DOEPCTL_EPENA | USB_OTG_DOEPCTL_CNAK;
				break;
		case 1:	otg_device->DOEPTSIZ1 = siz;
				otg_device->DOEPCTL1 |= USB_OTG_DOEPCTL_EPENA | USB_OTG_DOEPCTL_CNAK;
				break;
		case 2:	otg_device->DOEPTSIZ2 = siz;
				otg_device->DOEPCTL2 |= USB_OTG_DOEPCTL_EPENA | USB_OTG_DOEPCTL_CNAK;
				break;
		case 3:	otg_device->DOEPTSIZ3 = siz;
				otg_device->DOEPCTL3 |= USB_OTG_DOEPCTL_EPENA | USB_OTG_DOEPCTL_CNAK;
				break;
		case 4:	otg_device->DOEPTSIZ4 = siz;
				otg_device->DOEPCTL4 |= USB_OTG_DOEPCTL_EPENA | USB_OTG_DOEPCTL_CNAK;
				break;
		default:
			kernel_panic(KERNEL_ERROR_NOT_IMPLEMENTED);
	}
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
}


static void _disable_in_ep(unsigned ep_num)
{
	ASSERT(ep_num < USB_DEV_EP_COUNT, KERNEL_ERROR_KERNEL_PANIC);
	switch(ep_num)
	{
		case 0:	otg_device->DIEPCTL0 |= USB_OTG_DIEPCTL_EPDIS;	break;
		case 1:	otg_device->DIEPCTL1 |= USB_OTG_DIEPCTL_EPDIS;	break;
		case 2:	otg_device->DIEPCTL2 |= USB_OTG_DIEPCTL_EPDIS;	break;
		case 3:	otg_device->DIEPCTL3 |= USB_OTG_DIEPCTL_EPDIS;	break;
		case 4:	otg_device->DIEPCTL4 |= USB_OTG_DIEPCTL_EPDIS;	break;
		default:
			kernel_panic(KERNEL_ERROR_NOT_IMPLEMENTED);
	}
	// NOTE: will trigger an ep disabled interrupt
}

static void _write_fifo(unsigned ep_num, usb_io_buffer_t *iob, unsigned txlen)
{
	static volatile unsigned _busy = false;
	ASSERT(!_busy, KERNEL_ERROR_KERNEL_PANIC);
	_busy = true;

	ASSERT(ep_num < USB_DEV_EP_COUNT, KERNEL_ERROR_KERNEL_PANIC);
	ASSERT(iob != nullptr, KERNEL_ERROR_NULL_POINTER);
	unsigned rem = txlen;
	unsigned char *ptr = (unsigned char *)iob->Data + iob->Done; 

	unsigned short avail_words;
	switch(ep_num)
	{
		case 0:	avail_words = otg_device->DTXFSTS0;	break;
		case 1:	avail_words = otg_device->DTXFSTS1;	break;
		case 2:	avail_words = otg_device->DTXFSTS2;	break;
		case 3:	avail_words = otg_device->DTXFSTS3;	break;
		case 4:	avail_words = otg_device->DTXFSTS4;	break;
		default:
			kernel_panic(KERNEL_ERROR_NOT_IMPLEMENTED);
	}

	while(rem > 3 && avail_words != 0)
	{
		*otg_fifo[ep_num] = ptr[0] | (ptr[1] << 8) | (ptr[2] << 16) | (ptr[3] << 24);
		rem -= 4;
		ptr += 4;
		avail_words--;
	}
	if (rem < 4 && avail_words != 0)
	{
		switch(rem)
		{
			case 1:	*otg_fifo[ep_num] = ptr[0];	break;
			case 2: *otg_fifo[ep_num] = ptr[0] | (ptr[1] << 8);	break;
			case 3: *otg_fifo[ep_num] = ptr[0] | (ptr[1] << 8) | (ptr[2] << 16);	break;
		}
		rem = 0;
	}

	iob->Done += (txlen - rem);
	ASSERT(iob->Done <= iob->Length, KERNEL_ERROR_KERNEL_PANIC);

	if (rem == 0)
	{
		iob->Status = USB_IOSTA_IN_COMPLETE;
		// NOTE: core will generate a XFRCM (transfer complete) int later
	}
	else 
	{
		otg_device->DIEPEMPMSK |= (1 << ep_num);
//		kernel_panic(KERNEL_ERROR_NOT_IMPLEMENTED);	// fifo refill is not implemented
	}
	_busy = false;
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
		txlen = packet_cnt * max_packet_length;
	}

	// NOTE: sp==0 and packet_cnt!=1 is not supported by core!
	if (packet_cnt == 0)
	{
		packet_cnt = 1;
		_in_ep_last_packet[ep_num] = sp;
		txlen = sp;
	}
	else 
	{
		_in_ep_last_packet[ep_num] = max_packet_length;
		txlen = packet_cnt * max_packet_length;
	}

	unsigned dieptsiz = (1 << USB_OTG_DIEPTSIZ_MULCNT_Pos) | (packet_cnt << USB_OTG_DIEPTSIZ_PKTCNT_Pos) 
		| (txlen << USB_OTG_DIEPTSIZ_XFRSIZ_Pos);

	switch(ep_num)
	{
		case 0:
			ASSERT(packet_cnt < 4, KERNEL_ERROR_NOT_SUPPORTED);
			ASSERT(txlen < 127, KERNEL_ERROR_NOT_SUPPORTED);
			otg_device->DIEPTSIZ0 = dieptsiz;
			otg_device->DIEPCTL0 |= USB_OTG_DIEPCTL_EPENA | USB_OTG_DIEPCTL_CNAK; 
			break;
		case 1:
			otg_device->DIEPTSIZ1 = dieptsiz;
			otg_device->DIEPCTL1 |= USB_OTG_DIEPCTL_EPENA | USB_OTG_DIEPCTL_CNAK; 
			break;
		case 2:
			otg_device->DIEPTSIZ2 = dieptsiz;
			otg_device->DIEPCTL2 |= USB_OTG_DIEPCTL_EPENA | USB_OTG_DIEPCTL_CNAK; 
			break;
		case 3:
			otg_device->DIEPTSIZ3 = dieptsiz;
			otg_device->DIEPCTL3 |= USB_OTG_DIEPCTL_EPENA | USB_OTG_DIEPCTL_CNAK; 
			break;
		case 4:
			otg_device->DIEPTSIZ4 = dieptsiz;
			otg_device->DIEPCTL4 |= USB_OTG_DIEPCTL_EPENA | USB_OTG_DIEPCTL_CNAK; 
			break;
		default:
			kernel_panic(KERNEL_ERROR_NOT_IMPLEMENTED);
	}

	_write_fifo(ep_num, iob, txlen);	//// <---------------
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

static void _handle_rxf()
{
	static unsigned _fail = 0;

	otg_global->GINTMSK &= ~USB_OTG_GINTMSK_RXFLVLM;

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
					otg_device->DIEPCTL0 |= USB_OTG_DIEPCTL_EPDIS;	// will trigger an IEP_DIS interrupt
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

	otg_global->GINTMSK |= USB_OTG_GINTMSK_RXFLVLM;
}

static void _ep_out_handler(unsigned ep)
{
	ASSERT(ep < USB_DEV_EP_COUNT, KERNEL_ERROR_KERNEL_PANIC);

	unsigned intreq;
	switch(ep)
	{
		case 0:	intreq = otg_device->DOEPINT0;	break;
		case 1:	intreq = otg_device->DOEPINT1;	break;
		case 2:	intreq = otg_device->DOEPINT2;	break;
		case 3:	intreq = otg_device->DOEPINT3;	break;
		case 4:	intreq = otg_device->DOEPINT4;	break;
		default:
			kernel_panic(KERNEL_ERROR_NOT_IMPLEMENTED);
	}
	intreq &= otg_device->DOEPMSK;

	unsigned int_handled = 0;
	if (intreq & USB_OTG_DOEPINT_XFRC)
	{
		// NOTE: currently nothing
		int_handled = USB_OTG_DOEPINT_XFRC;
	}
	else if (intreq & USB_OTG_DOEPINT_STUP)	// setup(out) complete
	{
		// NOTE: currently nothing because the iob is notified in the rx-fifo handler
		int_handled = USB_OTG_DOEPINT_STUP;
	}

	if (int_handled != 0)
	{
    	// clear int
		switch(ep)
		{
			case 0:	otg_device->DOEPINT0 = int_handled;	break;
			case 1:	otg_device->DOEPINT1 = int_handled;	break;
			case 2:	otg_device->DOEPINT2 = int_handled;	break;
			case 3:	otg_device->DOEPINT3 = int_handled;	break;
			case 4:	otg_device->DOEPINT4 = int_handled;	break;			
		}
	}
	else
	{
		kernel_panic(KERNEL_ERROR_NOT_IMPLEMENTED);
	}
}

static void _ep_in_handler(unsigned ep)
{
	static unsigned _debug1 = 0;

	ASSERT(ep < USB_DEV_EP_COUNT, KERNEL_ERROR_KERNEL_PANIC);
	usb_io_buffer_t *iob = _ep_in_io[ep];
	unsigned ep_mask = 1 << ep;

	unsigned intreq;
	switch(ep)
	{
		case 0:	intreq = otg_device->DIEPINT0;	break;
		case 1:	intreq = otg_device->DIEPINT1;	break;
		case 2:	intreq = otg_device->DIEPINT2;	break;
		case 3:	intreq = otg_device->DIEPINT3;	break;
		case 4:	intreq = otg_device->DIEPINT4;	break;
		default:
			kernel_panic(KERNEL_ERROR_NOT_IMPLEMENTED);
	}
	unsigned intmsk = otg_device->DIEPMSK;
	if (otg_device->DIEPEMPMSK & ep_mask) intmsk |= USB_OTG_DIEPINT_TXFE;
	intreq &= intmsk;

	unsigned int_handled = 0;
	if (intreq & USB_OTG_DIEPINT_XFRC)	// transfer complete
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
			ASSERT(iob->Status == USB_IOSTA_IN_COMPLETE, KERNEL_ERROR_KERNEL_PANIC);
			ASSERT(iob->Done == iob->Length, KERNEL_ERROR_KERNEL_PANIC);
			iob->Status = USB_IOSTA_DONE;
			exos_event_set(iob->Event);

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

			iob->Status = USB_IOSTA_DONE;
			exos_event_set(iob->Event);

			_ep_in_io[ep] = nullptr;
		}
		else kernel_panic(KERNEL_ERROR_NULL_POINTER);

		int_handled = USB_OTG_DIEPINT_EPDISD;
	}
	else if (intreq & USB_OTG_DIEPINT_TXFE)
	{
		if (iob != nullptr && iob->Status != USB_IOSTA_DONE)
		{
			if (iob->Done < iob->Length)
			{
				_write_fifo(ep, iob, iob->Length - iob->Done);
			}
			else
			{
				// clear int mask
				otg_device->DIEPEMPMSK &= ~ep_mask;
			}
		}
		int_handled = USB_OTG_DIEPINT_TXFE; 
	}

	if (int_handled != 0)
	{
		// clear int
		switch(ep)
		{
			case 0:	otg_device->DIEPINT0 = int_handled;
			case 1:	otg_device->DIEPINT1 = int_handled;
			case 2:	otg_device->DIEPINT2 = int_handled;
			case 3:	otg_device->DIEPINT3 = int_handled;
			case 4:	otg_device->DIEPINT4 = int_handled;
		}
	}
	else 
	{
		kernel_panic(KERNEL_ERROR_NOT_IMPLEMENTED);
	}
} 


static void _reset(unsigned speed)
{
	if (speed == 0)	// reset received
	{
		otg_global->GINTMSK &= ~(USB_OTG_GINTMSK_RXFLVLM | USB_OTG_GINTMSK_NPTXFEM);

		otg_device->DOEPCTL0 |= USB_OTG_DOEPCTL_SNAK;
		otg_device->DOEPCTL1 |= USB_OTG_DOEPCTL_SNAK;
		otg_device->DOEPCTL2 |= USB_OTG_DOEPCTL_SNAK;
		otg_device->DOEPCTL3 |= USB_OTG_DOEPCTL_SNAK;
		otg_device->DOEPCTL4 |= USB_OTG_DOEPCTL_SNAK;

		// unmask ints for control 0 in/out
		otg_device->DAINTMSK = ( 1 << USB_OTG_DAINTMSK_OEPM_Pos) | (1 << USB_OTG_DAINTMSK_IEPM_Pos); 
		otg_device->DOEPMSK = USB_OTG_DOEPMSK_STUPM | USB_OTG_DOEPMSK_XFRCM;
		otg_device->DIEPMSK = USB_OTG_DIEPMSK_XFRCM | USB_OTG_DIEPMSK_EPDM /*| USB_OTG_DIEPMSK_TOM*/;
		// NOTE: IEPx enable bits for TXFE are in a sperate register (DIEPEMPMSK)
		otg_device->DIEPEMPMSK = 0;

		// setup data FIFOs
		// NOTE: we have 4 tx fifos and 4 endpoints so we are going to use each tx fifo for the each in ep (with the same number)
		unsigned word_ptr = 0;
		unsigned rxf_size = FIFO_RX_WORDS;
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
				case 0:	otg_global->DIEPTXF0 = txf_reg;	break;
				case 1:	otg_global->DIEPTXF1 = txf_reg;	break;
				case 2:	otg_global->DIEPTXF2 = txf_reg;	break;
				case 3:	otg_global->DIEPTXF3 = txf_reg;	break;
				case 4:	otg_global->DIEPTXF4 = txf_reg;	break;
				default:	kernel_panic(KERNEL_ERROR_NOT_SUPPORTED);
			}
		}

		// NOTE: docs recommend to configure 3 back-to-back setup-packets, but we are using just 1 for now
		otg_device->DOEPTSIZ0 = (1 << USB_OTG_DOEPTSIZ_STUPCNT_Pos) 
			| (8 << USB_OTG_DOEPTSIZ_XFRSIZ_Pos);
		// NOTE: max packet size is configured later, after speed is known (ENUMDNE int)
	}
	else
	{
		// configure max packet size for control ep according to speed
		switch(speed)
		{
			case 3:		// full-speed
				// NOTE: initial value for full-speed is 8 bytes, changed later 
				otg_device->DIEPCTL0 = (3 << USB_OTG_DIEPCTL_MPSIZ_Pos); // 3 = 8 bytes
				otg_device->DOEPCTL0 = USB_OTG_DOEPCTL_EPENA | USB_OTG_DOEPCTL_CNAK;	 // NOTE: size is taken from DIEPCTL0
				_ep_max_length[0] = 8;
				break;
			default:	kernel_panic(KERNEL_ERROR_NOT_IMPLEMENTED);
		}

        otg_global->GINTMSK |= USB_OTG_GINTMSK_RXFLVLM
			| USB_OTG_GINTMSK_IEPINT | USB_OTG_GINTMSK_OEPINT;
	}
}


void __usb_otg_device_irq_handler()
{
	static unsigned _rst_count = 0;
	static unsigned _edn_count = 0;
	static unsigned _rxf_count = 0;
	static unsigned _iep_count = 0;
	static unsigned _oep_count = 0;

	static bool _reset_state = false;
	unsigned intreq = otg_global->GINTMSK & otg_global->GINTSTS;
	unsigned daint = otg_device->DAINT & otg_device->DAINTMSK;

	if (intreq & USB_OTG_GINTSTS_RXFLVL)
	{
		_handle_rxf();
	
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
		_reset_state = true;

		otg_device->DCFG = otg_device->DCFG & ~(USB_OTG_DCFG_PFIVL_Msk | USB_OTG_DCFG_DAD_Msk | USB_OTG_DCFG_NZLSOHSK_Msk)
//			| USB_OTG_DCFG_NZLSOHSK	// auto-stall out status stage
			| (3 << USB_OTG_DCFG_DSPD_Pos); // full speed

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
		}

		otg_global->GINTSTS = USB_OTG_GINTSTS_ENUMDNE;	// clear int

		_edn_count++;
	}
	else 
	{
		ASSERT(intreq == 0, KERNEL_ERROR_KERNEL_PANIC);
	}
}

