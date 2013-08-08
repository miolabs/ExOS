// VPBE Des-Controller for TMS320DM36x
// by anonymous

#include <assert.h>
#include <kernel/thread.h>

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

typedef unsigned short OSD_PIXEL;

static inline void _reg_mod ( volatile unsigned long* reg, unsigned long val, unsigned long mask)
{
	*reg = (*reg & (~mask)) | (val & mask);
}

const int _basep_x = 132;
const int _basep_y = 22;
#define _width  (720)
#define _height  (480)

//static unsigned int videokk [_width * _height];

typedef union { unsigned int* ptr; unsigned int ptrbits;} BRIDGE;

static void _vpbe_config_osd ()
{
	// Configure OSD
    _osd->MODE       = (0xfc<<0) |   // Blackground color blue using clut in ROM0
					   (0<<8) |		// ROM Clut
					   (0<<9) |     // Field inversion
					   (0<<10) |	// Windows expansions (5 bits)
					   (0<<15);		// Color format Cb/Cr or Cr/Cb

    _osd->OSDWIN0MD  = 0;            // Disable both osd windows and cursor window
    _osd->OSDWIN1MD  = 0;
    _osd->RECTCUR    = 0;

    // AKN: Address are specified in multiple of 32 
    BRIDGE brg;
	//brg.ptr = videokk;
	brg.ptrbits = 0x80000000;
    unsigned int video_buffer = brg.ptrbits >> 5;

    _osd->VIDWIN0OFST = ((_width * sizeof(OSD_PIXEL)) >> 5) |	// No. of 32 byte burst spans
						((brg.ptrbits >> 28) << 9); // Addr. 4 msb
    // High address is non-zero 
    _osd->VIDWINADH = (video_buffer >> 16) & (0x7F); // 0x0000
    // Added 16 bit address
    _osd->VIDWIN0ADL = video_buffer & 0xFFFF; // Lower 16 bits

    _osd->VIDWINADH  = 0x0000;
    _osd->OSDWIN0ADL = 0x0000; /* Lower 16 bits */
    _osd->BASEPX     = _basep_x;
    _osd->BASEPY     = _basep_y;
    _osd->VIDWIN0XP  = 0;
    _osd->VIDWIN0YP  = 0;
    _osd->VIDWIN0XL  = _width;
    _osd->VIDWIN0YL  = _height >> 1;

	// Window0 enable, window1 disable, no expansions
    _osd->VIDWINMD   =  (1<<0) | // Window0 enable
						(1<<1);  // Window0 field or frame
}


// Configure VENC

// VENC enable requirements (from manual)
//----------------------------------------
//VPBE                  VPBE.CLK_OFF ??
//VENC                  VMOD.VIE
//Composite Analog Out  VMOD.VIE, CLKCTL.CLKENC
//Digital Output        VIOCTL.DOMD, CLKCTL.CLKDIG

