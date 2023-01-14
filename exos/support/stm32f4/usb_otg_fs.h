#ifndef STM32F4_USB_OTG_FS_H
#define STM32F4_USB_OTG_FS_H

#include <kernel/types.h>
#include <usb/host.h>

#define USB_OTG_FS_BASE 0x50000000UL

typedef struct 
{
	volatile unsigned GOTGCTL;		// offset $000
	volatile unsigned GOTGINT;
	volatile unsigned GAHBCFG;
	volatile unsigned GUSBCFG;
	volatile unsigned GRSTCTL;
	volatile unsigned GINTSTS;
	volatile unsigned GINTMSK;
   	volatile unsigned GRXSTSR;
   	volatile unsigned GRXSTSP;
	volatile unsigned GRXFSIZ;
	union 
	{
		volatile unsigned HNPTXFSIZ;	// host mode
		volatile unsigned DIEPTXF0;	// device mode
	};
	volatile unsigned HNPTXSTS;
	volatile unsigned Reserved30;	
	volatile unsigned Reserved34;	
	volatile unsigned GCCFG;
	volatile unsigned CID;	
	volatile unsigned Reserved40[48];
	volatile unsigned HPTXFSIZ;	// offset $100
	volatile unsigned DIEPTXF1;
	volatile unsigned DIEPTXF2;
	volatile unsigned DIEPTXF3;
	volatile unsigned DIEPTXF4;
} usb_otg_crs_global_t;

#define OTG_FS_GAHBCFG_GINTMASK (1<<0)
#define OTG_FS_GAHBCFG_TXFELVL (1<<7)
#define OTG_FS_GAHBCFG_PTXFELVL (1<<8)

typedef struct 
{
	volatile unsigned HCFG;		// offset $400
	volatile unsigned HFIR;
	volatile unsigned HFNUM;
   	volatile unsigned Reserved40C;
	volatile unsigned HPTXSTS;
	volatile unsigned HAINT;
	volatile unsigned HAINTMSK;
   	volatile unsigned Reserved41C[9];
   	volatile unsigned HPRT;		// 0ffset $440
	volatile unsigned Reserved444[47];

	struct {
		volatile unsigned HCCHAR;		// 0ffset $500
		volatile unsigned Reserved504;
		volatile unsigned HCINT;
		volatile unsigned HCINTMSK;
		volatile unsigned HCTSIZ;
		volatile unsigned Recerved514[3];
	} HC[8];

} usb_otg_crs_host_t;

typedef struct 
{
	volatile unsigned DCFG;		// offset $800
	volatile unsigned DCTL;
	volatile unsigned DSTS;
   	volatile unsigned Reserved80C;
	volatile unsigned DIEPMSK;
	volatile unsigned DOEPMSK;
	volatile unsigned DAINT;
	volatile unsigned DAINTMSK;
	volatile unsigned Reserved820;
	volatile unsigned Reserved824;
	volatile unsigned DVBUSDIS;
	volatile unsigned DVBUSPULSE;
	volatile unsigned Reserved830;
	volatile unsigned DIEPEMPMSK;
	volatile unsigned Reserved838[50];

   	volatile unsigned DIEPCTL0;	// offset $900
	volatile unsigned Reserved904;
	volatile unsigned DIEPINT0;
	volatile unsigned Reserved90c;
	volatile unsigned DIEPTSIZ0;
	volatile unsigned Reserved914;
	volatile unsigned DTXFSTS0;	
	volatile unsigned Reserved91c;

   	volatile unsigned DIEPCTL1;	// offset $920
	volatile unsigned Reserved924;
	volatile unsigned DIEPINT1;
	volatile unsigned Reserved92c;
	volatile unsigned DIEPTSIZ1;
	volatile unsigned Reserved934;
	volatile unsigned DTXFSTS1;	
	volatile unsigned Reserved93c;

   	volatile unsigned DIEPCTL2;	// offset $940
	volatile unsigned Reserved944;
	volatile unsigned DIEPINT2;
	volatile unsigned Reserved94c;
	volatile unsigned DIEPTSIZ2;
	volatile unsigned Reserved954;
	volatile unsigned DTXFSTS2;	
	volatile unsigned Reserved95c;

   	volatile unsigned DIEPCTL3;	// offset $960
	volatile unsigned Reserved964;
	volatile unsigned DIEPINT3;
	volatile unsigned Reserved96c;
	volatile unsigned DIEPTSIZ3;
	volatile unsigned Reserved974;
	volatile unsigned DTXFSTS3;	
	volatile unsigned Reserved97c;

   	volatile unsigned DIEPCTL4;	// offset $980
	volatile unsigned Reserved984;
	volatile unsigned DIEPINT4;
	volatile unsigned Reserved98c;
	volatile unsigned DIEPTSIZ4;
	volatile unsigned Reserved994;
	volatile unsigned DTXFSTS4;	
	volatile unsigned Reserved99c;

	volatile unsigned Reserved9a0[88];

	volatile unsigned DOEPCTL0;	// offset $b00
	volatile unsigned Reservedb04;
	volatile unsigned DOEPINT0;
	volatile unsigned Reservedb0c;
	volatile unsigned DOEPTSIZ0;
	volatile unsigned Reservedb14;
	volatile unsigned Reservedb18;
	volatile unsigned Reservedb1c;

	volatile unsigned DOEPCTL1;	// offset $b20
	volatile unsigned Reservedb24;
	volatile unsigned DOEPINT1;
	volatile unsigned Reservedb2c;
	volatile unsigned DOEPTSIZ1;
	volatile unsigned Reservedb34;
	volatile unsigned Reservedb38;
	volatile unsigned Reservedb3c;

	volatile unsigned DOEPCTL2;	// offset $b40
	volatile unsigned Reservedb44;
	volatile unsigned DOEPINT2;
	volatile unsigned Reservedb4c;
	volatile unsigned DOEPTSIZ2;
	volatile unsigned Reservedb54;
	volatile unsigned Reservedb58;
	volatile unsigned Reservedb5c;

	volatile unsigned DOEPCTL3;	// offset $b60
	volatile unsigned Reservedb64;
	volatile unsigned DOEPINT3;
	volatile unsigned Reservedb6c;
	volatile unsigned DOEPTSIZ3;
	volatile unsigned Reservedb74;
	volatile unsigned Reservedb78;
	volatile unsigned Reservedb7c;

	volatile unsigned DOEPCTL4;	// offset $b80
	volatile unsigned Reservedb84;
	volatile unsigned DOEPINT4;
	volatile unsigned Reservedb8c;
	volatile unsigned DOEPTSIZ4;
	volatile unsigned Reservedb94;
	volatile unsigned Reservedb98;
	volatile unsigned Reservedb9c;
} usb_otg_crs_device_t;

typedef struct 
{
	volatile unsigned PCGCCTL;		// offset $e00
} usb_otg_crs_power_t;

extern void __usb_otg_host_irq_handler() __weak;
extern void __usb_otg_device_irq_handler() __weak;

extern event_t *__otg_fs_event;

void usb_otg_fs_initialize();
usb_host_role_state_t usb_otg_fs_role_state();
void usb_otg_fs_notify(usb_host_role_state_t state);


#endif // STM32F4_USB_OTG_FS_H