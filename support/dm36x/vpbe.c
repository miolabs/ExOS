// VPBE Des-Controller for TMS320DM36x
// by anonymous

#include "vpbe.h"
#include "vpss.h"
#include "system.h"

static OSD_CONTROLLER  *_osd  = (OSD_CONTROLLER *) 0x01C71C00;
static VENC_CONTROLLER *_venc = (VENC_CONTROLLER *)0x01C71E00;


#define SDTV_NTSC  0
#define SDTV_PAL   1

#define HDTV_525P  0
#define HDTV_625P  1
#define HDTV_1080I 2
#define HDTV_720P  3

#define DIGITAL_OUT_YCC16   0
#define DIGITAL_OUT_YCC8    1
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

#define DAC_SELECT_CVBS    0
#define DAC_SELECT_SVID_Y  1
#define DAC_SELECT_SVID_C  2
#define DAC_SELECT_YG      3
#define DAC_SELECT_PbB     4
#define DAC_SELECT_PrR     5

void vpbe_initialize_simple  (VPBE_SIMPLE_SPEC *spec)
{
	int i;
	// Reset VPBE
	vpss_init (0);

	// Configure OSD

	// OSD enable

    // Configure VENC

	// VENC enable
	// -----------

	_venc->VMOD = ( 1 << 0) |  // VENC enable
	              (1 << 1) |   // Composite enable
	              (0 << 3) |   // Blanking normal/force
	              (0 << 4) |  // Video timing: PAL/NTSC/HDTV or not standard
	              (0 << 5) |  // Master mode
	              (SDTV_PAL << 6) |   // Mode
	              (0 << 8) |  // SDTV / HDTV
	              (1 << 9) |  // Non-standard modes: Progressive / interlaced
	              (0 << 10) |  // Standard modes: interlaced / non-interlaced
	              (0 << 11) |  // Non-interlace SDTV lines 312/313
	              (DIGITAL_OUT_PAR_RGB << 12);  // Digital video output mode

	// Video interface I/O control (can be all 0 for TV out)
	_venc->VIOCTL = (0<<0) |  // YOUT/COUT I/O; output or input
					(0<<2) |  // YOUT/COUT pin DC output mode; normal or DC level
					(0<<3) |  // Swaps YOUT/COUT; normal or interchaqnge
					(0<<4) |  // Digital data output mode: normal, inverse, L or H
					(0<<8) |  // HSYNC/VSYNC pin I/O control: output or input
					(0<<12) | // VCLK pin output enable: output or high impedance
					(0<<13) | // VCLK output enable: ? or DCLK
					(0<<14);	 // VCLK output polarity: normal or inverse

	// Filtrado del OSD -> VENC
	_venc->VDPRO = (0<<14) | // pre-filter select: no / 1+1 / 1+2+1 / reserved
	               (0<<12) | // Y pre-filter: no / 1+1 / 1+2+1 / reserved
	               (0<<11) | // pre-filter sampling freq: clock/2 or clock ?
	               (1<<9) | // Color bar type: 75% or 100%
	               (1<<8) | // Color bar mode, normal / color bar
	               (0<<7) | // DAC full-swing out normal / full swing
	               (0<<6) | // RGB input attenuation no / REC601
	               (0<<5) | // YCbCr input attenuation no / REC601
	               (0<<4) | // Composite input attenuation no / REC601
	               (0<<0);  // Chroma signal up sampling enable

	// Synchronism control; can be all 0 for TV out?
	_vecn->SYNCCTL = (0<<0) |  // Horizontal sync output enable
					 (0<<1) |  // Vertical sync output enable
					 (0<<2) |  // Horizontal sync output polarity. High or Low
					 (0<<3) |  // Vertical sync output polarity
					 (<<4) |  // VSYNC pin output signal select
					 (0<<5) |  // Output sync select; normal or SYNCPLS
					 (<<6) |  // Composite sync output enable. Off/On
					 (<<7) |  // Composite sync output polarity. H or L
					 (0<<8) |  // External horizontal sync input polarity. H or L
					 (0<<9) |  // External vertical sync input polarity. H or L
					 (0<<10) |  // External sync select. Effective in slave operation
					 (0<<11) |  // External field input inversion. Effective in slave operation
					 (0<<12) |  // External field input inversion. Effective in slave operation
					 (0<<14); // OSD vsync delay. 0 or 0.5H

	// ??
	_venc->HSPLS
	_venc->VSPLS

	//_venc->HSTART
	//_venc->HVALID	// number of ENC clocks
	//_venc->VSTART
	//_venc->VVALID // numer of lines

	//_venc->HINTVL  for nonstardard mode
	//_venc->VINTVL	 for nonstandard-mode
	//_venc->XHINTVL // Hor. interval extension. Effective only in 720p & 1080i


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
	// The VENC provides the sync signals to the OSD module
	//_venc->OSDCLK0 = 
	//_venc->OSDCLK1 = 

	_venc->CLKCTL = 
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

	//_venc->YCOLVL = 0; // DC output mode, levels ??

	//_venc->SCPROG = ??


	// YCbCr control (chroma out?)
	_venc->YCCCTL =   (0<<0); // REC656 mode no/yes
                     (0<<1) | // field toggle (REC656 mode)
                     (CHROMA_OUT_ORDER_8_BYRY <<2) |  // Yc output order
                     (0<<4);  // Chroma out: inmediate or latch

	_venc->RGBCTL =  (RGB_OUT_ORDER_RGB << 0) | // RGB output order ODD
					 (RGB_OUT_ORDER_RGB << 4) | // RGB output order EVEN
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
	_venc->CAPCTL =  (0<<0); // No caption; other fields ignored 
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

	// Composite mode fields ??
	_venc->CVBS = (0<<0) | // Sync build up time 140/200 microseconds
				  (0<<1) | // blanking build up time 140/300 microseconds
				  (0<<2) | // chroma low pass 1.5 mhz / 3 mhz
				  (0<<3) | // blanking shape disable (enable/disable) ??	
				  (0<<4) | // setup for composite 0% or 75%
				  (0<<5) | //  composite video level 286mv-714mv or 300mv-70mv
				  (0<<12); // delay adjust of Y signal

	// Component mode
	_venc->CMPNT = 0;

	// CVBS timing control
	_venc->ETMG0 = 0; // ??
	_venc->ETMG1 = 0; // ??
	_venc->ETMG2 = 0; // ??
	_venc->ETMG3 = 0; // ??

	_venc->DACSEL = (DAC_SELECT_CVBS<<0) |  // dac0 output select
					(DAC_SELECT_CVBS<<4) |  // dac1 output select
					(DAC_SELECT_CVBS<<8);   // dac2 output select

	_venc->ARGBX0 = 1024; // YCbCr->RGB matrix coefficient GY for analog RGB output. Default is 1024 (0x400)
	_venc->ARGBX1 = 1404; // YCbCr->RGB matrix coefficient RV for analog RGB output. Default is 1404 (0x57C)
	_venc->ARGBX2 = 345;  // YCbCr->RGB matrix coefficient GU for analog RGB output. Default is 345 (0x159)
	_venc->ARGBX3 = 715;  // YCbCr->RGB matrix coefficient GV for analog RGB output. Default is 715 (0x2CB).
	_venc->ARGBX4 = 1774; // YCbCr->RGB matrix coefficient BU for analog RGB output. Default is 1774 (0x2CB)

	_venc->DRGBX0 = 1024; // YCbCr->RGB matrix coefficient GY for analog RGB output. Default is 1024 (0x400)
	_venc->DRGBX1 = 1404; // YCbCr->RGB matrix coefficient RV for analog RGB output. Default is 1404 (0x57C)
	_venc->DRGBX2 = 345;  // YCbCr->RGB matrix coefficient GU for analog RGB output. Default is 345 (0x159)
	_venc->DRGBX3 = 715;  // YCbCr->RGB matrix coefficient GV for analog RGB output. Default is 715 (0x2CB).
	_venc->DRGBX4 = 1774; // YCbCr->RGB matrix coefficient BU for analog RGB output. Default is 1774 (0x2CB)

	_venc->VSTARTA = 0; // ?? Vertical data valid start position for even field. Specify the number of lines.
	_venc->VVALIDA = 0x1fff; // ?? Vertical data valid range for even field. 

	// Horizontal Valid Culling Control 0 ??
	_venc->HVLDCL0 = (0<<0) | // Horizontal valid culling pattern bit width.
					 (0<<4); // Horizontal valid culling mode (normal / culling mode)

	_venc->HVLDCL1 = 0; // Horizontal Valid Culling Pattern

	_venc->OSDHADV = 0; // Horizontal Sync Advance  ??

	_venc->GAMCTL = (0<<0) | // Gamma correction enable: No
					(0<<1) |  // Unique RGB gamma table mode
					(0<<4) | // DAC0 full-swing output Normal/full
					(0<<5) | // DAC1 full-swing output Normal/full
					(0<<6); // DAC2 full-swing output Normal/full

	// Attribute insertions
	_venc->BATR0 = 0;
   	_venc->BATR1 = 0;
	_venc->BATR2 = 0;
	_venc->BATR3 = 0;
	_venc->BATR4 = 0;
	_venc->BATR5 = 0;
	_venc->BATR6 = 0;
	_venc->BATR7 = 0;
	_venc->BATR8 = 0;

	// DAC gain & offset (in DAC full range output mode DAFUL=1) ?errata en manual?
	_venc->DACAMP = (0<<0) |
	                (0<<10);

/*
	// Linux venc setup

	// Reset video encoder module 
	_venc->VMOD = 0;
	// Enable Composite output and start video encoder 
	_venc->VMOD = (1 << 0) |  // VENC enable
	              (1 << 1);   // Composite enable;
	// Set REC656 Mode 
	_venc->YCCCTL = 0x1;
	// Enable output mode and PAL 
	_venc->VMOD = 0x1043;
	// Enable all DACs 
	_venc->DACTST = 0;
*/
}
