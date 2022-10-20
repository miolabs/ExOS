#include "usb_hs_host_core.h"
#include "usb_otg_hs.h"
#include "cpu.h"
#include <usb/host.h>
#include <support/misc/pools.h>
#include <kernel/panic.h>
#include <kernel/verbose.h>

#ifdef STM32_USB_DEBUG
#define _verbose(level, ...)	verbose(level, "usb-hs-core", __VA_ARGS__)
#else
#define _verbose(level, ...)	{ /* nothing */ }
#endif

#define NUM_CHANNELS 8

static usb_otg_crs_global_t * const otg_global = (usb_otg_crs_global_t *)(USB_OTG_HS_BASE + 0x000);
static usb_otg_crs_host_t * const otg_host = (usb_otg_crs_host_t *)(USB_OTG_HS_BASE + 0x400);
static usb_otg_crs_power_t * const otg_power = (usb_otg_crs_power_t *) (USB_OTG_HS_BASE + 0xe00);
static volatile uint32_t * const otg_fifo[NUM_CHANNELS] = {
	 (volatile uint32_t *)(USB_OTG_HS_BASE + 0x1000), (volatile uint32_t *)(USB_OTG_HS_BASE + 0x2000), 
	 (volatile uint32_t *)(USB_OTG_HS_BASE + 0x3000), (volatile uint32_t *)(USB_OTG_HS_BASE + 0x4000), 
	 (volatile uint32_t *)(USB_OTG_HS_BASE + 0x5000), (volatile uint32_t *)(USB_OTG_HS_BASE + 0x6000), 
	 (volatile uint32_t *)(USB_OTG_HS_BASE + 0x7000), (volatile uint32_t *)(USB_OTG_HS_BASE + 0x8000) }; 

// FIXME: total words for OTG_FS is 320 (1280 bytes)
#define RX_WORDS 128
#define NPTX_WORDS 96
#define PTX_WORDS 96 

#ifndef NUM_ENDPOINTS
#define NUM_ENDPOINTS 6
#endif

static stm32_usbh_channel_t _ch_table[NUM_CHANNELS];
static stm32_usbh_ep_t *_ch2ep[NUM_CHANNELS];
static stm32_usbh_ep_t _ep_array[NUM_ENDPOINTS];
static pool_t _ep_pool;
static stm32_usbh_ep_t _common_control_ep;

static usb_host_controller_t *_hc = nullptr;
static enum { HPORT_DISABLED = 0, HPORT_POWERED, HPORT_RESET, HPORT_RESET_DONE, HPORT_SUSPEND, HPORT_READY } _port_state = HPORT_DISABLED;
static enum { HPORT_FULL_SPEED = 1, HPORT_LOW_SPEED = 2 } _port_speed;
static bool _role_switch_requested = false;

static dispatcher_t _port_dispatcher;
static void _port_callback(dispatcher_context_t *context, dispatcher_t *dispatcher);

void usb_hs_host_initialize(usb_host_controller_t *hc, dispatcher_context_t *context)
{
	ASSERT(hc != nullptr, KERNEL_ERROR_NULL_POINTER);
	ASSERT(_hc == nullptr, KERNEL_ERROR_NOT_ENOUGH_MEMORY);	// already in use
	_hc = hc;

	usb_otg_hs_initialize();

	otg_global->GUSBCFG |= USB_OTG_GUSBCFG_FHMOD;	// Force Host MODe
	while(!(otg_global->GINTSTS & USB_OTG_GINTSTS_CMOD)); 
	exos_thread_sleep(50);

	otg_power->PCGCCTL = 0;
	otg_host->HCFG = (otg_host->HCFG & ~USB_OTG_HCFG_FSLSPCS_Msk) 
		/*| (0x1 << USB_OTG_HCFG_FSLSPCS_Pos)*/ | USB_OTG_HCFG_FSLSS;	// FIXME

	otg_global->GRXFSIZ = RX_WORDS;
	otg_global->HNPTXFSIZ = (NPTX_WORDS << 16) | (RX_WORDS);
	otg_global->HPTXFSIZ = (PTX_WORDS << 16) | (RX_WORDS + NPTX_WORDS);

	otg_global->GAHBCFG |= USB_OTG_GAHBCFG_PTXFELVL | USB_OTG_GAHBCFG_TXFELVL;

	otg_global->GINTMSK |= USB_OTG_GINTMSK_RXFLVLM
		| USB_OTG_GINTMSK_SOFM
		| USB_OTG_GINTMSK_HCIM		// Host Channels Int Mask
		| USB_OTG_GINTMSK_PRTIM;	// host PoRT Int Mask

	otg_host->HPRT |= USB_OTG_HPRT_PPWR;
	_port_state = HPORT_POWERED;	// FIXME

	pool_create(&_ep_pool, (node_t *)_ep_array, sizeof(stm32_usbh_ep_t), NUM_ENDPOINTS);

	exos_dispatcher_create(&_port_dispatcher, &_hc->RootHubEvent, _port_callback, nullptr);
	exos_dispatcher_add(context, &_port_dispatcher, EXOS_TIMEOUT_NEVER);

	_role_switch_requested = false;
}

