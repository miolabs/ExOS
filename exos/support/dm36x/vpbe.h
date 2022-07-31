#ifndef DM36X_VPBE_H
#define DM36X_VPBE_H

#define VMOD_VDMD_BIT 12
#define VDMD_VDMD_YCC16 0
#define VDMD_VDMD_YCC8 1
#define VDMD_VDMD_PRGB 2
#define VDMD_VDMD_SRGB 3
#define VMOD_ITLCL (1<<11)	// Non-interlace line count 
#define VMOD_ITLC (1<<10)	// Interlaced modes non-interlace
#define VMOD_NSIT (1<<9)	// Non-std interlace
#define VMOD_TVTYP_BIT 6
#define VMOD_TVTYP_NTSC 0
#define VMOD_TVTYP_PAL 1
#define VMOD_TVTYP_525P 0
#define VMOD_TVTYP_625P 1
#define VMOD_TVTYP_1080I 2
#define VMOD_TVTYP_720P 3
#define VMOD_SLAVE (1<<5)
#define VMOD_VMD (1<<4)	// Non-std modes
#define VMOD_BLNK (1<<3) // Force blanking
#define VMOD_VIE (1<<1) // Normal composite mode
#define VMOD_VENC (1<<0) // Video encoder enable

#define VDPRO_PFLTC_BIT 14
#define VDPRO_PFLTC_NOFILTER 0
#define VDPRO_PFLTC_1_1 1
#define VDPRO_PFLTC_1_2_1 2
#define VDPRO_PFLTY_BIT 12
#define VDPRO_PFLTY_NOFILTER 0
#define VDPRO_PFLTY_1_1 1
#define VDPRO_PFLTY_1_2_1 2
#define VDPRO_PFLTR (1<<11)
#define VDPRO_CBTYP (1<<9)	// Color-bar 100%
#define VDPRO_CBMD (1<<8)	// Color-bar mode enable
#define VDPRO_DAFUL (1<<7)	// Full-swing output
#define VDPRO_ATRGB (1<<6)	// RGB attenuation REC601
#define VDPRO_ATYCC (1<<5)	// YCbCr attenuation REC601
#define VDPRO_ATCOM (1<<4)	// Composite attenuation REC601
#define VDPRO_CUPS (1<<1)	// Chroma up-sampling
#define VDPRO_YUPS (1<<0)	// Y signal up-sampling

struct _CVBS_BITS
{
	unsigned CSBLD:1;
	unsigned CBBLD:1;
	unsigned CRCUT:1;
	unsigned CBLS:1;
	unsigned CSTUP:1;
	unsigned CVLVL:1;
	unsigned :6;
	signed CYDLY:3;
};

