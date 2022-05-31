#ifndef DM36X_SYSTEM_H
#define DM36X_SYSTEM_H

#include "types.h"

struct _PERI_CLKCTL_BITS
{
	unsigned CLOCKOUT0EN:1;
	unsigned CLOCKOUT1EN:1;
	unsigned CLOCKOUT2EN:1;
   	unsigned DIV1:4;
	unsigned DIV2:9;
	unsigned DIV3:10;
	unsigned HDVICPCLKS:1;
	unsigned DDRCLKS:1;
	unsigned KEYSCLKS:1;
	unsigned ARMCLKS:1;
	unsigned PRTCCLKS:1;
};

struct _VDAC_CONFIG_BITS
{
	unsigned PWD_A:1;
	unsigned PWD_B:1;
	unsigned PWD_C:1;
	unsigned PWDNBUFZ:1;
	unsigned PWDNZ_TVDETECT:1;
	unsigned XDMODE:1;
	unsigned :13;
	unsigned PDTVSHORTZ:1;
	unsigned :10;
	unsigned TVINT:1;
	unsigned TVSHORT:1;
};

struct _VPSS_CLK_CTRL_BITS
{
	unsigned VPSS_MUXSEL:2;
	unsigned PCLK_INV:1;
	unsigned VENCLKEN:1;
	unsigned DACCLKEN:1;
	unsigned VENC_CLK_SRC:2;
	unsigned VPSS_CLKMD:1;
};

#define VPSS_MUXSEL_PLLC_CLK 0
#define VPSS_MUXSEL_EXTCLK 2
#define VPSS_MUXSEL_PCLK 3

#define VENC_CLK_SRC_PLLC1SYSCLK6 0
#define VENC_CLK_SRC_PLLC2SYSCLK5 1
#define VENC_CLK_SRC_MXI 2

typedef volatile struct
{
	unsigned long PINMUX0;
	union
	{
		unsigned long PINMUX1;
		struct
		{
			unsigned COUT7:2;
			unsigned COUT6:2;
			unsigned COUT5:2;
			unsigned COUT4:2;
			unsigned COUT3:2;
			unsigned COUT2:2;
			unsigned COUT1:2;
			unsigned COUT0:2;
			unsigned HVSYNC:1;
			unsigned LCD_OE:1;
			unsigned FIELD:2;
			unsigned EXTCLK:2;
			unsigned VCLK:1;
		} PINMUX1bits;
	};
	unsigned long PINMUX2;
	union
	{
		unsigned long PINMUX3;
		struct
		{
			unsigned GIO1:1;
			unsigned GIO2:1;
			unsigned GIO3:1;
			unsigned GIO4:1;
			unsigned GIO5:1;
			unsigned GIO6:1;
			unsigned GIO7:1;
			unsigned GIO8:1;
			unsigned GIO9:1;
			unsigned GIO10:1;
			unsigned GIO11:1;
			unsigned GIO12:1;
			unsigned GIO13:1;
			unsigned GIO14:1;
			unsigned GIO15:1;
			unsigned GIO16:2;
			unsigned GIO17:2;
			unsigned GIO18:1;
			unsigned GIO19:1;
			unsigned GIO20:2;
			unsigned GIO21:2;
			unsigned GIO22:1;
			unsigned GIO23:2;
			unsigned GIO24:1;
			unsigned GIO25:2;
			unsigned GIO26:1;
		} PINMUX3bits;
	};
	union
	{
		unsigned long PINMUX4;
		struct
		{
			unsigned GIO27:2;
			unsigned GIO28:2;
			unsigned GIO29:2;
			unsigned GIO30:2;
			unsigned GIO31:2;
			unsigned GIO32:2;
			unsigned GIO33:2;
			unsigned GIO34:2;
			unsigned GIO35:2;
			unsigned GIO36:2;
			unsigned GIO37:2;
			unsigned GIO38:2;
			unsigned GIO39:2;
			unsigned GIO40:2;
			unsigned GIO41:2;
			unsigned GIO42:2;
		} PINMUX4bits;
	};
	unsigned long BOOTCFG;
	union
	{
		unsigned long ARM_INTMUX;
		struct
		{
			unsigned INT10:1;
			unsigned INT13:1;
            unsigned INT17:1;
            unsigned INT18:1;
            unsigned INT19:1;
            unsigned INT20:1;
            unsigned :1;
            unsigned INT24:1;
            unsigned INT26:1;
            unsigned INT28:1;
            unsigned INT29:1;
            unsigned INT30:1;
            unsigned INT38:1;
            unsigned INT43:1;
            unsigned INT52:1;
            unsigned INT53:1;
            unsigned INT54:1;
            unsigned INT55:1;
            unsigned INT56:1;
            unsigned INT57:1;
            unsigned INT58:1;
            unsigned INT59:1;
            unsigned INT61:1;
            unsigned INT62:1;
            unsigned INT8:1;
            unsigned INT7:1;
            unsigned :1;
            unsigned INT0:1;
		} ARM_INTMUXbits;
	};
	unsigned long EDMA_EVTMUX;
   	unsigned long Reserved20;
	unsigned long HPI_CTL;
	unsigned long DEVICE_ID;
	union
	{
		unsigned long VDAC_CONFIG;
        struct _VDAC_CONFIG_BITS VDAC_CONFIGbits;
	};
	unsigned long TIMER64_CTL;
	unsigned long USB_PHY_CTRL;
    unsigned long MISC;
    unsigned long MSTPRI0;
	unsigned long MSTPRI1;
	union
	{
		unsigned long VPSS_CLK_CTRL;
		struct _VPSS_CLK_CTRL_BITS VPSS_CLK_CTRLbits;
	};
	union
	{
		unsigned long PERI_CLKCTL;
		struct _PERI_CLKCTL_BITS PERI_CLKCTLbits;
	};
    unsigned long DEEPSLEEP;
    unsigned long Reserved50;
    unsigned long DEBOUNCE0;
    unsigned long DEBOUNCE1;
    unsigned long DEBOUNCE2;
    unsigned long DEBOUNCE3;
    unsigned long DEBOUNCE4;
    unsigned long DEBOUNCE5;
    unsigned long DEBOUNCE6;
    unsigned long DEBOUNCE7;
    unsigned long VTPIOCR;
    unsigned long PUPDCTL0;
    unsigned long PUPDCTL1;
    unsigned long HDVICPBT;
    unsigned long PLLC1_CONFIG;
    unsigned long PLLC2_CONFIG;
} SYSTEM_CONTROLLER;