bool usb_hs_request_role_switch(usb_host_controller_t *hc)
{
	ASSERT(hc != nullptr, KERNEL_ERROR_NULL_POINTER);
	ASSERT(_hc == hc, KERNEL_ERROR_KERNEL_PANIC);
	
	if (_port_state == HPORT_READY)	// AKA idle
	{
		_role_switch_requested = true;
		_verbose(VERBOSE_DEBUG, "role-switch requested");	
		return true;
	}
	return false;
}

static void _port_callback(dispatcher_context_t *context, dispatcher_t *dispatcher)
{
	unsigned timeout = EXOS_TIMEOUT_NEVER;

	unsigned hprt = otg_host->HPRT;
	unsigned hprt_const = USB_OTG_HPRT_PENA | 
		(hprt & (USB_OTG_HPRT_PPWR | USB_OTG_HPRT_PRST | USB_OTG_HPRT_PSUSP | USB_OTG_HPRT_PRES));

	switch(_port_state)
	{
		case HPORT_POWERED:
			if (hprt & USB_OTG_HPRT_PCSTS)
			{
				verbose(VERBOSE_DEBUG, "usb-hs-roothub", "connect detected");
				
				exos_thread_sleep(50);	// at least 50ms before reset as of USB 2.0 spec

				_port_state = HPORT_RESET;
				otg_host->HPRT = hprt_const | USB_OTG_HPRT_PRST;
				timeout = 20;	// min 20ms
			}
			break;
		case HPORT_RESET:
			otg_host->HPRT = hprt_const & ~USB_OTG_HPRT_PRST;
			if (hprt & USB_OTG_HPRT_PCSTS)
			{
				_port_state = HPORT_RESET_DONE;
			}
			else
			{
				// NOTE usually caused by a board power glitch or very slow devices, increase wait before hub port reset
				verbose(VERBOSE_DEBUG, "usb-hs-roothub", "device disconnected during reset!");
				_port_state = HPORT_POWERED;
				timeout = 100;
			}
			break;
		case HPORT_RESET_DONE:
			_port_speed = (hprt & USB_OTG_HPRT_PSPD_Msk) >> USB_OTG_HPRT_PSPD_Pos;
			if (hprt & USB_OTG_HPRT_PENA)
			{
				switch (_port_speed)
				{
					case HPORT_FULL_SPEED:
						otg_host->HFIR = 48000;
						otg_host->HCFG = (1 << USB_OTG_HCFG_FSLSPCS_Pos);	// 48MHz core
						break;
					case HPORT_LOW_SPEED:
						otg_host->HFIR = 6000;
						otg_host->HCFG = (2 << USB_OTG_HCFG_FSLSPCS_Pos);	// 6MHz core
						break;
					default:
						kernel_panic(KERNEL_ERROR_KERNEL_PANIC);
				}
				
				usb_hs_host_flush_tx_fifo(0x10); // TXNUM_FLUSH_ALL
				usb_hs_host_flush_rx_fifo();

				_port_state = HPORT_READY;
				exos_thread_sleep(200);	// FIXME

				usb_host_device_t *child = usb_host_create_root_device(_hc, 0, 
					(_port_speed == HPORT_FULL_SPEED) ? USB_HOST_DEVICE_FULL_SPEED : USB_HOST_DEVICE_LOW_SPEED);
				if (child != nullptr)
					verbose(VERBOSE_COMMENT, "usb-hs-roothub", "child %04x/%04x added at port #%d", child->Vendor, child->Product, child->Port);
				else	
					verbose(VERBOSE_ERROR, "usb-hs-roothub", "device add failed");		
			}
			else 
			{
				verbose(VERBOSE_ERROR, "usb-hs-roothub", "not enabled after reset");
				_port_state = HPORT_POWERED;
				timeout = 500;
			}
			break;
		case HPORT_READY:
			if (hprt & USB_OTG_HPRT_PENA)
				kernel_panic(KERNEL_ERROR_KERNEL_PANIC);
			
			ASSERT(_hc->Devices != nullptr, KERNEL_ERROR_NULL_POINTER);
			usb_host_device_t *child = &_hc->Devices[0];	// single port
			verbose(VERBOSE_DEBUG, "usb-hs-roothub", "child %04x/%04x removing at port #%d", child->Vendor, child->Product, child->Port);
			usb_host_destroy_device(child);
			verbose(VERBOSE_COMMENT, "usb-hs-roothub", "child %04x/%04x removed", child->Vendor, child->Product);
			
			exos_thread_sleep(10);	// NOTE: avoid glitchy re-connect

			if (_role_switch_requested)
			{				
				usb_otg_hs_initialize();

				// remove root hub dispatchers
				exos_dispatcher_remove(context, &_port_dispatcher);
				_port_state = HPORT_DISABLED;
			}
			else _port_state = HPORT_POWERED;
			break;
		default:
			kernel_panic(KERNEL_ERROR_KERNEL_PANIC);
			break;
	}

	if (_port_state != HPORT_DISABLED)
	{
		exos_dispatcher_add(context, dispatcher, timeout);
	}
	else 
	{
		verbose(VERBOSE_DEBUG, "usb-hs-roothub", "host root-hub disabled");
		__usb_host_disabled(_hc);
	}
}

