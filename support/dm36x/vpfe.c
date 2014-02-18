// Video Processing Front End (VPFE) for TMS320DM36x

#include <assert.h>
#include <kernel/thread.h>

#include "vpfe.h"
#include "vpss.h"
#include "system.h"

static ISIF_CONTROLLER *_isif = (ISIF_CONTROLLER *)0x0;
static IPIPE_CONTROLLER *_ipie = (IPIPE_CONTROLLER *)0x0;
static IPIPEIF_CONTROLLER *_ipieif = (IPIPEIF_CONTROLLER *)0x0;
static RESIZER_CONTROLLER *_resizer = (RESIZER_CONTROLLER *)0x0;
static H3A_CONTROLLER *_h3a = (H3A_CONTROLLER *)0x0;


#if 0

static void _vpbe_config_osd()
{
	// Configure OSD
    _osd->MODE       = (210<<0) |   // Blackground color blue using clut in ROM0
					   (0<<8) |		// ROM Clut
					   (0<<9) |     // Field inversion
					   (0<<10) |	// Windows expansions (5 bits)
					   (0<<15);		// Color format Cb/Cr or Cr/Cb

    _osd->OSDWIN0MD  = 0;            // Disable both osd windows and cursor window
    _osd->OSDWIN1MD  = 0;
    _osd->RECTCUR    = 0;

	_osd->VIDWINMD = 0;

//    _osd->VIDWIN0OFST = ((_width * sizeof(OSD_PIXEL)) >> 5) |	// No. of 32 byte burst spans
//						((brg.ptrbits >> 28) << 9); // Addr. 4 msb
//    // High address is non-zero 
//    _osd->VIDWINADH = (video_buffer >> 16) & (0x7F); // 0x0000
//    // Added 16 bit address
//    _osd->VIDWIN0ADL = video_buffer & 0xFFFF; // Lower 16 bits
//
//    _osd->VIDWINADH  = 0x0000;
//    _osd->OSDWIN0ADL = 0x0000; /* Lower 16 bits */
	_osd->BASEPX     = 132;
	_osd->BASEPY     = 22;
//    _osd->VIDWIN0XP  = 0;
//    _osd->VIDWIN0YP  = 0;
//    _osd->VIDWIN0XL  = _width;
//    _osd->VIDWIN0YL  = _height >> 1;
//
//	// Window0 enable, window1 disable, no expansions
//    _osd->VIDWINMD   =  (1<<0) | // Window0 enable
//						(1<<1);  // Window0 field or frame
}


