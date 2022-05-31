#include <net/tcp_io.h>
#include <comm/comm.h>
#include <kernel/tree.h>
#include <support/board_hal.h>
#include <kernel/thread.h>

#include <support/dm36x/vpbe.h>

#include <stdio.h>

static unsigned short _bitmap[720*576] __attribute__((__aligned__(32)));

#define RGB565(r,g,b) ((((r) >> 3) << 11) | (((g) >> 2) << 5) | ((b) >> 3))

int main()
{
	VPBE_SIMPLE_SPEC test = (VPBE_SIMPLE_SPEC) { .Width = 320, .Height = 256, .Stride = 320 * 2, .Bitmap = _bitmap };

	//hal_board_init_pinmux(HAL_RESOURCE_TVOUT, 0);

	exos_thread_sleep(20);

	int off = 0;
	for(int r = 0; r < 16; r++)
	{
		for (int rep = 0; rep < 16; rep++)
		{
			for (int g = 0; g < 16; g++)
			{
				for (int b = 0; b < 16; b++)
				{
					_bitmap[off++] = RGB565(r << 4, g << 4, b << 4);
				}
			}
			off += (test.Stride >> 1) - 256;
		}
	}
	
	vpbe_initialize_simple(&test);
	while (1)
	{
		
	}
}


#if 0

#include <kernel/tree.h>
#include <support/board_hal.h>
#include <kernel/thread.h>
#include "support/dm36x/vpbe.h"

#include <stdio.h>

#define VPBE_ENABLE

#define NTSC            1
#define PAL             0

#define COLORBARS       1
#define LOOPBACK        0

#define SVIDEO_OUT      1
#define COMPOSITE_OUT   0

/* AKN: Added video memory base and size */
#define DDR_BASE            (0x80000000)
#define DDR_VIDMEM_OFFSET   (0x6000000U)
#define DDR_VIDMEM_SIZE     (0xA8C00U) /*720X480X2BPP*/

#define VPSS_VPBE_CLK_CTRL    *( volatile unsigned long* )( 0x1c70200 )
#define VDAC_CONFIG   *( volatile unsigned long* )( 0x01C4002c )
#define VPSS_CLKCTL   *( volatile unsigned long* )( 0x01C40044 )
#define PLL0_CONFIG   *( volatile unsigned long* )(0x1c40084)

#define OSD_BASE            (0x1c71c00)

#define OSD_MODE    *( volatile unsigned long* )( OSD_BASE + 0x0 )
#define OSD_VIDWINADH_H    *( volatile unsigned long* )( OSD_BASE + 0x28 )
#define OSD_OSDWIN0MD    *( volatile unsigned long* )( OSD_BASE + 0x8 )
#define OSD_OSDWIN1MD    *( volatile unsigned long* )( OSD_BASE + 0xc )
#define OSD_RECTCUR    *( volatile unsigned long* )( OSD_BASE + 0x10 )
#define OSD_VIDWIN0OFST    *( volatile unsigned long* )( OSD_BASE + 0x18 )
#define OSD_VIDWIN0ADR    *( volatile unsigned long* )( OSD_BASE + 0x2c )
#define OSD_VIDWINADH    *( volatile unsigned long* )( OSD_BASE + 0x28 )
#define OSD_OSDWIN0ADL    *( volatile unsigned long* )( OSD_BASE + 0x38 )
#define OSD_BASEPX    *( volatile unsigned long* )( OSD_BASE + 0x40 )
#define OSD_BASEPY    *( volatile unsigned long* )( OSD_BASE + 0x44 )
#define OSD_VIDWIN0XP    *( volatile unsigned long* )( OSD_BASE + 0x48 )
#define OSD_VIDWIN0YP    *( volatile unsigned long* )( OSD_BASE + 0x4c )
#define OSD_VIDWIN0XL    *( volatile unsigned long* )( OSD_BASE + 0x50 )
#define OSD_VIDWIN0YL    *( volatile unsigned long* )( OSD_BASE + 0x54 )
#define OSD_VIDWINMD    *( volatile unsigned long* )( OSD_BASE + 0x4 )
#define OSD_MISCCTL   *( volatile unsigned long* )( OSD_BASE + 0xe8 )