// VENC video encoder registers
typedef volatile struct
{
	unsigned long VMOD; // 0x0
	unsigned long VIOCTL; // 0x4
	unsigned long VDPRO;  // 0x8
	unsigned long SYNCCTL; // 0xc
	unsigned long HSPLS; // 0x10
	unsigned long VSPLS; // 0x14
	unsigned long HINTVL; // 0x18
	unsigned long HSTART; // 0x1c 
    unsigned long HVALID; // 0x20
    unsigned long VINTVL; // 0x24
    unsigned long VSTART; // 0x28 
    unsigned long VVALID; // 0x2c
    unsigned long HSDLY; // 0x30
    unsigned long VSDLY; // 0x34
    unsigned long YCCCTL; // 0x38
    unsigned long RGBCTL; // 0x3c
    unsigned long RGBCLP; // 0x40
    unsigned long LINECTL; // 0x44
    unsigned long CULLLINE; // 0x48
    unsigned long LCDOUT; // 0x4c
    unsigned long BRT0; // 0x50
    unsigned long BRT1; // 0x54
    unsigned long ACCTL; // 0x58
    unsigned long PWM0; // 0x5c
    unsigned long PWM1; // 0x60
    unsigned long DCLKCTL; // 0x64
    unsigned long DCLKPTN0; // 0x68
    unsigned long DCLKPTN1; // 0x6c
    unsigned long DCLKPTN2; // 0x70
    unsigned long DCLKPTN3; // 0x74
    unsigned long DCLKPTN0A; // 0x78
    unsigned long DCLKPTN1A; // 0x7C 
    unsigned long DCLKPTN2A; // 0x80
    unsigned long DCLKPTN3A; // 0x84
    unsigned long DCLKHSTT; // 0x88
    unsigned long DCLKHSTTA; // 0x8C
    unsigned long DCLKHVLD;  // 0x90
    unsigned long DCLKVSTT;  // 0x94
    unsigned long DCLKVVLD; // 0x98
    unsigned long CAPCTL; // 0x9C
    unsigned long CAPDO; // 0xa0
    unsigned long CAPDE; // 0xa4
    unsigned long ATR0; // 0xa8 
    unsigned long ATR1; // 0xac
    unsigned long ATR2; // 0xB0h
    unsigned long RSV0; // 0xB4h
    unsigned long VSTAT; // 0x B8h
    unsigned long RAMADR; // 0x BCh
	unsigned long RAMPORT; // 0x C0h
    unsigned long DACTST; // 0xC4h
	unsigned long YCOLVL; // 0xC8h
	unsigned long SCPROG; // 0x CCh
	unsigned long RSV1; // 0xD0h
	unsigned long RSV2; // 0xD4h
	unsigned long RSV3; // 0xD8h
	union
	{
		unsigned long CVBS; // 0xDCh
		struct _CVBS_BITS CVBSbits;
	};
	unsigned long CMPNT; // 0xE0h
	unsigned long ETMG0; // 0xE4h
	unsigned long ETMG1; // 0xE8h
	unsigned long ETMG2; // 0xECh
	unsigned long ETMG3; // 0xF0h
	unsigned long DACSEL; // 0xF4h
	unsigned long ReservedF8;
	unsigned long ReservedFC;
	unsigned long ARGBX0; // 0x100h
	unsigned long ARGBX1; // 0x104h
	unsigned long ARGBX2; // 0x108h
	unsigned long ARGBX3; // 0x10Ch
	unsigned long ARGBX4; // 0x110h
	unsigned long DRGBX0; // 0x114h
	unsigned long DRGBX1; // 0x118h
	unsigned long DRGBX2; // 0x11Ch
	unsigned long DRGBX3; // 0x120h
	unsigned long DRGBX4; // 0x124h
	unsigned long VSTARTA; // 0x128h
	unsigned long OSDCLK0; // 0x12Ch
	unsigned long OSDCLK1; // 0x130h
	unsigned long HVLDCL0; // 0x134h
	unsigned long HVLDCL1; // 0x138h
	unsigned long OSDHADV; // 0x13Ch
	unsigned long CLKCTL; // 0x140h
	unsigned long GAMCTL; // 0x144h
	unsigned long VVALIDA; // 0x148h
	unsigned long BATR0; // 0x14Ch
	unsigned long BATR1; // 0x150h
	unsigned long BATR2; // 0x154h
	unsigned long BATR3; // 0x158h
	unsigned long BATR4; // 0x15Ch
	unsigned long BATR5; // 0x160h
	unsigned long BATR6; // 0x164h
	unsigned long BATR7; // 0x168h
	unsigned long BATR8; // 0x16Ch
	unsigned long DACAMP; // 0x170h
	unsigned long XHINTVL; // 0x174h
} VENC_CONTROLLER;

#define OSDWIN_MD_BITMAP 0
#define OSDWIN_MD_RGB565 1
#define OSDWIN_MD_RGB888 2
#define OSDWIN_MD_YC 3

#define OSDWIN_ZOOM_X1 0
#define OSDWIN_ZOOM_X2 1
#define OSDWIN_ZOOM_X4 2

#define OSDWIN_BMW_1BIT 0
#define OSDWIN_BMW_2BIT 1
#define OSDWIN_BMW_4BIT 2
#define OSDWIN_BMW_8BIT 3

#define OSDWIN0MD_OACT0 (1<<0)
#define OSDWIN0MD_OFF0 (1<<1)
#define OSDWIN0MD_TE0 (1<<2)
#define OSDWIN0MD_BLND0_BIT 3
#define OSDWIN0MD_BMW0_BIT 6
#define OSDWIN0MD_OVZ0_BIT 8
#define OSDWIN0MD_OHZ0_BIT 10
#define OSDWIN0MD_CLUT_RAM (1<<12)
#define OSDWIN0MD_BMP0MD_BIT 13
#define OSDWIN0MD_BMPMDE (1<<15)

