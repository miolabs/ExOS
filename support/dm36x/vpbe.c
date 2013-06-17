// VPBE Des-Controller for TMS320DM36x
// by anonymous

#include "vpbe.h"
#include "vpss.h"
#include "system.h"

static OSD_CONTROLLER  *_osd  = (OSD_CONTROLLER *) 0x01C71C00;
static VENC_CONTROLLER *_venc = (VENC_CONTROLLER *)0x01C71E00;

void vpbe_initialize_simple  (VPBE_SIMPLE_SPEC *spec)
{
	// Reset VPBE
	vpss_init (0);

	// Configure OSD

	// OSD enable

    // Configure VENC

	// VENC enable
	// -----------

	_venc->VMOD = ( 1 << 0) |  // VENC enable
	              (1 << 1) |   // Composite enable
	              (0 << 3) |	  // Blanking normal/force
	              (0 << 4) |  // Video timing: PAL/NTSC/HDTV or not standard
	              (0 << 5) |  // Master mode
	              (SDTV_PAL << 6) |   // Mode
	              (0 << 8) |  // SDTV / HDTV
	              (1 << 9) |  // Non-standard modes: Progressive / interlaced
	              (0 << 10) |  // Standard modes: interlaced / non-interlaced
	              (0 << 11) |  // Non-interlace SDTV lines 312/313
	              (DIGITAL_OUT_PAR_RGB << 12);  // Digital video output mode

	// ??
	//_venc->VIOCTL = 

	// Filtrado del OSD -> VENC
	_venc->VDPRO = (0<<14) | // pre-filter select: no / 1+1 / 1+2+1 / reserved
	               (0<<12) | // Y pre-filter: no / 1+1 / 1+2+1 / reserved
	               (0<<11) | // pre-filter sampling freq: clock/2 or clock ?
	               (0<<9) | // Color bar type: 75% or 100%
	               (0<<8) | // Color bar mode, normal / color bar (!!??)
	               (0<<7) | // DAC full-swing out normal / full swing
	               (0<<6) | // RGB input attenuation no / REC601
	               (0<<5) | // YCbCr input attenuation no / REC601
	               (0<<4) | // Composite input attenuation no / REC601
	               (0<<0);  // Chroma signal up sampling enable

	// ??
	//_vecn->SYNCCTL = (<<) |

	// ??
	//_venc->HSPLS
	//_venc->VSPLS

	//_venc->HSTART
	//_venc->HVALID	// number of ENC clocks
	//_venc->VSTART
	//_venc->VVALID // numer of lines

	//_venc->HINTVL  for nonstardard mode
	//_venc->VINTVL	 for nonstandard-mode

	//_venc->HSDLY  // hsync delay in ENC clocks
	//_venc->HSDLY  // vsync delay in ENC clocks

	//_venc->LINECTL =  
	//_venc->CULLLINE =  

	//_venc->BRT0 =    // Bright pulse start position ??
	//_venc->BRT1 =    // Bright pulse width  ??

	// LCD_AC signal control
	//_venc->ACCTL = ??
	// _venc->PWM0
	// _venc->PWM1

	// Clock
	//_venc->DCLK = 
	//_venc->DCLKPTN0 = 
	//_venc->DCLKPTN1 = 
	//_venc->DCLKPTN2 = 
	//_venc->DCLKPTN3 = 
	//_venc->DCLKPTN0A = 
	//_venc->DCLKPTN1A = 
	//_venc->DCLKPTN2A = 
	//_venc->DCLKPTN3A = 
	//_venc->DCLKHSTTA = // Horizontal mask start position (ENC)	
	//_venc->DCLKHVLD = // Horizontal mask range (ENC)
	//_venc->DCLKVSTTA = // Vertical mask start position (ENC)	
	//_venc->DCLKVVLD = // Vertical mask range (ENC)

	// YCbCr control (chroma out?)
	venc->YCCCTL =   (0<<0); // REC656 mode no/yes
                     (0<<1) | // field toggle (REC656 mode)
                     (CHROMA_OUT_ORDER_8_BYRY <<2) |  // Yc output order
                     (0<<4) |  // Chroma out: inmediate or latch

	venc->RGBCTL =   (0<<RGB_OUT_ORDER_RGB); // RGB output order ODD
					 (4<<RGB_OUT_ORDER_RGB); // RGB output order EVEN
                     (0<<8) | // Low pass filter: no
                     (0<<10) | // Sampling: ENC/2 or ENC (?)
                     (0<<11) | // Ironman RGB enable
                     (0<<12) | // Ironman 9bits enable
                     (0<<13) | // Ironman swap
                     (0<<15); // RGB latching
 
	// RGB clipping
	_venc->RGBCLP =  (0<<0) |	// Offset
                     (0xff<<8);	// upper limit


	_venc->LCDOUT =  (0<<0) | // LCD_OE output control enable
                     (0<<1) | // Polarity H/L
                     (0<<2) | // bright output control enable
                     (0<<3) | // bright polarity H/L
                     (0<<4) | // lcd ac output control enable
                     (0<<5) | // pwm output control enable
                     (0<<6) | // pwm output polarity H/L
                     (0<<7) | // Field ID output polarity  normal/inverse
                     (0<<8); // LCD_OE or BRIGHT to alternate output

	// Captions
	_venc->CAPTCTL =  (0<<0); // No caption; other fields ignored 
	_venc->CAPDO   =  0; 
	_venc->CAPDE   =  0; 

	_venc->ATR0   =  0; // Vertical blanking data insertion (teletexto)
	_venc->ATR1   =  0; 
	_venc->ATR2   =  0; 
	//_venc->VSTAT   // Read only, status de los captions 

	for (i=0; i<64; i++)
		_venc->RAMADR =  i, // Gamma correction table address. ??
		_venc->RAMPORT = 0xffff;	// ??

	_venc->DACTST = (0x3ff) | // DC level
					(0<<10) | // ?? 
					(0<<11) | // DAC normal/inverse 
					(0<<12) | // Power down: normal / power down 
					(0<<13) | // Power down: normal / power down 
					(0<<14);  // Power down: normal / power down 

	_venc->YCOLLV = 0; // DC output mode, levels ??

}

#define SDTV_NTSC  0
#define SDTV_PAL   1

#define HDTV_525P  0
#define HDTV_625P  1
#define HDTV_1080I 2
#define HDTV_720P  3

#define DIGITAL_OUT_YCC16   0
#define DIGITAL_OUT_ YCC8   1
#define DIGITAL_OUT_PAR_RGB 2
#define DIGITAL_OUT_SER_RGB 3

#define CHROMA_OUT_ORDER_16_BR    0  // CbCr
#define CHROMA_OUT_ORDER_16_RB    1  // CrCb
#define CHROMA_OUT_ORDER_8_BYRY   0  // Cb-Y-Cr-Y
#define CHROMA_OUT_ORDER_8_YRYB   1  // Y-Cr-Y-Cb 
#define CHROMA_OUT_ORDER_8_RYBY   2  // Cr-Y-Cb-Y 
#define CHROMA_OUT_ORDER_8_YBYR   3  // Y-Cb-Y-Cr

#define RGB_OUT_ORDER_RGB    0
#define RGB_OUT_ORDER_RBG    1
#define RGB_OUT_ORDER_GRB    2
#define RGB_OUT_ORDER_GBR    3
#define RGB_OUT_ORDER_BRG    4
#define RGB_OUT_ORDER_BGR    5