void usb_hs_host_port_reset()
{
	unsigned hport = otg_host->HPRT & ~USB_OTG_HPRT_PRST;
	otg_host->HPRT = hport | USB_OTG_HPRT_PRST;
	exos_thread_sleep(100);
	otg_host->HPRT = hport;
	exos_thread_sleep(20);
}

void usb_hs_host_flush_tx_fifo(unsigned num)
{
	otg_global->GRSTCTL = (num << USB_OTG_GRSTCTL_TXFNUM_Pos)
		| USB_OTG_GRSTCTL_TXFFLSH; 
	while(otg_global->GRSTCTL & USB_OTG_GRSTCTL_TXFFLSH);
	for(unsigned volatile i = 0; i < 12; i++);	// wait 3 phy clks
}

void usb_hs_host_flush_rx_fifo()
{
	otg_global->GRSTCTL = USB_OTG_GRSTCTL_RXFFLSH; 
	while(otg_global->GRSTCTL & USB_OTG_GRSTCTL_RXFFLSH);
	for(unsigned volatile i = 0; i < 12; i++);	// wait 3 phy clks
}

static void _open_channel(stm32_usbh_channel_t *ch, uint8_t addr, usb_host_device_speed_t speed, unsigned max_packet_size)
{
	ASSERT(ch != nullptr, KERNEL_ERROR_NULL_POINTER);
	ASSERT(ch->Index < 8, KERNEL_ERROR_KERNEL_PANIC);
	
	otg_host->HC[ch->Index].HCINT = -1;	// clear all pending interrupts

	switch(ch->EndpointType)
	{
		case USB_TT_CONTROL:
		case USB_TT_BULK:
			otg_host->HC[ch->Index].HCINTMSK = USB_OTG_HCINTMSK_XFRCM 
				| USB_OTG_HCINTMSK_NAKM | USB_OTG_HCINTMSK_STALLM 
				| USB_OTG_HCINTMSK_TXERRM | USB_OTG_HCINTMSK_DTERRM 
				| ((ch->Direction == USB_DEVICE_TO_HOST) ? USB_OTG_HCINTMSK_BBERRM : 0)
				| USB_OTG_HCINTMSK_CHHM;
			break;
		case USB_TT_INTERRUPT:
			otg_host->HC[ch->Index].HCINTMSK = USB_OTG_HCINTMSK_XFRCM 
				| USB_OTG_HCINTMSK_NAKM | USB_OTG_HCINTMSK_STALLM 
				| USB_OTG_HCINTMSK_TXERRM | USB_OTG_HCINTMSK_DTERRM 
				| USB_OTG_HCINTMSK_FRMORM | USB_OTG_HCINTMSK_BBERRM;
			break;
		default:
			kernel_panic(KERNEL_ERROR_NOT_IMPLEMENTED);
	}

	otg_host->HAINTMSK |= (1 << ch->Index);	 // enable top-level channel interrupt

	otg_host->HC[ch->Index].HCCHAR = ((addr & 0x7f) << USB_OTG_HCCHAR_DAD_Pos)
		| (ch->EndpointNumber << USB_OTG_HCCHAR_EPNUM_Pos)
		| ((ch->Direction == USB_DEVICE_TO_HOST) << USB_OTG_HCCHAR_EPDIR_Pos)
		| ((speed == USB_HOST_DEVICE_LOW_SPEED && _port_speed == HPORT_FULL_SPEED) ? USB_OTG_HCCHAR_LSDEV : 0)
		| (ch->EndpointType << USB_OTG_HCCHAR_EPTYP_Pos)
		| (max_packet_size << USB_OTG_HCCHAR_MPSIZ_Pos)
		| ((ch->Period != 0) << USB_OTG_HCCHAR_MC_Pos) 
		| ((ch->EndpointType == USB_TT_INTERRUPT) ? USB_OTG_HCCHAR_ODDFRM : 0);
}

static void _update_channel(stm32_usbh_channel_t *ch, uint8_t addr, usb_host_device_speed_t speed, unsigned max_packet_size)
{
	ASSERT(ch != nullptr, KERNEL_ERROR_NULL_POINTER);
	ASSERT(ch->Index < 8, KERNEL_ERROR_KERNEL_PANIC);

	otg_host->HC[ch->Index].HCCHAR = 
		(otg_host->HC[ch->Index].HCCHAR & ~(USB_OTG_HCCHAR_CHENA_Msk | USB_OTG_HCCHAR_DAD_Msk | USB_OTG_HCCHAR_LSDEV_Msk | USB_OTG_HCCHAR_MPSIZ_Msk))
		| ((addr & 0x7f) << USB_OTG_HCCHAR_DAD_Pos)
		| ((speed == USB_HOST_DEVICE_LOW_SPEED && _port_speed == HPORT_FULL_SPEED) ? USB_OTG_HCCHAR_LSDEV : 0)
		| (max_packet_size << USB_OTG_HCCHAR_MPSIZ_Pos);
}