#define VENC_BASE  (0x1c71e00)

#define VENC_VDPRO  *( volatile unsigned long* )( VENC_BASE + 0x8 )
#define VENC_VMOD *( volatile unsigned long* )( VENC_BASE + 0x0 )
#define VENC_DACTST *( volatile unsigned long* )( VENC_BASE + 0xc4 )
#define VENC_CMPNT *( volatile unsigned long* )( VENC_BASE + 0xe0 )
#define VENC_DACSEL *( volatile unsigned long* )( VENC_BASE + 0xf4 )
#define VENC_VIOCTL *( volatile unsigned long* )( VENC_BASE + 0x4 )
#define VENC_DCLKCTL *( volatile unsigned long* )( VENC_BASE + 0x64 )
#define VENC_DCLKPTN0 *( volatile unsigned long* )( VENC_BASE + 0x68 )
#define VENC_CLKCTL  *( volatile unsigned long* )( VENC_BASE + 0x140 )

#define SYSTEM_BASE (0x01C40000) 
#define PINMUX1  *( volatile unsigned long* )( SYSTEM_BASE + 0x4 )

#define YUV_DUMP 1 /* Dumps YUV data to frame buffer */

/* AKN: Added to offset in the SDRAM for video memory */
unsigned long video_buffer = DDR_BASE + DDR_VIDMEM_OFFSET;

static void _wait(int count)
{
	for (int volatile i = 0; i < count; i++);
}

static unsigned char *pu8FrameBufPtr; /* AKN: Pointer for frame buffer */

void Setup_PLL0_NTSC()
{
    volatile unsigned int* pll_ctl       = ( volatile unsigned int* )( 0x01c40d00 );
    volatile unsigned int* pll_secctl    = ( volatile unsigned int* )( 0x01c40d08 );
    volatile unsigned int* pll_cmd       = ( volatile unsigned int* )( 0x01c40d38 );
    volatile unsigned int* pll_div5      = ( volatile unsigned int* )( 0x01c40d64 );

    //if (0x20 == ((*pll_ctl) & (0x20)))   printf("PLL is already powered up \n");

    /* Reconfigure SYSCLK6 divider for VENC = 74.25MHz */
    *pll_ctl &= ~0x0002;              // Power up PLL
    *pll_ctl |=  0x0010;              // Put PLL in disable mode
    *pll_ctl &= ~0x0010;              // Take PLL out of disable mode

    *pll_ctl &= ~0x0020;             // Clear PLLENSRC
    *pll_ctl &= ~0x0001;             // Set PLL in bypass
    _wait(50);

    *pll_ctl |= 0x0008;              // Assert PLL reset
    *pll_ctl &= ~0x0008;             // Take PLL out of reset

    *pll_ctl &= ~0x0010;             // Enable PLL
    _wait(50);        // Wait for PLL to stabilize

    exos_thread_sleep(100 );

    *pll_secctl  = 0x00470000;       // Assert TENABLE = 1, TENABLEDIV = 1, TINITZ = 1 
    *pll_secctl  = 0x00460000;       // Assert TENABLE = 1, TENABLEDIV = 1, TINITZ = 0 
    *pll_secctl  = 0x00400000;       // Assert TENABLE = 0, TENABLEDIV = 0, TINITZ = 0 
    *pll_secctl  = 0x00410000;       // Assert TENABLE = 0, TENABLEDIV = 0, TINITZ = 1

    *pll_div5    = 0x8015;           // 594/8 -> 74.25MHz VENC

    *pll_cmd |= 0x0001;              // Set GOSET

    _wait(50);

    while(! (((PLL0_CONFIG) & 0x07000000) == 0x07000000));  // Wait for PLL to lock

    _wait(50);
    *pll_ctl = 0x0001;               // Enable PLL

}