struct _WINADH_BITS
{
	unsigned O0AH:7;
	unsigned :1;
	unsigned O1AH:7;
	unsigned :1;
};

typedef volatile struct 
{
	unsigned long MODE; // 0x
	unsigned long VIDWINMD; // 0x4h 
	unsigned long OSDWIN0MD; // 0x8h 
	unsigned long OSDWIN1MD; // or OSDATRMD 0xc
	unsigned long RECTCUR; // 0x10
	unsigned long RSV0; // 0x14
	unsigned long VIDWIN0OFST; // 0x18
	unsigned long VIDWIN1OFST; // 0x1C
	unsigned long OSDWIN0OFST; // 0x20
	unsigned long OSDWIN1OFST; // 0x24
	union
	{
		unsigned long VIDWINADH; // 0x28
		struct _WINADH_BITS VIDWINADHbits;
	};
	unsigned long VIDWIN0ADL; // 0x2C 
	unsigned long VIDWIN1ADL; // 0x30
	union
	{
		unsigned long OSDWINADH; // 0x34
		struct _WINADH_BITS OSDWINADHbits;
	};
	unsigned long OSDWIN0ADL; // 0x38
	unsigned long OSDWIN1ADL; // 0x3C
	unsigned long BASEPX; // 0x40
	unsigned long BASEPY; // 0x44
	unsigned long VIDWIN0XP; // 0x48h
	unsigned long VIDWIN0YP; // 0x4C
	unsigned long VIDWIN0XL; // 0x50
	unsigned long VIDWIN0YL; // 0x54
	unsigned long VIDWIN1XP; // 0x58
	unsigned long VIDWIN1YP; // 0x5C
	unsigned long VIDWIN1XL; // 0x60
	unsigned long VIDWIN1YL; // 0x64
	unsigned long OSDWIN0XP; // 0x68
	unsigned long OSDWIN0YP; // 0x6C
	unsigned long OSDWIN0XL; // 0x70
	unsigned long OSDWIN0YL; // 0x74
	unsigned long OSDWIN1XP; // 0x78
	unsigned long OSDWIN1YP; // 0x7C
	unsigned long OSDWIN1XL; // 0x80
	unsigned long OSDWIN1YL; // 0x84
	unsigned long CURXP; // 0x88
	unsigned long CURYP; // 0x8C
	unsigned long CURXL; // 0x90
	unsigned long CURYL; // 0x94
	unsigned long RSV1; // 0x98
	unsigned long RSV2; // 0x9C
	unsigned long W0BMP01; // 0xA0
	unsigned long W0BMP23; // 0xA4
	unsigned long W0BMP45; // 0xA8
	unsigned long W0BMP67; // 0xAC
	unsigned long W0BMP89; // 0xB0
	unsigned long W0BMPAB; // 0xB4
	unsigned long W0BMPCD; // 0xB8
	unsigned long W0BMPEF; // 0xBC
	unsigned long W1BMP01; // 0xC0
	unsigned long W1BMP23; // 0xC4
	unsigned long W1BMP45; // 0xC8
	unsigned long W1BMP67; // 0xCC
	unsigned long W1BMP89; // 0xD0
	unsigned long W1BMPAB; // 0xD4
	unsigned long W1BMPCD; // 0xD8
	unsigned long W1BMPEF; // 0xDC
	unsigned long VBNDRY; // 0xE0
	unsigned long EXTMODE; // 0xE4
	unsigned long MISCCTL; // 0xE8
	unsigned long CLUTRAMYCB; // 0xEC
	unsigned long CLUTRAMCR; // 0xF0
	unsigned long TRANSPVALL; // 0xF4
	unsigned long TRANSPVALU; // 0xF8
	unsigned long TRANSPBMPIDX; // 0xFC
} OSD_CONTROLLER;

/*
#define CR_TA_BIT 2

typedef enum
{
	CR_ASIZE_8BIT = 0,
	CR_ASIZE_16BIT = 1,
} CR_ASIZE;

#define NANDFCR_CS2NAND (1<<0)
*/

// Simple single screen configuration
typedef struct
{
	unsigned short Width;
	unsigned short Height;
	unsigned short Stride;
	void *Bitmap;
} VPBE_SIMPLE_SPEC;

void vpbe_initialize_simple(VPBE_SIMPLE_SPEC *spec);

#endif // DM36X_VPBE_H
