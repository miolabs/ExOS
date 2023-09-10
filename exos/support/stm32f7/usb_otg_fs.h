#ifndef STM32F7_USB_OTG_FS_H
#define STM32F7_USB_OTG_FS_H

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
		volatile unsigned DIEPTXF0;		// device mode
	};
	volatile unsigned HNPTXSTS;
	volatile unsigned Reserved030;	
	volatile unsigned Reserved034;	
	volatile unsigned GCCFG;
	volatile unsigned CID;	
	volatile unsigned Reserved40[48];
	volatile unsigned HPTXFSIZ;	// offset $100
	volatile unsigned DIEPTXF1;
	volatile unsigned DIEPTXF2;
	volatile unsigned DIEPTXF3;
	volatile unsigned DIEPTXF4;	// offset $110
	volatile unsigned DIEPTXF5;
	volatile unsigned DIEPTXF6;
	volatile unsigned DIEPTXF7;	 
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

	struct {	// offset $900
   	volatile unsigned CTL;	
	volatile unsigned ResIEP04;
	volatile unsigned INT;
	volatile unsigned ResIEP0c;
	volatile unsigned TSIZ;
	volatile unsigned ResIEP14;
	volatile unsigned TXFSTS;	
	volatile unsigned ResIEP1c;
	} DIEP[8];

	volatile unsigned Reserveda00[64];

	struct {	// offset $b00
	volatile unsigned CTL;
	volatile unsigned ResOEP04;
	volatile unsigned INT;
	volatile unsigned ResOEP0c;
	volatile unsigned TSIZ;
	volatile unsigned ResOEP14;
	volatile unsigned ResOEP18;
	volatile unsigned ResOEP1c;
	} DOEP[8];

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

#endif // STM32F7_USB_OTG_FS_H