static void _vpbe_config_venc ()
{
	int i;

	// Clock
	_venc->OSDCLK0 = 1; // 2 bits pattern
	_venc->OSDCLK1 = 2; // Pattern = %10

	// LCD clocks
//	_venc->DCLKCTL = 0;
//	_venc->DCLKPTN0 = 0; 
//	_venc->DCLKPTN1 = 0;
//	_venc->DCLKPTN2 = 0;
//	_venc->DCLKPTN3 = 0;
//	_venc->DCLKPTN0A = 0;
//	_venc->DCLKPTN1A = 0;
//	_venc->DCLKPTN2A = 0;
//	_venc->DCLKPTN3A = 0;
//	_venc->DCLKHSTTA = 0; // Horizontal mask start position (ENC)	
//	_venc->DCLKHVLD = 0; // Horizontal mask range (ENC)
//	_venc->DCLKVSTT = 0; // Vertical mask start position (ENC)	
//	_venc->DCLKVVLD = 0; // Vertical mask range (ENC)

	_venc->YCOLVL = 0xffff; // For DC output mode only, levels	

	_venc->VMOD = VMOD_VENC |  // VENC enable
	              VMOD_VIE |   // Normal composite mode
	              (VMOD_TVTYP_PAL << VMOD_TVTYP_BIT) |   // Mode
	              (VDMD_VDMD_YCC8 << VMOD_VDMD_BIT);  // Digital video output mode

	// Video interface I/O control (can be all 0 for TV out?)
	_venc->VIOCTL = 0;

	// Filtrado del OSD -> VENC
	_venc->VDPRO = (VDPRO_PFLTC_NOFILTER << VDPRO_PFLTC_BIT) |
					(VDPRO_PFLTY_NOFILTER << VDPRO_PFLTY_BIT);

	// Synchronism control: only OVD applies to TVOUT
	_venc->SYNCCTL = 0; 

	// Sync pulses width only applies in non-std modes or sync proc. mode (SYSW=1)
	_venc->HSPLS = 0; // Hor. sync. pulse width
	_venc->VSPLS = 0; // vertical sync. pulse width

	// non-stardard mode only
//	_venc->HINTVL = 0;	
//	_venc->HSTART = 0; // Hor. valid data start position 
//	_venc->HVALID = 0; // number of ENC clocks
//	_venc->VINTVL = 0;  
//	_venc->VSTART = 0; // Ver. valid data start position 
//	_venc->VVALID = 0; // number of lines
//	_venc->XHINTVL = 0; // Hor. interval extension. Effective only in 720p & 1080i
	_venc->HSDLY = 0; // hsync delay in ENC clocks
	_venc->HSDLY = 0; // vsync delay in ENC clocks

	// YCbCr control (doesn't apply for tv out)
//	_venc->YCCCTL =  (1<<0); // REC656 mode no/yes
//                     (0<<1) | // field toggle (REC656 mode)
//                     (CHROMA_OUT_ORDER_8_BYRY <<2) |  // Yc output order
//                     (0<<4);  // Chroma out: inmediate or latch

//	_venc->RGBCTL =  (RGB_OUT_ORDER_RGB << 0) | // RGB output order ODD
//					 (RGB_OUT_ORDER_RGB << 4) | // RGB output order EVEN
//                     (0<<8) | // Low pass filter: no
//                     (0<<10) | // Sampling: ENC/2 or ENC (?)
//                     (0<<11) | // Ironman RGB enable
//                     (0<<12) | // Ironman 9bits enable
//                     (0<<13) | // Ironman swap
//                     (0<<15); // RGB latching

	// RGB clipping
//	_venc->RGBCLP =  (0<<0) |	// Offset
//                     (0xff<<8);	// upper limit

//	_venc->LINECTL = 0;
//	_venc->CULLLINE = 0; 

//	_venc->LCDOUT =  (0<<0) | // LCD_OE output control enable
//                     (0<<1) | // Polarity H/L
//                     (0<<2) | // bright output control enable
//                     (0<<3) | // bright polarity H/L
//                     (0<<4) | // lcd ac output control enable
//                     (0<<5) | // pwm output control enable
//                     (0<<6) | // pwm output polarity H/L
//                     (0<<7) | // Field ID output polarity  normal/inverse
//                     (0<<8); // LCD_OE or BRIGHT to alternate output

	// Captions
	_venc->CAPCTL =  (0<<0); // No caption; other fields ignored 
	_venc->CAPDO   =  0; 
	_venc->CAPDE   =  0; 
	_venc->ATR0   =  0; // Vertical blanking data insertion (teletexto)
	_venc->ATR1   =  0; 
	_venc->ATR2   =  0;

	//  Probably used only in nostandard mode
	_venc->SCPROG = 356; // Subcarrier initial phase NTSC 378 / PAL 356

	// Composite mode fields
	_venc->CVBS = 0;	// all standard

	// CVBS timing control (optional for TV out)
	//_venc->ETMG0 = 0; // T1: PAL & NTSC
	//_venc->ETMG1 = 0; // T2: PAL 151 / NTSC 141   T3: P212/N210 T4: P263/N243 T5: P1703/N1683
	//_venc->ETMG2 = 0; // 
	//_venc->ETMG3 = 0; // 

	for (i=0; i<64; i++)
		_venc->RAMADR =  i, // Gamma correction table address. ??
		_venc->RAMPORT = 0xffff;	// ??

	// Enable all DACs, normal output mode 
	_venc->DACTST = 0;	// no-dc, all on

	_venc->GAMCTL = 0;

	// Composite video output (CVBS).
	_venc->DACSEL = (DAC_SELECT_CVBS<<0) |  // dac0 output select
					(DAC_SELECT_CVBS<<4) |  // dac1 output select
					(DAC_SELECT_CVBS<<8);   // dac2 output select

	// Component mode
	//_venc->CMPNT = 0x8000; // For components video mode

	// Color conversion matrices (unnecesary for CVBS)
//	_venc->ARGBX0 = 1024; // YCbCr->RGB matrix coefficient GY for analog RGB output. Default is 1024 (0x400)
//	_venc->ARGBX1 = 1404; // YCbCr->RGB matrix coefficient RV for analog RGB output. Default is 1404 (0x57C)
//	_venc->ARGBX2 = 345;  // YCbCr->RGB matrix coefficient GU for analog RGB output. Default is 345 (0x159)
//	_venc->ARGBX3 = 715;  // YCbCr->RGB matrix coefficient GV for analog RGB output. Default is 715 (0x2CB).
//	_venc->ARGBX4 = 1774; // YCbCr->RGB matrix coefficient BU for analog RGB output. Default is 1774 (0x2CB)
//
//	_venc->DRGBX0 = 1024; // YCbCr->RGB matrix coefficient GY for analog RGB output. Default is 1024 (0x400)
//	_venc->DRGBX1 = 1404; // YCbCr->RGB matrix coefficient RV for analog RGB output. Default is 1404 (0x57C)
//	_venc->DRGBX2 = 345;  // YCbCr->RGB matrix coefficient GU for analog RGB output. Default is 345 (0x159)
//	_venc->DRGBX3 = 715;  // YCbCr->RGB matrix coefficient GV for analog RGB output. Default is 715 (0x2CB).
//	_venc->DRGBX4 = 1774; // YCbCr->RGB matrix coefficient BU for analog RGB output. Default is 1774 (0x2CB)
}