static void _halt_channel(stm32_usbh_channel_t *ch)
{
	unsigned sts;
	unsigned cchar = otg_host->HC[ch->Index].HCCHAR & 
		~(USB_OTG_HCCHAR_CHENA_Msk | USB_OTG_HCCHAR_CHDIS_Msk);

	switch(ch->EndpointType)
	{
		case USB_TT_CONTROL:
		case USB_TT_BULK:
			sts = (otg_global->HNPTXSTS & USB_OTG_GNPTXSTS_NPTQXSAV_Msk) >> USB_OTG_GNPTXSTS_NPTQXSAV_Pos;
			break;
		default:
			kernel_panic(KERNEL_ERROR_NOT_IMPLEMENTED);
	}
	
	if (sts == 0)
	{
		usb_hs_host_flush_tx_fifo(ch->Index);
	}
	otg_host->HC[ch->Index].HCCHAR = cchar | USB_OTG_HCCHAR_CHENA; // halts channel
}

static bool _alloc_channel(unsigned *pindex, usb_host_pipe_t *pipe, usb_direction_t dir, unsigned char period)
{
	for(unsigned i = 0; i < NUM_CHANNELS; i++)
	{
		if (_ch2ep[i] == nullptr)
		{
			stm32_usbh_ep_t *ep = (stm32_usbh_ep_t *)pipe->Endpoint;
			ASSERT(ep != nullptr, KERNEL_ERROR_KERNEL_PANIC);

			_ch_table[i] = (stm32_usbh_channel_t) { .Index = i,
				.EndpointNumber = pipe->EndpointNumber,
				.EndpointType = pipe->EndpointType,
				.Direction = dir,
				.Period = period };
			_ch2ep[i] = ep;

			_open_channel(&_ch_table[i], 
				pipe->Device->Address, pipe->Device->Speed, pipe->MaxPacketSize); 

			*pindex = i;
			return true;
		}
	}
	return false;
}

static void _free_channel(unsigned index)
{
	ASSERT(index < NUM_CHANNELS, KERNEL_ERROR_KERNEL_PANIC);
	stm32_usbh_ep_t *ep = _ch2ep[index];
	ASSERT(ep != nullptr, KERNEL_ERROR_KERNEL_PANIC);
	if (ep->Rx == &_ch_table[index])
		ep->Rx = nullptr;
	else if (ep->Tx == &_ch_table[index])
		ep->Tx = nullptr;

	_ch2ep[index] = nullptr;
}

bool usb_hs_host_start_pipe(usb_host_pipe_t *pipe)
{
	static stm32_usbh_ep_t *_root_control_ep = nullptr;

	ASSERT(pipe != nullptr, KERNEL_ERROR_NULL_POINTER);
	ASSERT(pipe->Device != nullptr, KERNEL_ERROR_NULL_POINTER);
	unsigned chn, chn2;

	unsigned ep_num = pipe->EndpointNumber;
	if (ep_num == 0 && _root_control_ep != nullptr)
	{
		ASSERT(pipe->EndpointType == USB_TT_CONTROL, KERNEL_ERROR_KERNEL_PANIC);
		pipe->Endpoint = _root_control_ep;
		// NOTE: channels are not re-opened because they're already open and control requests will update ep when used
		_verbose(VERBOSE_DEBUG, "endpoint recycled for control pipe (port #%d)", pipe->Device->Port);
		return true;
	}

	stm32_usbh_ep_t *ep = (stm32_usbh_ep_t *)pool_allocate(&_ep_pool);
	if (ep != nullptr)
	{
		ep->Tx = ep->Rx = nullptr;
		ep->Status = STM32_EP_STA_IDLE;
		pipe->Endpoint = ep;

		switch(pipe->EndpointType)
		{
			case USB_TT_CONTROL:
				// control ep supports both input and output
				if (_alloc_channel(&chn, pipe, USB_HOST_TO_DEVICE, 0))
				{
					if (_alloc_channel(&chn2, pipe, USB_DEVICE_TO_HOST, 0))
					{
						_verbose(VERBOSE_DEBUG, "channels %d+%d allocated for control (port #%d)", chn, chn2, pipe->Device->Port); 
						ep->Tx = &_ch_table[chn];
						ep->Rx = &_ch_table[chn2];

						_root_control_ep = ep;
						return true;
					}
					else
					{
						_free_channel(chn);
					}
				}
				break;
			case USB_TT_BULK:
				if (_alloc_channel(&chn, pipe, pipe->Direction, 0))
				{
					if (pipe->Direction == USB_DEVICE_TO_HOST) 
					{
						_verbose(VERBOSE_DEBUG, "channel %d allocated for ep %d as bulk IN (port #%d)", chn, pipe->EndpointNumber, pipe->Device->Port);
						ep->Rx = &_ch_table[chn];
					}
					else
					{
						_verbose(VERBOSE_DEBUG, "channel %d allocated for ep %d as bulk OUT (port #%d)", chn, pipe->EndpointNumber, pipe->Device->Port);
						ep->Tx = &_ch_table[chn];
					}
					return true;
				}
				break;
			case USB_TT_INTERRUPT:
				if (_alloc_channel(&chn, pipe, pipe->Direction, pipe->InterruptInterval))
				{
					if (pipe->Direction == USB_DEVICE_TO_HOST) 
					{
						_verbose(VERBOSE_DEBUG, "channel %d allocated for ep %d as interrupt IN (port #%d)", chn, pipe->EndpointNumber, pipe->Device->Port); 
						ep->Rx = &_ch_table[chn];
					}
					else
					{
						_verbose(VERBOSE_DEBUG, "channel %d allocated for ep %d as interrupt OUT (port #%d)", chn, pipe->EndpointNumber, pipe->Device->Port); 
						ep->Tx = &_ch_table[chn];
					}
					return true;
				}
				break;
			default:
				kernel_panic(KERNEL_ERROR_NOT_IMPLEMENTED);
		}

		_verbose(VERBOSE_ERROR, "alloc channel failed (addr=%d, ep=%d)", pipe->Device->Address, pipe->EndpointNumber); 
		pipe->Endpoint = nullptr;
		pool_free(&_ep_pool, &ep->Node);
	}
	else _verbose(VERBOSE_ERROR, "alloc endpoint failed (addr=%d, ep=%d)", pipe->Device->Address, pipe->EndpointNumber); 

	return false;
}