#define VTPIO_CLRZ (1<<13)
#define VTPIO_LOCK (1<<7)
#define VTPIO_IOPWRDN (1<<14)
#define VTPIO_PWRDN (1<<6)
#define VTPIO_READY (1<<15)

#define PLLC_CONFIG_LOCK123_BIT 25
#define PLLC_CONFIG_LOCK123_MASK (0x7 << PLLC_CONFIG_LOCK123_BIT)

// POWER AND SLEEP Controller
//////////////////////////////////////
typedef enum
{
	PSC_MODULE_EDMA_CC = 0,
	PSC_MODULE_EDMA_TC0,
	PSC_MODULE_EDMA_TC1,
	PSC_MODULE_EDMA_TC2,
	PSC_MODULE_EDMA_TC3,
	PSC_MODULE_TIMER3,
	PSC_MODULE_SPI1,
	PSC_MODULE_MMC_SD1,
	PSC_MODULE_McBSP,
	PSC_MODULE_USB,
	PSC_MODULE_PWM3,
   	PSC_MODULE_SPI2,
	PSC_MODULE_RTO,
	PSC_MODULE_DDR_EMIF,
	PSC_MODULE_AEMIF,
	PSC_MODULE_MMC_SD0,
	PSC_MODULE_RESERVED16,
	PSC_MODULE_TIMER4,
	PSC_MODULE_I2C,
	PSC_MODULE_UART0,
	PSC_MODULE_UART1,
	PSC_MODULE_HPI,
	PSC_MODULE_SPI0,
	PSC_MODULE_PWM0,
	PSC_MODULE_PWM1,
	PSC_MODULE_PWM2,
	PSC_MODULE_GPIO,
   	PSC_MODULE_TIMER0,
   	PSC_MODULE_TIMER1,
   	PSC_MODULE_TIMER2,
	PSC_MODULE_SYSTEM,
   	PSC_MODULE_ARM,
   	PSC_MODULE_RESERVED32,
   	PSC_MODULE_RESERVED33,
   	PSC_MODULE_RESERVED34,
   	PSC_MODULE_EMULATION,
   	PSC_MODULE_RESERVED36,
   	PSC_MODULE_RESERVED37,
   	PSC_MODULE_SPI3,
   	PSC_MODULE_SPI4,
   	PSC_MODULE_EMAC,
   	PSC_MODULE_PRTCIF,
   	PSC_MODULE_KEYSCAN,
   	PSC_MODULE_ADC,
   	PSC_MODULE_VOICE_CODEC,
   	PSC_MODULE_VDAC_CLKREC,
   	PSC_MODULE_VDAC_CLK,
   	PSC_MODULE_VPSS_MASTER,
   	PSC_MODULE_RESERVED48,
   	PSC_MODULE_RESERVED49,
   	PSC_MODULE_MJCP,
   	PSC_MODULE_HDVICP,
} PSC_MODULE;