static void _vpbe_config_venc ()
{
	int i;

	// Clock
	// The VENC provides the sync signals to the OSD module
	_venc->CLKCTL = (1<<0) | // Clock enable for video encoder
					(0<<4) | // Clock enable for digital LCD encoder
					(0<<8); // Clock enable for gamma correction table?
	
	exos_thread_sleep (1);

	_venc->OSDCLK0 = 1; // 2 bits pattern
	_venc->OSDCLK1 = 2; // Patter %10

	// LCD clocks
	_venc->DCLKCTL = 0;
	_venc->DCLKPTN0 = 0; 
	_venc->DCLKPTN1 = 0;
	_venc->DCLKPTN2 = 0;
	_venc->DCLKPTN3 = 0;
	_venc->DCLKPTN0A = 0;
	_venc->DCLKPTN1A = 0;
	_venc->DCLKPTN2A = 0;
	_venc->DCLKPTN3A = 0;
	_venc->DCLKHSTTA = 0; // Horizontal mask start position (ENC)	
	_venc->DCLKHVLD = 0; // Horizontal mask range (ENC)
	_venc->DCLKVSTT = 0; // Vertical mask start position (ENC)	
	_venc->DCLKVVLD = 0; // Vertical mask range (ENC)

	_venc->YCOLVL = 0xffff; // For DC output mode only, levels	

	_venc->VMOD = 0; // Reset video encoder

	_venc->VMOD = (1 << 0) |  // VENC enable
	              (1 << 1) |   // Composite enable
	              (0 << 3) |   // Blanking normal/force
	              (0 << 4) |  // Video timing: PAL/NTSC/HDTV or not standard
	              (0 << 5) |  // Master mode
	              (SDTV_PAL << 6) |   // Mode
	              (0 << 8) |  // SDTV / HDTV
	              (0 << 9) |  // Non-standard modes: Progressive / interlaced
	              (0 << 10) |  // Standard modes: interlaced / non-interlaced
	              (0 << 11) |  // Non-interlace SDTV lines 312/313
	              (DIGITAL_OUT_YCC8 << 12);  // Digital video output mode

	// Video interface I/O control (can be all 0 for TV out)
	_venc->VIOCTL = (0<<0) |  // YOUT/COUT I/O; output or input
					(0<<2) |  // YOUT/COUT pin DC output mode; normal or DC level
					(0<<3) |  // Swaps YOUT/COUT; normal or interchange
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
	_venc->SYNCCTL = (0<<0) |  // Horizontal sync output enable
					 (0<<1) |  // Vertical sync output enable
					 (0<<2) |  // Horizontal sync output polarity. High or Low
					 (0<<3) |  // Vertical sync output polarity
					 (0<<4) |  // VSYNC pin output signal select
					 (0<<5) |  // Output sync select; normal or SYNCPLS
					 (1<<6) |  // Composite sync output enable. Off/On
					 (0<<7) |  // Composite sync output polarity. H or L
					 (0<<8) |  // External horizontal sync input polarity. H or L
					 (0<<9) |  // External vertical sync input polarity. H or L
					 (0<<10) |  // External sync select. Effective in slave operation
					 (0<<11) |  // External field input inversion. Effective in slave operation
					 (0<<12) |  // External field input inversion. Effective in slave operation
					 (1<<14); // OSD vsync delay. 0 or 0.5H 

	// ??
	_venc->HSPLS = 0; // Hor. sync. pulse width
	_venc->VSPLS = 0; // vertical sync. pulse width

	// Probably for nonstardard mode only
	_venc->HINTVL = 0;  
	_venc->HSTART = 0; // Hor. valid data start position 
	_venc->HVALID = 0; // number of ENC clocks
	_venc->VINTVL = 0;  
	_venc->VSTART = 0; // Ver. valid data start position 
	_venc->VVALID = 0; // number of lines
	_venc->XHINTVL = 0; // Hor. interval extension. Effective only in 720p & 1080i
	_venc->HSDLY = 0; // hsync delay in ENC clocks
	_venc->HSDLY = 0; // vsync delay in ENC clocks

	_venc->LINECTL = 0;
	_venc->CULLLINE = 0; 

	//_venc->BRT0 =    // Bright pulse start position ??
	//_venc->BRT1 =    // Bright pulse width  ??

	// LCD_AC signal control
	//_venc->ACCTL = ??
	// _venc->PWM0
	// _venc->PWM1

	//  Probably used only in nostandard mode
	_venc->SCPROG = 356; // Subcarrier initial phase NTSC 378 / PAL 356


	// YCbCr control (chroma out?)
	_venc->YCCCTL =  (1<<0); // REC656 mode no/yes
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

	// Enable all DACs, normal output mode 
	_venc->DACTST = (0x3ff) | // DC level
					(0<<10) | // DC level enable (no)
					(0<<11) | // DAC normal/inverse 
					(0<<12) | // Power down: normal / power down 
					(0<<13) | // Power down: normal / power down 
					(0<<14);  // Power down: normal / power down 

	_venc->GAMCTL = (0<<0) | // Gamma correction enable: No
					(0<<1) |  // Unique RGB gamma table mode
					(0<<4) | // DAC0 full-swing output Normal/full
					(0<<5) | // DAC1 full-swing output Normal/full
					(0<<6); // DAC2 full-swing output Normal/full
	// DAC gain & offset (in DAC full range output mode DAFUL=1) ?errata en manual?
	_venc->DACAMP = (0<<0) |
	                (0<<10);

	// Composite video output (CVBS).
	_venc->DACSEL = (DAC_SELECT_CVBS<<0) |  // dac0 output select
					(DAC_SELECT_CVBS<<4) |  // dac1 output select
					(DAC_SELECT_CVBS<<8);   // dac2 output select

	// Composite mode fields. Probably not used in standard mode
	_venc->CVBS = (0<<0) | // Sync build up time 140/200 microseconds
				  (0<<1) | // blanking build up time 140/300 microseconds
				  (0<<2) | // chroma low pass 1.5 mhz / 3 mhz
				  (0<<3) | // blanking shape disable (enable/disable) ??	
				  (0<<4) | // setup for composite 0% or 75%
				  (0<<5) | //  composite video level 286mv-714mv or 300mv-70mv
				  (0<<12); // delay adjust of Y signal

	// Component mode
	//_venc->CMPNT = 0x8000; // For components video mode

	// CVBS timing control (for non-standard mode?)
	//_venc->ETMG0 = 0; // T1: PAL & NTSC
	//_venc->ETMG1 = 0; // T2: PAL 151 / NTSC 141   T3: P212/N210 T4: P263/N243 T5: P1703/N1683
	//_venc->ETMG2 = 0; // 
	//_venc->ETMG3 = 0; // 

	// Color conversion matrices (unnecesary for CVBS)
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

	// Probably for non-standard mode
	_venc->VSTARTA = 0; // Vertical data valid start position for even field. Specify the number of lines.
	_venc->VVALIDA = 0x1fff; //  Vertical data valid range for even field. 
	_venc->HVLDCL0 = (0<<0) | // Horizontal valid culling pattern bit width.
					 (0<<4); // Horizontal valid culling mode (normal / culling mode)
	_venc->HVLDCL1 = 0; // Horizontal Valid Culling Pattern

	_venc->OSDHADV = 0; // Horizontal Sync Advance  ??

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
}