void usb_hs_host_update_control_pipe(usb_host_pipe_t *pipe)
{
	ASSERT(pipe != nullptr, KERNEL_ERROR_NULL_POINTER);
	ASSERT(pipe->EndpointType == USB_TT_CONTROL, KERNEL_ERROR_KERNEL_PANIC);
	stm32_usbh_ep_t *ep = (stm32_usbh_ep_t *)pipe->Endpoint;
	ASSERT(ep != nullptr, KERNEL_ERROR_KERNEL_PANIC);

	// NOTE: update control pipes, they have both RX and TX channels
	_update_channel(ep->Rx, pipe->Device->Address, pipe->Device->Speed, pipe->MaxPacketSize);
	_update_channel(ep->Tx, pipe->Device->Address, pipe->Device->Speed, pipe->MaxPacketSize);
}

static void _xfer_complete(unsigned ch_num, urb_status_t status)
{
	stm32_usbh_channel_t *ch = &_ch_table[ch_num];
	ASSERT(ch->EndpointNumber < NUM_ENDPOINTS, KERNEL_ERROR_KERNEL_PANIC);
	stm32_usbh_ep_t *ep = _ch2ep[ch_num];
	if (ep != nullptr && ep->Status != STM32_EP_STA_IDLE)
	{
		usb_request_buffer_t *urb = ch->Current.Request;
		if (urb != nullptr)
		{
			if (status == URB_STATUS_DONE && urb->Pipe->Direction == USB_HOST_TO_DEVICE)
			{
				urb->Done = urb->Length;
			}

			urb->Status = status;

			exos_event_reset(&urb->Event);
			ch->Current.Request = nullptr;
		}

		ep->Status = STM32_EP_STA_IDLE;
	}
}

static void _disable_channel(unsigned ch_num, bool wait)
{
	if (otg_host->HPRT & USB_OTG_HPRT_PENA)
	{
		otg_host->HC[ch_num].HCINTMSK |= USB_OTG_HCINTMSK_CHHM;
		otg_host->HC[ch_num].HCCHAR |= USB_OTG_HCCHAR_CHDIS | USB_OTG_HCCHAR_CHENA;	// FIXME
		if (wait)
		{
			stm32_usbh_ep_t *ep;
			while(ep = _ch2ep[ch_num], ep != nullptr)
			{
				//ASSERT(ep->Status == STM32_EP_STA_STOPPING, KERNEL_ERROR_KERNEL_PANIC);
			}
		}
	}
	else
	{
		otg_host->HC[ch_num].HCINTMSK = 0;
		_xfer_complete(ch_num, URB_STATUS_FAILED);
		_free_channel(ch_num);
	}
}

void usb_hs_host_stop_pipe(usb_host_pipe_t *pipe)
{
	ASSERT(pipe != nullptr, KERNEL_ERROR_NULL_POINTER);
	stm32_usbh_ep_t *ep = (stm32_usbh_ep_t *)pipe->Endpoint;
	ASSERT(ep != nullptr, KERNEL_ERROR_KERNEL_PANIC);

	ep->Status = STM32_EP_STA_STOPPING;
	if (ep->Tx != nullptr)
		_disable_channel(ep->Tx->Index, true);
	if (ep->Rx != nullptr)
		_disable_channel(ep->Rx->Index, true);

	ep->Status = STM32_EP_STA_IDLE;
	pool_free(&_ep_pool, &ep->Node);
}

