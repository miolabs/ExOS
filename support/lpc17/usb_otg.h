#ifndef LPC17_USB_OTG_H
#define LPC17_USB_OTG_H

#include "cpu.h"

#define USBIntSt_USB_INT_REQ_LP 0x1
#define USBIntSt_USB_INT_REQ_HP 0x2
#define USBIntSt_USB_INT_REQ_DMA 0x4
#define USBIntSt_USB_HOST_INT 0x8
#define USBIntSt_USB_ATX_INT 0x10
#define USBIntSt_USB_OTG_INT 0x20
#define USBIntSt_USB_I2C_INT 0x40
#define USBIntSt_USB_NEED_CLK 0x100
#define USBIntSt_EN_USB_INTS 0x80000000

#define USB_BASE 0xFFE0C000

#define OTGIntSt_TMR 0x1
#define OTGIntSt_REMOVE_PU 0x2
#define OTGIntSt_HNP_FAILURE 0x4
#define OTGIntSt_HNP_SUCCESS 0x8

#define OTGIntEn_TMR 0x1
#define OTGIntEn_REMOVE_PU 0x2
#define OTGIntEn_HNP_FAILURE 0x4
#define OTGIntEn_HNP_SUCCESS 0x8

#define OTGIntSet_TMR 0x1
#define OTGIntSet_REMOVE_PU 0x2
#define OTGIntSet_HNP_FAILURE 0x4
#define OTGIntSet_HNP_SUCCESS 0x8

#define OTGIntClr_TMR 0x1
#define OTGIntClr_REMOVE_PU 0x2
#define OTGIntClr_HNP_FAILURE 0x4
#define OTGIntClr_HNP_SUCCESS 0x8

typedef struct _OTG_ST_CTRL_BITS
{
	unsigned PORT_FUNC:2;
	unsigned TMR_SCALE:2;
	unsigned TMR_MODE:1;
	unsigned TMR_EN:1;
	unsigned TRM_RST:1;
	unsigned :1;
	unsigned B_HNP_TRACK:1;
	unsigned A_HNP_TRACK:1;
	unsigned PU_REMOVED:1;
	unsigned :5;
	unsigned TMR_CNT:16;
} OTG_ST_CTRL_BITS;


#define OTGStCtrlBits (*(volatile OTG_ST_CTRL_BITS *)&LPC_USB->OTGStCtrl)

#define OTGTmr_OFFSET 0x114
#define OTGTmr_TIMEOUT_CNT_MASK 0xFFFF
#define OTGTmr_TIMEOUT_CNT_BIT 0

typedef struct _OTG_CLK_BITS
{
	unsigned HOST_CLK:1;
	unsigned DEV_CLK:1;
	unsigned I2C_CLK:1;
	unsigned OTG_CLK:1;
	unsigned AHB_CLK:1;
} OTG_CLK_BITS;

#define OTGClkCtrlBits (*(volatile OTG_CLK_BITS *)&LPC_USB->OTGClkCtrl)
#define OTGClkCtrl_HOST_CLK_EN 0x1
#define OTGClkCtrl_DEV_CLK_EN 0x2
#define OTGClkCtrl_I2C_CLK_EN 0x4
#define OTGClkCtrl_OTG_CLK_EN 0x8
#define OTGClkCtrl_AHB_CLK_EN 0x10

#define OTGClkStBits (*(volatile OTG_CLK_BITS *)&LPC_USB->OTGClkSt)
#define OTGClkSt_HOST_CLK_ON 0x1
#define OTGClkSt_DEV_CLK_ON 0x2
#define OTGClkSt_I2C_CLK_ON 0x4
#define OTGClkSt_OTG_CLK_ON 0x8
#define OTGClkSt_AHB_CLK_ON 0x10


#if (__TARGET_PROCESSOR >= 1770)
#define OTGStCtrl StCtrl
#endif

// prototypes
void usb_otg_initialize_host();
void usb_otg_initialize_device();
void usb_otg_device_int_control(int enable);

#endif // LPC17_USB_OTG_H
