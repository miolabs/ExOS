#ifndef STM32F1_USB_OTG_HS_H
#define STM32F1_USB_OTG_HS_H

#include <kernel/types.h>
#include <usb/host.h>

#define USB_OTG_HS_BASE 0x40040000UL

typedef struct 
{
	volatile unsigned long GOTGCTL;		// offset $000
	volatile unsigned long GOTGINT;
	volatile unsigned long GAHBCFG;
	volatile unsigned long GUSBCFG;
	volatile unsigned long GRSTCTL;
	volatile unsigned long GINTSTS;
	volatile unsigned long GINTMSK;
   	volatile unsigned long GRXSTSR;
   	volatile unsigned long GRXSTSP;
	volatile unsigned long GRXFSIZ;
	union 
	{
		volatile unsigned long HNPTXFSIZ;	// host mode
		volatile unsigned long DIEPTXF0;	// device mode
	};
	volatile unsigned long HNPTXSTS;
	volatile unsigned long Reserved030;	
	volatile unsigned long Reserved034;	
	volatile unsigned long GCCFG;
	volatile unsigned long CID;	
	volatile unsigned long Reserved40[48];
	volatile unsigned long HPTXFSIZ;	// offset $100
	volatile unsigned long DIEPTXF1;
	volatile unsigned long DIEPTXF2;
	volatile unsigned long DIEPTXF3;
} usb_otg_crs_global_t;

#define OTG_FS_GAHBCFG_GINTMASK (1<<0)
#define OTG_FS_GAHBCFG_TXFELVL (1<<7)
#define OTG_FS_GAHBCFG_PTXFELVL (1<<8)

typedef struct 
{
	volatile unsigned long HCFG;		// offset $400
	volatile unsigned long HFIR;
	volatile unsigned long HFNUM;
   	volatile unsigned long Reserved40C;
	volatile unsigned long HPTXSTS;
	volatile unsigned long HAINT;
	volatile unsigned long HAINTMSK;
   	volatile unsigned long Reserved41C[9];
   	volatile unsigned long HPRT;		// 0ffset $440
	volatile unsigned long Reserved444[47];

	struct {
		volatile unsigned long HCCHAR;		// 0ffset $500
		volatile unsigned long Reserved504;
		volatile unsigned long HCINT;
		volatile unsigned long HCINTMSK;
		volatile unsigned long HCTSIZ;
		volatile unsigned long Recerved514[3];
	} HC[8];

} usb_otg_crs_host_t;

typedef struct 
{
	volatile unsigned long DCFG;		// offset $800
	volatile unsigned long DCTL;
	volatile unsigned long DSTS;
   	volatile unsigned long Reserved80C;
	volatile unsigned long DIEPMSK;
	volatile unsigned long DOEPMSK;
	volatile unsigned long DAINT;
	volatile unsigned long DAINTMSK;
	volatile unsigned long Reserved820;
	volatile unsigned long Reserved824;
	volatile unsigned long DVBUSDIS;
	volatile unsigned long DVBUSPULSE;
	volatile unsigned long Reserved830;
	volatile unsigned long DIEPEMPMSK;
	volatile unsigned long Reserved838[50];

   	volatile unsigned long DIEPCTL0;	// offset $900
	volatile unsigned long Reserved904;
	volatile unsigned long DIEPINT0;
	volatile unsigned long Reserved90c;
	volatile unsigned long DIEPTSIZ0;
	volatile unsigned long Reserved914;
	volatile unsigned long DTXFSTS0;	
	volatile unsigned long Reserved91c;

   	volatile unsigned long DIEPCTL1;	// offset $920
	volatile unsigned long Reserved924;
	volatile unsigned long DIEPINT1;
	volatile unsigned long Reserved92c;
	volatile unsigned long DIEPTSIZ1;
	volatile unsigned long Reserved934;
	volatile unsigned long DTXFSTS1;	
	volatile unsigned long Reserved93c;

   	volatile unsigned long DIEPCTL2;	// offset $940
	volatile unsigned long Reserved944;
	volatile unsigned long DIEPINT2;
	volatile unsigned long Reserved94c;
	volatile unsigned long DIEPTSIZ2;
	volatile unsigned long Reserved954;
	volatile unsigned long DTXFSTS2;	
	volatile unsigned long Reserved95c;

   	volatile unsigned long DIEPCTL3;	// offset $960
	volatile unsigned long Reserved964;
	volatile unsigned long DIEPINT3;
	volatile unsigned long Reserved96c;
	volatile unsigned long DIEPTSIZ3;
	volatile unsigned long Reserved974;
	volatile unsigned long DTXFSTS3;	
	volatile unsigned long Reserved97c;

	volatile unsigned long Reserved980[96];

	volatile unsigned long DOEPCTL0;	// offset $b00
	volatile unsigned long Reservedb04;
	volatile unsigned long DOEPINT0;
	volatile unsigned long Reservedb0c;
	volatile unsigned long DOEPTSIZ0;
	volatile unsigned long Reservedb14;
	volatile unsigned long Reservedb18;
	volatile unsigned long Reservedb1c;

	volatile unsigned long DOEPCTL1;	// offset $b20
	volatile unsigned long Reservedb24;
	volatile unsigned long DOEPINT1;
	volatile unsigned long Reservedb2c;
	volatile unsigned long DOEPTSIZ1;
	volatile unsigned long Reservedb34;
	volatile unsigned long Reservedb38;
	volatile unsigned long Reservedb3c;

	volatile unsigned long DOEPCTL2;	// offset $b40
	volatile unsigned long Reservedb44;
	volatile unsigned long DOEPINT2;
	volatile unsigned long Reservedb4c;
	volatile unsigned long DOEPTSIZ2;
	volatile unsigned long Reservedb54;
	volatile unsigned long Reservedb58;
	volatile unsigned long Reservedb5c;

	volatile unsigned long DOEPCTL3;	// offset $b60
	volatile unsigned long Reservedb64;
	volatile unsigned long DOEPINT3;
	volatile unsigned long Reservedb6c;
	volatile unsigned long DOEPTSIZ3;
	volatile unsigned long Reservedb74;
	volatile unsigned long Reservedb78;
	volatile unsigned long Reservedb7c;
} usb_otg_crs_device_t;

typedef struct 
{
	volatile unsigned long PCGCCTL;		// offset $e00
} usb_otg_crs_power_t;


extern void __usb_otg_hs_host_irq_handler() __weak;
extern void __usb_otg_hs_device_irq_handler() __weak;

extern event_t *__otg_hs_event;

void usb_otg_hs_initialize();
usb_host_role_state_t usb_otg_hs_role_state();
void usb_otg_hs_notify(usb_host_role_state_t state);



#endif // STM32F1_USB_OTG_HS_H