static unsigned _write_fifo(unsigned ch_num, bool ack_done)
{
	ASSERT(ch_num < NUM_CHANNELS, KERNEL_ERROR_KERNEL_PANIC);
	stm32_usbh_channel_t *ch = &_ch_table[ch_num];
	stm32_usbh_ep_t *ep = _ch2ep[ch_num];
	ASSERT(ep != nullptr, KERNEL_ERROR_NULL_POINTER);

	stm32_usbh_xfer_t *xfer = &ch->Current;
	usb_request_buffer_t *urb = xfer->Request;
	if (urb == nullptr)
		return 0;
	
	ASSERT(urb->Pipe != nullptr && urb->Pipe->Endpoint == ep, KERNEL_ERROR_KERNEL_PANIC);
	ASSERT(urb->Pipe->EndpointNumber == ch->EndpointNumber, KERNEL_ERROR_KERNEL_PANIC);
	if (ack_done)
		urb->Done += xfer->LastPacketLength;

	unsigned rem = (urb->Length > urb->Done) ? urb->Length - urb->Done : 0;
	unsigned lenw = (rem + 3) >> 2;
	unsigned availw = (otg_global->HNPTXSTS & USB_OTG_GNPTXSTS_NPTXFSAV_Msk) >> USB_OTG_GNPTXSTS_NPTXFSAV_Pos;
	ASSERT(availw >= lenw, KERNEL_ERROR_KERNEL_PANIC);
	uint32_t *ptr = (uint32_t *)(urb->Data + urb->Done);	// FIXME: alignment required?

	for(unsigned i = 0; i < lenw; i++)
		*otg_fifo[ch_num] = *ptr++;

	xfer->LastPacketLength = lenw << 2;
	unsigned fifo_done = urb->Done + xfer->LastPacketLength;
	rem = (fifo_done < urb->Length) ? urb->Length - fifo_done : 0;
	return rem;
}

static void _enable_channel(unsigned ch_num)
{
	stm32_usbh_channel_t *ch = &_ch_table[ch_num];

	unsigned hcchar = otg_host->HC[ch_num].HCCHAR & ~USB_OTG_HCCHAR_CHDIS;
	otg_host->HC[ch_num].HCCHAR = hcchar | USB_OTG_HCCHAR_CHENA;

	if (ch->Direction == USB_HOST_TO_DEVICE)
	{
		unsigned rem = _write_fifo(ch->Index, false);
		if (rem != 0)
		{
			switch(ch->EndpointType)
			{
				case USB_TT_CONTROL:
				case USB_TT_BULK:
					// allow processing in nptxfempty interrupt
					otg_global->GINTMSK |= USB_OTG_GINTMSK_NPTXFEM;
					break;
				case USB_TT_INTERRUPT:
				case USB_TT_ISO:
					// allow processing in ptxfempty interrupt
					otg_global->GINTMSK |= USB_OTG_GINTMSK_PTXFEM;
					break;
			}
		}
	}
}

bool usb_hs_host_begin_xfer(usb_request_buffer_t *urb, usb_direction_t dir, bool setup)
{
	ASSERT(urb != nullptr && urb->Pipe != nullptr, KERNEL_ERROR_NULL_POINTER);
	stm32_usbh_ep_t *ep = (stm32_usbh_ep_t *)urb->Pipe->Endpoint;
	ASSERT(ep != nullptr && urb->Pipe->Endpoint == ep, KERNEL_ERROR_KERNEL_PANIC);

	ASSERT(urb->Data != nullptr || urb->Length == 0, KERNEL_ERROR_NULL_POINTER);
	unsigned packet_size = urb->Pipe->MaxPacketSize;
	ASSERT(packet_size != 0, KERNEL_ERROR_KERNEL_PANIC);
	unsigned num_packets = (urb->Length != 0) ? 
		(urb->Length + (packet_size - 1)) / packet_size : 1;

	stm32_usbh_channel_t *ch = nullptr;
	unsigned length; 
	if (dir == USB_DEVICE_TO_HOST)
	{
		length = num_packets * packet_size;
		ch = ep->Rx;
	}
	else
	{
		length = urb->Length;
		ch = ep->Tx;
	}

	ASSERT(ch != nullptr, KERNEL_ERROR_KERNEL_PANIC);
	unsigned ch_num = ch->Index;

	ASSERT(ep->Status == STM32_EP_STA_IDLE, KERNEL_ERROR_NOT_IMPLEMENTED);	// TODO: queue
	ep->Status = STM32_EP_STA_BUSY;
	stm32_usbh_xfer_t *xfer = &ch->Current;	
	xfer->Request = urb;

	switch(urb->Pipe->EndpointType)
	{
		case USB_TT_CONTROL:	
			ch->Pid = setup ? PID_SETUP : PID_DATA1; 
			break;
		case USB_TT_INTERRUPT:
		case USB_TT_BULK:		
			ch->Pid = ch->Toggle ? PID_DATA1 : PID_DATA0;
			ch->Toggle ^= num_packets & 1;

			otg_host->HC[ch_num].HCCHAR = 
				(otg_host->HC[ch_num].HCCHAR & ~(USB_OTG_HCCHAR_CHENA_Msk | USB_OTG_HCCHAR_ODDFRM_Msk))
				| ((otg_host->HFNUM & 1) ? 0 : USB_OTG_HCCHAR_ODDFRM);
			break;
		default:	kernel_panic(KERNEL_ERROR_NOT_IMPLEMENTED);
	}

	otg_host->HC[ch_num].HCTSIZ = (length << USB_OTG_HCTSIZ_XFRSIZ_Pos)
		| (num_packets << USB_OTG_HCTSIZ_PKTCNT_Pos)
		| (ch->Pid << USB_OTG_HCTSIZ_DPID_Pos);

	_enable_channel(ch_num);
	return true;
}

