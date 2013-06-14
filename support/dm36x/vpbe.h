#ifndef DM36X_VPBE_H
#define DM36X_VPBE_H

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
} VENC_CONTROLLER;


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
	unsigned long VIDWINADH; // 0x28
	unsigned long VIDWIN0ADL; // 0x2C 
	unsigned long VIDWIN1ADL; // 0x30
	unsigned long OSDWINADH; // 0x34
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
  unsigned char a;

} VPBE_SIMPLE_SPEC;

void vpbe_initialize_simple (VPBE_SIMPLE_SPEC *spec);

#endif // DM36X_VPBE_H