typedef volatile struct
{
	unsigned long PID;
	unsigned long Reserved004[5];
	unsigned long INTEVAL;
	unsigned long Reserved01C[9];
	unsigned long MERRPR0;
	unsigned long MERRPR1;
	unsigned long Reserved048;
	unsigned long Reserved04C;
	unsigned long MERRCR0;
	unsigned long MERRCR1;
	unsigned long Reserved058[50];
	unsigned long PTCMD;
	unsigned long Reserved124;
	unsigned long PTSTAT;
	unsigned long Reserved12C[437];
	unsigned long MDSTAT[128];
	unsigned long MDCTL[128];
} PSC_CONTROLLER;

#define PTCMD_GOSET (1<<0)

#define PTSTAT_GOSTAT (1<<0)

#define MDCTL_NEXT_MASK (0x1F)
#define MDCTL_LRST (1<<8)
#define MDCTL_EMURSTIE (1<<9)
#define MDCTL_EMUIHBIE (1<<10)

typedef enum
{
	PSC_MODULE_SW_RESET_DISABLE = 0,
	PSC_MODULE_SYNC_RESET,
	PSC_MODULE_DISABLE,
	PSC_MODULE_ENABLE,
} PSC_MODULE_STATE;

// PLL Controllers
//////////////////////////////////////

typedef enum
{
	PLLC1 = 0, 
	PLLC2
} PLLC_INDEX;

typedef enum
{
	PLLC_SYSCLK1 = 0,
	PLLC_SYSCLK2,
	PLLC_SYSCLK3,
	PLLC_SYSCLK4,
	PLLC_SYSCLK5,
	PLLC_SYSCLK6,
	PLLC_SYSCLK7,
	PLLC_SYSCLK8,
	PLLC_SYSCLK9,
} PLLC_SYSCLK_INDEX;

struct _PLLCTL_BITS
{
	unsigned PLLEN:1;
	unsigned PLLPWRDN:1;
	unsigned :1;
	unsigned PLLRST:1;
	unsigned :1;
	unsigned PLLENSRC:1;
	unsigned :1;
};

typedef volatile struct
{
	unsigned long PID;
	unsigned long Reserved004[56];
	unsigned long RSTYPE;
	unsigned long Reserved0E8[6];
	union
	{
		unsigned long PLLCTL;
		struct _PLLCTL_BITS PLLCTLbits;
	};
	unsigned long OCSEL;
	unsigned long PLLSECCTL;
	unsigned long Reserved10C;
	unsigned long PLLM;
	unsigned long PREDIV;
	unsigned long PLLDIV1;
	unsigned long PLLDIV2;
	unsigned long PLLDIV3;
	unsigned long OSCDIV1;
	unsigned long POSTDIV;
	unsigned long BPDIV;
	unsigned long Reserved130;
	unsigned long Reserved134;
	unsigned long PLLCMD;
	unsigned long PLLSTAT;
	unsigned long ALNCTL;
	unsigned long DCHANGE;
	unsigned long CKEN;
	unsigned long CKSTAT;
	unsigned long SYSTAT;
	unsigned long Reserved154;
	unsigned long Reserved158;
	unsigned long Reserved15C;
	unsigned long PLLDIV4;
	unsigned long PLLDIV5;
	unsigned long PLLDIV6;
	unsigned long PLLDIV7;
	unsigned long PLLDIV8;
	unsigned long PLLDIV9;
} PLL_CONTROLLER;

#define PLLCMD_GOSET (1<<0)

#define PLLSTAT_GOSTAT (1<<0)
#define PLLSTAT_LOCK (1<<1)
#define PLLSTAT_STABLE (1<<2)

#define PLLDIV_EN (1<<15)
#define PLLDIV_RATIO_MASK (0x1F)

#define PLLSECCTL_TINITZ (1<<16)
#define PLLSECCTL_TENABLE (1<<17)
#define PLLSECCTL_TENABLEDIV (1<<18)

// prototypes
void system_select_armss_clock(PLLC_INDEX plli);
void system_select_ddr2_clock(PLLC_INDEX plli);
void system_perform_vtpio_calibration();
void system_select_pinmux(int gio, int func);
void system_select_intmux(int number, int func);
int system_get_sysclk(PLLC_INDEX plli, PLLC_SYSCLK_INDEX sysi);

//int  system_vpss_enable_clock ( unsigned long mask);

void psc_set_module_state(PSC_MODULE module, PSC_MODULE_STATE state);
PSC_MODULE_STATE psc_get_module_state(PSC_MODULE module);

void pllc_set_divider(PLLC_INDEX plli, PLLC_SYSCLK_INDEX index, int ratio);
void pllc_setup(PLLC_INDEX plli, int pre_div, int multiplier, int post_div, int sysclkdiv[], int sysclkdiv_count);

#endif // DM36X_SYSTEM_H