void __usb_hs_rxflvl_irq_handler()
{
	otg_global->GINTMSK &= ~USB_OTG_GINTMSK_RXFLVLM;

	unsigned stsp = otg_global->GRXSTSP;
	unsigned pktsts = (stsp & USB_OTG_GRXSTSP_PKTSTS_Msk) >> USB_OTG_GRXSTSP_PKTSTS_Pos;
	unsigned bcnt = (stsp & USB_OTG_GRXSTSP_BCNT_Msk) >> USB_OTG_GRXSTSP_BCNT_Pos;
	unsigned ch_num = (stsp & USB_OTG_GRXSTSP_EPNUM_Msk) >> USB_OTG_GRXSTSP_EPNUM_Pos;
	ASSERT(ch_num < NUM_CHANNELS, KERNEL_ERROR_KERNEL_PANIC);
	stm32_usbh_channel_t *ch = &_ch_table[ch_num];

	if (pktsts == 0b0010		// IN data_packet
		&& bcnt != 0)
	{
		stm32_usbh_ep_t *ep = _ch2ep[ch_num];
		ASSERT(ep != nullptr, KERNEL_ERROR_KERNEL_PANIC);

		stm32_usbh_xfer_t *xfer = &ch->Current;
		ASSERT(ep->Status == STM32_EP_STA_BUSY, KERNEL_ERROR_KERNEL_PANIC);
		usb_request_buffer_t *urb = xfer->Request;
		ASSERT(urb != nullptr && urb->Pipe != nullptr, KERNEL_ERROR_NULL_POINTER);
		ASSERT(urb->Pipe->Endpoint == ep, KERNEL_ERROR_KERNEL_PANIC);
		unsigned char *ptr = (unsigned char *)urb->Data + urb->Done;

		unsigned dataw;
		unsigned rem = bcnt;
		while (rem >= 4)
		{
			dataw = *otg_fifo[ch_num];
			*ptr++ = (unsigned char)dataw;
			*ptr++ = (unsigned char)(dataw >> 8);
			*ptr++ = (unsigned char)(dataw >> 16);
			*ptr++ = (unsigned char)(dataw >> 24);
			rem -= 4;
		}
		if (rem != 0)
		{
			dataw = *otg_fifo[ch_num];
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

		urb->Done += bcnt;
		ASSERT(ptr == (unsigned char *)urb->Data + urb->Done, KERNEL_ERROR_KERNEL_PANIC);

		unsigned pkt_rem = (otg_host->HC[ch_num].HCTSIZ & USB_OTG_HCTSIZ_PKTCNT_Msk) >> USB_OTG_HCTSIZ_PKTCNT_Pos;
		if (pkt_rem != 0)
		{
			otg_host->HC[ch_num].HCINT = USB_OTG_HCINT_ACK;
			_enable_channel(ch_num);	// NOTE: re-enable channel
		}
	}

	otg_global->GINTMSK |= USB_OTG_GINTMSK_RXFLVLM;
}

void __usb_hs_nptxfe_irq_handler()
{
	unsigned sts = otg_global->HNPTXSTS;
	unsigned qtop = (sts & USB_OTG_GNPTXSTS_NPTXQTOP_Msk) >> USB_OTG_GNPTXSTS_NPTXQTOP_Pos;
	unsigned ch_num = qtop >> 3;

	unsigned rem = _write_fifo(ch_num, true);
	if (rem == 0)
	{
		// disable int
		otg_global->GINTMSK &= ~USB_OTG_GINTMSK_NPTXFEM;
	}
}

void __usb_hs_hcint_irq_handler()
{
	while(1)
	{
		unsigned haint = otg_host->HAINT;
		if (haint == 0) break;

		for (int ch_num = NUM_CHANNELS - 1; ch_num >= 0; ch_num--)
		{
			if (haint & (1 << ch_num))
			{
				unsigned hcint = otg_host->HC[ch_num].HCINT;
				stm32_usbh_channel_t *ch = &_ch_table[ch_num];

				if (hcint & USB_OTG_HCINT_TXERR)
				{
					_xfer_complete(ch_num, URB_STATUS_FAILED);
					otg_host->HC[ch_num].HCINT = USB_OTG_HCINT_TXERR;
				}
				else if (hcint & USB_OTG_HCINT_STALL)
				{
					_xfer_complete(ch_num, URB_STATUS_FAILED);
					otg_host->HC[ch_num].HCINT = USB_OTG_HCINT_STALL;
				}
				else if (hcint & USB_OTG_HCINT_XFRC)
				{
					_xfer_complete(ch_num, URB_STATUS_DONE);
					otg_host->HC[ch_num].HCINT = USB_OTG_HCINT_XFRC | USB_OTG_HCINT_ACK;
				}
				else if (hcint & USB_OTG_HCINT_NAK)
				{
					if (ch->Period == 0)
					{
						if (_ch_table[ch_num].Direction == USB_DEVICE_TO_HOST)
						{
							// TODO: limit with error count
							_enable_channel(ch_num);		// NOTE: retry
						}
						else
						{
							_disable_channel(ch_num, false); // NOTE: will trigger CHH interrupt
						}
					}
					else
					{
						_disable_channel(ch_num, false); // NOTE: will trigger CHH interrupt
					}
					otg_host->HC[ch_num].HCINT = USB_OTG_HCINT_NAK;
				}
				else if (hcint & USB_OTG_HCINT_CHH)
				{
					otg_host->HC[ch_num].HCINTMSK &= ~USB_OTG_HCINTMSK_CHHM;

					stm32_usbh_ep_t *ep = _ch2ep[ch_num];
					if (ep != nullptr)
					{
						switch (ep->Status)
						{
							case STM32_EP_STA_BUSY:
								if (ch->Period == 0)
								{
									// TODO: error count?

									// NOTE: GD32x workaround: need to re-write HCSIZx register to re-enable non-periodic
									otg_host->HC[ch_num].HCTSIZ = otg_host->HC[ch_num].HCTSIZ;

									_enable_channel(ch_num);
								}
								else
								{
									// TODO: we cannot re-start a periodic channel immediately
									// we have to wait a number of SOF before
									unsigned hcchar = otg_host->HC[ch_num].HCCHAR;
									hcchar &= ~(USB_OTG_HCCHAR_CHDIS | USB_OTG_HCCHAR_CHENA);
									hcchar ^= USB_OTG_HCCHAR_ODDFRM;
									otg_host->HC[ch_num].HCCHAR = hcchar;
									_enable_channel(ch_num);
								}
								break;
							case STM32_EP_STA_STOPPING:
								_xfer_complete(ch_num, URB_STATUS_FAILED);
								_free_channel(ch_num);
								break;
						}
					}
					otg_host->HC[ch_num].HCINT = USB_OTG_HCINT_CHH;
				}
				else if (hcint & USB_OTG_HCINT_FRMOR)
				{
					otg_host->HC[ch_num].HCINT = USB_OTG_HCINT_FRMOR;
					_disable_channel(ch_num, false); // NOTE: will trigger CHH interrupt
				}
				else kernel_panic(KERNEL_ERROR_NOT_IMPLEMENTED);
			}
		}
	}
}

void __usb_otg_hs_host_irq_handler()
{
	unsigned sta = (otg_global->GINTSTS & otg_global->GINTMSK);

	if (sta & USB_OTG_GINTSTS_SOF)
	{
		ASSERT(_hc != nullptr, KERNEL_ERROR_KERNEL_PANIC);
		exos_event_reset(&_hc->SOF);

		otg_global->GINTSTS = USB_OTG_GINTSTS_SOF;	//clear int request
	}
	if (sta & USB_OTG_GINTSTS_RXFLVL)
	{
		__usb_hs_rxflvl_irq_handler();
	}
	if (sta & USB_OTG_GINTSTS_PTXFE)
	{
		kernel_panic(KERNEL_ERROR_NOT_IMPLEMENTED);
	}
	if (sta & USB_OTG_GINTSTS_NPTXFE)
	{
		__usb_hs_nptxfe_irq_handler();
	}

	if (sta & USB_OTG_GINTSTS_HCINT)
	{
		__usb_hs_hcint_irq_handler();
	}

	if (sta & USB_OTG_GINTSTS_HPRTINT)
	{
		unsigned hprt = otg_host->HPRT;
		unsigned hprt_const = hprt & (USB_OTG_HPRT_PPWR | USB_OTG_HPRT_PRST | USB_OTG_HPRT_PSUSP | USB_OTG_HPRT_PRES);

		if (hprt & USB_OTG_HPRT_PCDET)
		{
			otg_host->HPRT = hprt_const | USB_OTG_HPRT_PCDET;	 // clear interrupt
			exos_event_set(&_hc->RootHubEvent);
		}
		else if (hprt & USB_OTG_HPRT_PENCHNG)
		{
			otg_host->HPRT = hprt_const | USB_OTG_HPRT_PENCHNG;	 // clear interrupt
			exos_event_set(&_hc->RootHubEvent);
		}
	}
}
