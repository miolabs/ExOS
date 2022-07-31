#ifndef LPC17_USB_OTG_H
#define LPC17_USB_OTG_H

#include <support/lpc2k/cpu.h>

/*------------- Universal Serial Bus (USB) -----------------------------------*/
typedef struct
{
  __I  unsigned long HcRevision;             /* USB Host Registers                 */
  __IO unsigned long HcControl;
  __IO unsigned long HcCommandStatus;
  __IO unsigned long HcInterruptStatus;
  __IO unsigned long HcInterruptEnable;
  __IO unsigned long HcInterruptDisable;
  __IO unsigned long HcHCCA;
  __I  unsigned long HcPeriodCurrentED;
  __IO unsigned long HcControlHeadED;
  __IO unsigned long HcControlCurrentED;
  __IO unsigned long HcBulkHeadED;
  __IO unsigned long HcBulkCurrentED;
  __I  unsigned long HcDoneHead;
  __IO unsigned long HcFmInterval;
  __I  unsigned long HcFmRemaining;
  __I  unsigned long HcFmNumber;
  __IO unsigned long HcPeriodicStart;
  __IO unsigned long HcLSTreshold;
  __IO unsigned long HcRhDescriptorA;
  __IO unsigned long HcRhDescriptorB;
  __IO unsigned long HcRhStatus;
  __IO unsigned long HcRhPortStatus1;
  __IO unsigned long HcRhPortStatus2;
       unsigned long RESERVED0[40];
  __I  unsigned long Module_ID;

  __I  unsigned long OTGIntSt;               /* USB On-The-Go Registers            */
  __IO unsigned long OTGIntEn;
  __O  unsigned long OTGIntSet;
  __O  unsigned long OTGIntClr;
  __IO unsigned long OTGStCtrl;
  __IO unsigned long OTGTmr;
       unsigned long RESERVED1[58];

  __I  unsigned long USBDevIntSt;            /* USB Device Interrupt Registers     */
  __IO unsigned long USBDevIntEn;
  __O  unsigned long USBDevIntClr;
  __O  unsigned long USBDevIntSet;

  __O  unsigned long USBCmdCode;             /* USB Device SIE Command Registers   */
  __I  unsigned long USBCmdData;

  __I  unsigned long USBRxData;              /* USB Device Transfer Registers      */
  __O  unsigned long USBTxData;
  __I  unsigned long USBRxPLen;
  __O  unsigned long USBTxPLen;
  __IO unsigned long USBCtrl;
  __O  unsigned long USBDevIntPri;

  __I  unsigned long USBEpIntSt;             /* USB Device Endpoint Interrupt Regs */
  __IO unsigned long USBEpIntEn;
  __O  unsigned long USBEpIntClr;
  __O  unsigned long USBEpIntSet;
  __O  unsigned long USBEpIntPri;

  __IO unsigned long USBReEp;                /* USB Device Endpoint Realization Reg*/
  __O  unsigned long USBEpInd;
  __IO unsigned long USBMaxPSize;

  __I  unsigned long USBDMARSt;              /* USB Device DMA Registers           */
  __O  unsigned long USBDMARClr;
  __O  unsigned long USBDMARSet;
       unsigned long RESERVED2[9];
  __IO unsigned long USBUDCAH;
  __I  unsigned long USBEpDMASt;
  __O  unsigned long USBEpDMAEn;
  __O  unsigned long USBEpDMADis;
  __I  unsigned long USBDMAIntSt;
  __IO unsigned long USBDMAIntEn;
       unsigned long RESERVED3[2];
  __I  unsigned long USBEoTIntSt;
  __O  unsigned long USBEoTIntClr;
  __O  unsigned long USBEoTIntSet;
  __I  unsigned long USBNDDRIntSt;
  __O  unsigned long USBNDDRIntClr;
  __O  unsigned long USBNDDRIntSet;
  __I  unsigned long USBSysErrIntSt;
  __O  unsigned long USBSysErrIntClr;
  __O  unsigned long USBSysErrIntSet;
       unsigned long RESERVED4[15];

  __I  unsigned long I2C_RX;                 /* USB OTG I2C Registers              */
  __O  unsigned long I2C_WO;
  __I  unsigned long I2C_STS;
  __IO unsigned long I2C_CTL;
  __IO unsigned long I2C_CLKHI;
  __O  unsigned long I2C_CLKLO;
       unsigned long RESERVED5[823];

  union {
  __IO unsigned long USBClkCtrl;             /* USB Clock Control Registers        */
  __IO unsigned long OTGClkCtrl;
  };
  union {
  __I  unsigned long USBClkSt;
  __I  unsigned long OTGClkSt;
  };
} LPC_USB_TypeDef;

extern LPC_USB_TypeDef *LPC_USB;
#define LPC_USB_BASE 0xFFE0C000

#define USBIntSt_USB_INT_REQ_LP 0x1
#define USBIntSt_USB_INT_REQ_HP 0x2
#define USBIntSt_USB_INT_REQ_DMA 0x4
#define USBIntSt_USB_HOST_INT 0x8
#define USBIntSt_USB_ATX_INT 0x10
#define USBIntSt_USB_OTG_INT 0x20
#define USBIntSt_USB_I2C_INT 0x40
#define USBIntSt_USB_NEED_CLK 0x100
#define USBIntSt_EN_USB_INTS 0x80000000

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


// prototypes
void usb_otg_initialize_host();
void usb_otg_initialize_device();
void usb_otg_device_int_control(int enable);

#endif // LPC17_USB_OTG_H