static void vpbe_init( unsigned long colorbar_loopback_mode, unsigned long ntsc_pal_mode, unsigned long output_mode )
{
    unsigned long basep_x;
    unsigned long basep_y;
    unsigned long width;
    unsigned long height;
    unsigned long test;
    unsigned long u32WidthComp;

    if ( ntsc_pal_mode == NTSC )
    {
        basep_x = 122;
        basep_y = 18;
        width   = 720;
        height  = 480;
    }
    else
    {
        basep_x = 132;
        basep_y = 22;
        width   = 720;
        height  = 480;
    }


    VDAC_CONFIG         = /*0x081141CF*/0x101941DC;   // Take DACs out of power down mode
    VPSS_CLKCTL         = 0x00000038;   // Enable DAC and VENC clock, both at 27 MHz
    VENC_CLKCTL         = 0x00000001;   // Enable venc & digital LCD clock
    VPSS_VPBE_CLK_CTRL  = 0x00000011;   // Select enc_clk*1, turn on VPBE clk


    OSD_MODE       = 0x000000fc;   // Background color blue using clut in ROM0

    OSD_OSDWIN0MD  = 0;            // Disable both osd windows and cursor window
    OSD_OSDWIN1MD  = 0;
    OSD_RECTCUR    = 0;

    /* AKN: Address are specified in multiple of 32 */
    video_buffer = video_buffer >> 5;

    OSD_VIDWIN0OFST = 0x1000 | (width >> 4);
    /* AKN: High address is non-zero */
    OSD_VIDWINADH_H = (video_buffer >> 16) & (0x7F);/*0x0000*/
    /* AKN: Added 16 bit address */
    OSD_VIDWIN0ADR = video_buffer & 0xFFFF/*0x0000*/; /* Lower 16 bits */

    OSD_VIDWINADH  = 0x0000;
    OSD_OSDWIN0ADL = 0x0000; /* Lower 16 bits */
    OSD_BASEPX     = basep_x;
    OSD_BASEPY     = basep_y;
    OSD_VIDWIN0XP  = 0;
    OSD_VIDWIN0YP  = 0;
    OSD_VIDWIN0XL  = width;
    OSD_VIDWIN0YL  = height >> 1;

    OSD_VIDWINMD   = 0x00000003;   // Disable vwindow 1 and enable vwindow 0
                                        // Frame mode with no up-scaling

    /*
     *  Setup VENC
     */
    if ( ntsc_pal_mode == NTSC )
        VENC_VMOD  = 0x00000003;   // Standard NTSC interlaced output
    else
        VENC_VMOD  = 0x00000043;   // Standard PAL interlaced output

    VENC_VDPRO     = colorbar_loopback_mode << 8;
    VENC_VDPRO    |= 0x200;  // 100% Color bars
    VENC_DACTST    = 0;
    VENC_CMPNT |= 0x8000; /* Component video RGB */

    /*
     *  Choose Output mode
     */
    if ( output_mode == COMPOSITE_OUT )
        VENC_DACSEL = 0x00000000; 
    else if ( output_mode == SVIDEO_OUT )
        VENC_DACSEL = 0x00004210;

}

int main( int argc, char** argv)
{

    unsigned long u32Cnt;

    /* Reconfigure SYSCLK6 divider for VENC = 27MHz */
    //Setup_PLL0_NTSC();

#ifdef VPBE_ENABLE
    /* Setup Front-End */
    //EVMDM365_CPLD_rset(3, 0x05);  // Select TVP5416 on decoder MUX
    //tvp5146_init( NTSC, COMPOSITE_OUT );
#endif

    pu8FrameBufPtr = (unsigned char *)video_buffer; /* Pointer for frame buffer */

     /* Setup Back-End */
    vpbe_init( LOOPBACK, NTSC, COMPOSITE_OUT );

    VENC_VIOCTL = 0x2000;    // Enable VCLK (VIDCTL)
    VENC_DCLKCTL = 0x0800;   // Enable DCLK (DCLKCTL)
    VENC_DCLKPTN0 = 0x0001;  // Set DCLK pattern (DCLKPTN0)
    PINMUX1 &= ~0x00400000;  // Set PINMUX for VCLK
    PINMUX1 |= 0XAA00;
    OSD_MISCCTL |= 0x10;

    /* Populate the frame buffer with YUV422 data */

	/* Copy one frame of YUV file one by one */
	for (u32Cnt = 0; u32Cnt < DDR_VIDMEM_SIZE; u32Cnt++)
	{
		*pu8FrameBufPtr++ = u32Cnt;            
		pu8FrameBufPtr++;
	}

	while (1);

    return 0;
}

#endif