void vpbe_initialize_simple  (VPBE_SIMPLE_SPEC *spec)
{
	// Reset VPBE
	vpss_init (0);

	// OSD (frame buffer)
	_vpbe_config_osd ();

	// VENC (video encoder digital->video signal)
    _vpbe_config_venc ();
}




/*
void vpbe_initialize_simple  (VPBE_SIMPLE_SPEC *spec)
{
#define VENC_VMOD_VENC				(1 << 0)

#define VENC_VMOD_VMD_SHIFT			4
#define VENC_VMOD_VMD				(1 << 4)

#define VENC_VMOD_TVTYP_SHIFT			6
#define VENC_VMOD_TVTYP				(3 << 6)

#define VENC_VMOD_VIE				(1 << 1)
#define VENC_VMOD_VIE_SHIFT			1

#define VENC_SYNCCTL_OVD_SHIFT			14
#define VENC_SYNCCTL_OVD			(1 << 14)

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

	// Setup clock at VPSS & VENC for SD 
	vpss_enable_clock(4);

	vpss_enable_clock ( VPSS_VENCCLKEN_ENABLE);
	vpss_enable_clock ( VPSS_DACCLKEN_ENABLE);

	// if (venc_type == VPBE_VERSION_2 && (type == VPBE_ENC_STD || (type == VPBE_ENC_DV_TIMINGS && pclock <= 27000000)))
	vpss_enable_clock(4);
	vpss_enable_clock(1);

	_venc->VMOD = 0;
	// disable VCLK output pin enable 
	_venc->VIOCTL = 0x141; // 0x2000 ??
//                  0001 0100 0001
//	_venc->VIOCTL = (1<<0) |  // YOUT/COUT I/O; output or input
//					(0<<2) |  // YOUT/COUT pin DC output mode; normal or DC level
//					(0<<3) |  // Swaps YOUT/COUT; normal or interchaqnge
//					(0<<4) |  // Digital data output mode: normal, inverse, L or H
//					(1<<8) |  // HSYNC/VSYNC pin I/O control: output or input
//					(0<<12) | // VCLK pin output enable: output or high impedance
//					(0<<13) | // VCLK output enable: ? or DCLK
//					(0<<14);	 // VCLK output polarity: normal or inverse

	// Disable output sync pins 
	_venc->SYNCCTL = 0;

	// Disable DCLOCK 
	_venc->DCLKCTL = 0;
	_venc->DRGBX1 = 0x0000057C;

	// Disable LCD output control (accepting default polarity) 
	_venc->LCDOUT = 0;
	// _venc->CMPNT = 0x100; // VPBE_VERSION_3
	_venc->HSPLS = 0;
	_venc->HINTVL = 0;
	_venc->HSTART = 0;
	_venc->HVALID = 0;

	_venc->VSPLS = 0;
	_venc->VINTVL = 0;
	_venc->VSTART = 0;
	_venc->VVALID = 0;

	_venc->HSDLY = 0;
	_venc->VSDLY = 0;

	_venc->YCCCTL = 0;
	_venc->VSTARTA = 0;

	// Set OSD clock and OSD Sync Adavance registers 
	_venc->OSDCLK0 = 1;
	_venc->OSDCLK1 = 2;

	// VPBE_VERSION_2
	_venc->CLKCTL = 0x1;
	_venc->VIOCTL = 0;

	_reg_mod(&_venc->SYNCCTL, 1 << VENC_SYNCCTL_OVD_SHIFT,VENC_SYNCCTL_OVD);
	_venc->VMOD = 0;
	_reg_mod(&_venc->VMOD,(1 << VENC_VMOD_VIE_SHIFT),VENC_VMOD_VIE);
	_reg_mod(&_venc->VMOD,(0 << VENC_VMOD_VMD_SHIFT), VENC_VMOD_VMD);
	_reg_mod(&_venc->VMOD,(1 << VENC_VMOD_TVTYP_SHIFT),VENC_VMOD_TVTYP);
	_venc->DACTST = 0x0;
	_reg_mod(&_venc->VMOD, VENC_VMOD_VENC, VENC_VMOD_VENC);

}

*/