void vpbe_setup_osdwin(int win, VPBE_SIMPLE_SPEC *spec)
{
	unsigned long addr = (unsigned long)spec->Bitmap >> 5;
	_osd->OSDWIN0ADL = addr & 0xFFFF;
	_osd->OSDWINADHbits.O0AH = (addr >> 16) & 0x7F;
	_osd->OSDWIN0OFST = ((spec->Stride >> 5) & 0x1FF) | ((addr >> 23) << 9);

	_osd->OSDWIN0XP = 0;
	_osd->OSDWIN0YP = 0;
	_osd->OSDWIN0XL = spec->Width;
	_osd->OSDWIN0YL = spec->Height;

	_osd->OSDWIN0MD = OSDWIN0MD_OACT0 |
		OSDWIN0MD_OFF0 | // frame-mode
		7 << OSDWIN0MD_BLND0_BIT |
		OSDWIN_ZOOM_X1 << OSDWIN0MD_OVZ0_BIT |
		OSDWIN_ZOOM_X1 << OSDWIN0MD_OHZ0_BIT |
        OSDWIN_MD_RGB565 << OSDWIN0MD_BMP0MD_BIT;
}

void vpbe_initialize_simple(VPBE_SIMPLE_SPEC *spec)
{
	// Reset VPBE
	vpss_init(0);

	int venc_clk = system_get_sysclk(PLLC2, PLLC_SYSCLK5);

	// The VENC provides the sync signals to the OSD module
	_venc->CLKCTL = (1<<0) | // Clock enable for video encoder
					(0<<4) | // Clock enable for digital LCD encoder
					(0<<8); // Clock enable for gamma correction table?
	
	exos_thread_sleep(1);

	// OSD (frame buffer)

	_vpbe_config_osd();

	// VENC (video encoder digital->video signal)
    _vpbe_config_venc();

	_vbe_setup_osdwin(0, spec);
}






#endif