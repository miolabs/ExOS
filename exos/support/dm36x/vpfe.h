#ifndef DM36X_VPFE_H
#define DM36X_VPFE_H

// Image sensor interface ISIF registers
typedef volatile struct
{
	unsigned long SYNCEN;	// 0h Synchronization Enable
	unsigned long MODESET;	// 4h Mode Setup
	unsigned long HDW;		// 8h HD pulse width
	unsigned long VDW;		// Ch VD pulse width
	unsigned long PPLN;		// 10h Pixels per line
	unsigned long LPFR;		// 14h Lines per frame
	unsigned long SPH;		// 18h Start pixel horizontal
	unsigned long LNH;		// 1Ch Number of pixels in line
	unsigned long SLV0;		// 20h Start line vertical - field 0
	unsigned long SLV1;		// 24h Start line vertical - field 1
	unsigned long LNV;		// 28h Number of lines vertical
	unsigned long CULH;		// 2Ch Culling - horizontal
	unsigned long CULV;		// 30h Culling - vertical
	unsigned long HSIZE;	// 34h Horizontal size
	unsigned long SDOFST;	// 38h SDRAM Line Offset
	unsigned long CADU;		// 3Ch SDRAM Address - high
	unsigned long CADL;		// 40h SDRAM Address - low
	unsigned long reserved1;	// 44h 
	unsigned long reserved2;	// 48h 
	unsigned long CCOLP;		// 4Ch CCD Color Pattern
	unsigned long CRGAIN;		// 50h CCD Gain Adjustment - R/Ye
	unsigned long CGRGAIN;		// 54h CCD Gain Adjustment - Gr/Cy
	unsigned long CGBGAIN;		// 58h CCD Gain Adjustment - Gb/G
	unsigned long CBGAIN;		// 5Ch CCD Gain Adjustment - B/Mg
	unsigned long COFSTA;		// 60h CCD Offset Adjustment
	unsigned long FLSHCFG0;		// 64h FLSHCFG0
	unsigned long FLSHCFG1;		// 68h FLSHCFG1
	unsigned long FLSHCFG2;		// 6Ch FLSHCFG2
	unsigned long VDINT0;		// 70h VD Interrupt #0
	unsigned long VDINT1;		// 74h VD Interrupt #1
	unsigned long VDINT2;		// 78h VD Interrupt #2
	unsigned long reserved3;	// 7Ch
	unsigned long CGAMMAWD;		// 80h Gamma Correction settings
	unsigned long REC656IF;	// 84h CCIR 656 Control
	unsigned long CCDCFG;	// 88h CCD Configuration
	unsigned long DFCCTL;		// 8Ch Defect Correction - Control
	unsigned long VDFSATLV;		// 90h Defect Correction - Vertical Saturation Level
	unsigned long DFCMEMCTL;	// 94h Defect Correction - Memory Control
	unsigned long DFCMEM0;		// 98h Defect Correction - Set V Position 
	unsigned long DFCMEM1;		// 9Ch Defect Correction - Set H Position
	unsigned long DFCMEM2;		// A0h Defect Correction - Set SUB1
	unsigned long DFCMEM3;		// A4h Defect Correction - Set SUB2
	unsigned long DFCMEM4;		// A8h Defect Correction - Set SUB3
	unsigned long CLAMPCFG;		// ACh Black Clamp configuration
	unsigned long CLDCOFST;		// B0h DC offset for Black Clamp
	unsigned long CLSV;		// B4h Black Clamp Start position
	unsigned long CLHWIN0;		// B8h Horizontal Black Clamp configuration
	unsigned long CLHWIN1;		// BCh Horizontal Black Clamp configuration
	unsigned long CLHWIN2;		// C0h Horizontal Black Clamp configuration
	unsigned long CLVRV;		// C4h Vertical Black Clamp configuration
	unsigned long CLVWIN0;		// C8h Vertical Black Clamp configuration
	unsigned long CLVWIN1;		// CCh Vertical Black Clamp configuration
	unsigned long CLVWIN2;		// D0h Vertical Black Clamp configuration
	unsigned long CLVWIN3;		// D4h Vertical Black Clamp configuration
	unsigned long reserved4;		// D8h
	unsigned long reserved5;		// DCh
	unsigned long reserved6[3*4];	// E0h-10Ch
	unsigned long reserved7;		// 110h
	unsigned long FMTCFG;   //114h Not in manual??
	unsigned long reserved8;		// 118h
	unsigned long FMTSPH;	// 11Ch CCD Formatter - Start pixel horizontal
	unsigned long FMTLNH;	// 120h CCD Formatter - number of pixels
	unsigned long FMTSLV;	// 124h CCD Formatter - start line vertical
	unsigned long FMTLNV;	// 128h CCD Formatter - number of lines
	unsigned long FMTRLEN;	// 12Ch CCD Formatter - Read out line length
	unsigned long FMTHCNT;	// 130h CCD Formatter - HD cycles
	unsigned long reserved9;	// 134h
	unsigned long reservedA;	// 138h
	unsigned long reservedB;	// 13Ch
	unsigned long reservedC[6*4];	// 140h-19C
	unsigned long reservedD;	// 1A0h
	unsigned long CSCCTL;	// 1A4h Color Space Converter Enable
	unsigned long CSCM0;	// 1A8h Color Space Converter - Coefficients #0
	unsigned long CSCM1;	// 1ACh Color Space Converter - Coefficients #1
	unsigned long CSCM2;	// 1B0h Color Space Converter - Coefficients #2
	unsigned long CSCM3;	// 1B4h Color Space Converter - Coefficients #3
	unsigned long CSCM4;	// 1B8h Color Space Converter - Coefficients #4
	unsigned long CSCM5;	// 1BCh Color Space Converter - Coefficients #5
	unsigned long CSCM6;	// 1C0h Color Space Converter - Coefficients #6
	unsigned long CSCM7;	// 1C4h Color Space Converter - Coefficients #7

} ISIF_CONTROLLER;


// RESIZER registers
typedef volatile struct
{
    unsigned long SRC_EN; //0h RSZ Enable
    unsigned long SRC_MODE; //004h One Shot Mode
    unsigned long SRC_FMT0; //008h Input Data Paths
    unsigned long SRC_FMT1; //00Ch Source Image Format
    unsigned long SRC_VPS; //010h Vertical Start Position
    unsigned long SRC_VSZ; // 014h Vertical Processing Size
    unsigned long SRC_HPS; //018h Horizontal Start Position
    unsigned long SRC_HSZ; //01Ch Horizontal Processing Size
    unsigned long DMA_RZA; //020h SDRAM Request Minimum Interval for RZA
    unsigned long DMA_RZB; // 024h SDRAM Request Minimum Interval for RZB
    unsigned long DMA_STA; // 028h Status of Resizer (Reserved)
    unsigned long GCK_MMR; // 02Ch MMR Gated Clock Control
    unsigned long reserved1; // 030h Reserved
    unsigned long GCK_SDR; //034h SDR Gated Clock Control
    unsigned long IRQ_RZA; //038h Interval of RZA circular IRQ
    unsigned long IRQ_RZB; //03Ch Interval of RZB circular IRQ
    unsigned long YUV_Y_MIN; //040h Saturation (Luminance Minimum)
    unsigned long YUV_Y_MAX; //044h Saturation (Luminance Maximum)
    unsigned long YUV_C_MIN; // 048h Saturation (Chrominance Minimum)
    unsigned long YUV_C_MAX; // 04Ch Saturation (Chrominance Maximum)
    unsigned long YUV_PHS; // 050h Chrominance Position
    unsigned long SEQ; //054h Processing Mode
    unsigned long RZA_EN; //058h RZA (Resizer Channel A): Enable
    unsigned long RZA_MODE; //05Ch RZA: One Shot Mode
    unsigned long RZA_420; // 060h RZA: Output Format
    unsigned long RZA_I_VPS; // 064h RZA: Vertical Start Position of the Input
    unsigned long RZA_I_HPS; // 068h RZA: Horizontal Start Position of the Input
    unsigned long RZA_O_VSZ; //06Ch RZA: Vertical Size of the Output
    unsigned long RZA_O_HSZ; // 070h RZA: Horizontal Size of the Output
    unsigned long RZA_V_PHS_Y; //074h RZA: Initial Phase of Vertical Resizing Process for Luminance
    unsigned long RZA_V_PHS_C; //078h RZA: Initial Phase of Vertical Resizing Process for Chrominance
    unsigned long RZA_V_DIF; //07Ch RZA: Vertical Resize Parameter
    unsigned long RZA_V_TYP; //080h RZA: Interpolation method for Vertical Rescaling
    unsigned long RZA_V_LPF; //084h RZA: Vertical LPF Intensity
    unsigned long RZA_H_PHS; //088h RZA: Initial Phase of Horizontal Resizing Process
    unsigned long RZA_H_PHS_ADJ; //08Ch RZA: Additional Initial Phase of Vertical Resizing Process for Luminance
    unsigned long RZA_H_DIF; //090h RZA: Horizontal Resize Parameter
    unsigned long RZA_H_TYP; //094h RZA: Interpolation method for Horizontal Rescaling
    unsigned long RZA_H_LPF; //098h RZA: Horizontal LPF Intensity
    unsigned long RZA_DWN_EN; //09Ch RZA: Down Scale Mode Enable
    unsigned long RZA_DWN_AV; //0A0h RZA: Down Scale Mode Averaging Size
    unsigned long RZA_RGB_EN; //0A4h RZA: RGB Output Enable
    unsigned long RZA_RGB_TYP; //0A8h RZA: RGB Output Bit Mode
    unsigned long RZA_RGB_BLD; //0ACh RZA: YC422 to YC444 conversion method
    unsigned long RZA_SDR_Y_BAD_H; //0B0h RZA: SDRAM Base Address MSB
    unsigned long RZA_SDR_Y_BAD_L; // 0B4h RZA: SDRAM Base Address LSB
    unsigned long RZA_SDR_Y_SAD_H; //0B8h RZA: SDRAM Start Address MSB
    unsigned long RZA_SDR_Y_SAD_L; // 0BCh RZA: SDRAM Start Address LSB
    unsigned long RZA_SDR_Y_OFT; //0C0h RZA: SDRAM Line Offset
    unsigned long RZA_SDR_Y_PTR_S; //0C4h RZA: Start Line of SDRAM Pointer
    unsigned long RZA_SDR_Y_PTR_E; //0C8h RZA: End line of SDRAM Pointer
    unsigned long RZA_SDR_C_BAD_H; //0CCh RZA: SDRAM Base Address MSB (for 420 Chroma)
    unsigned long RZA_SDR_C_BAD_L; // 0D0h RZA: SDRAM Base Address LSB (for 420 Chroma)
    unsigned long RZA_SDR_C_SAD_H; //0D4h RZA: SDRAM Start Address MSB (for 420 Chroma)
    unsigned long RZA_SDR_C_SAD_L; //0D8h RZA: SDRAM Start Address LSB (for 420 Chroma)
    unsigned long RZA_SDR_C_OFT; // 0DCh RZA: SDRAM Line Offset (for 420 Chroma)
    unsigned long RZA_SDR_C_PTR_S; // 0E0h RZA: Start Line of SDRAM Pointer (for 420 Chroma)
    unsigned long RZA_SDR_C_PTR_E; //0E4h RZA: End line of SDRAM Pointer (for 420 Chroma)
    unsigned long RZB_EN; // 0E8h RZB (Resizer Channel B): Enable
    unsigned long RZB_MODE; //0ECh RZB: One Shot Mode
    unsigned long RZB_420; //0F0h RZB: Output Format
    unsigned long RZB_I_VPS; // 0F4h RZB: Vertical Start Position of the Input
    unsigned long RZB_I_HPS; //0F8h RZB: Horizontal Start Position of the Input
    unsigned long RZB_O_VSZ; // 0FCh RZB: Vertical Size of the Output
    unsigned long RZB_O_HSZ; //100h RZB: Horizontal Size of the Output
    unsigned long RZB_V_PHS_Y; //104h RZB: Initial Phase of Vertical Resizing Process for Luminance
    unsigned long RZB_V_PHS_C; //108h RZB: Initial Phase of Vertical Resizing Process for Chrominance
    unsigned long RZB_V_DIF; //10Ch RZB: Vertical Resize Parameter
    unsigned long RZB_V_TYP; // 110h RZB: Interpolation method for Vertical Rescaling
    unsigned long RZB_V_LPF; //114hRZB: Vertical LPF Intensity
    unsigned long RZB_H_PHS; //118h RZB: Initial Phase of Horizontal Resizing Process
    unsigned long RZB_H_PHS_ADJ; //11Ch RZB: Additional Initial Phase of Horizontal Resizing Process for Luminance
    unsigned long RZB_H_DIF; //120h RZB: Horizontal Resize Parameter
    unsigned long RZB_H_TYP; //124h RZB: Interpolation method for Horizontal Rescaling
    unsigned long RZB_H_LPF; //128hRZB: Horizontal LPF Intensity
    unsigned long RZB_DWN_EN; //12Ch RZB: Down Scale Mode Enable
    unsigned long RZB_DWN_AV; //130hRZB: Down Scale Mode Averaging Size
    unsigned long RZB_RGB_EN; //134hRZB: RGB Output Enable
    unsigned long RZB_RGB_TYP; //138h RZB: RGB Output Bit Mode
    unsigned long RZB_RGB_BLD; // 13Ch RZB: YC422 to YC444 conversion method
    unsigned long RZB_SDR_Y_BAD_H; //140hRZB: SDRAM Base Address MSB
    unsigned long RZB_SDR_Y_BAD_L; //144hRZB: SDRAM Base Address LSB
    unsigned long RZB_SDR_Y_SAD_H; //148h RZB: SDRAM Start Address MSB
    unsigned long RZB_SDR_Y_SAD_L; //14ChRZB: SDRAM Start Address LSB
    unsigned long RZB_SDR_Y_OFT; //150h RZB: SDRAM Line Offset
    unsigned long RZB_SDR_Y_PTR_S; //154h RZB: Start Line of SDRAM Pointer
    unsigned long RZB_SDR_Y_PTR_E; //158h RZB: End line of SDRAM Pointer
    unsigned long RZB_SDR_C_BAD_H; //15Ch RZB: SDRAM Base Address MSB (for 420 Chroma)
    unsigned long RZB_SDR_C_BAD_L; //160h RZB: SDRAM Base Address LSB (for 420 Chroma)
    unsigned long RZB_SDR_C_SAD_H; //164h RZB: SDRAM Start Address MSB (for 420 Chroma)
    unsigned long RZB_SDR_C_SAD_L; // 168h RZB: SDRAM Start Address LSB (for 420 Chroma)
    unsigned long RZB_SDR_C_OFT; //16Ch RZB: SDRAM Line Offset (for 420 Chroma)
    unsigned long RZB_SDR_C_PTR_S; //170h RZB: Start Line of SDRAM Pointer (for 420 Chroma)
    unsigned long RZB_SDR_C_PTR_E; //174h RZB: End line of SDRAM Pointer (for 420 Chroma)
} RESIZER_CONTROLLER;


// Image pipe interface IPIPEIF
typedef struct
{
    unsigned long ENABLE; // 0h IPIPE I/F Enable
    unsigned long CFG1; // 04h IPIPE I/F Configuration 1
    unsigned long PPLN; // 08h IPIPE I/F Interval of HD / Start pixel in HD
    unsigned long LPFR; // 0Ch IPIPE I/F Interval of VD / Start line in VD
    unsigned long HNUM; // 10h IPIPE I/F Number of valid pixels per line
    unsigned long VNUM; // 14h IPIPE I/F Number of valid lines per frame
    unsigned long ADDRU; // 18h IPIPE I/F Memory address (upper)
    unsigned long ADDRL; // 1Ch IPIPE I/F Memory address (lower)
    unsigned long ADOFS; // 20h IPIPE I/F Address offset of each line
    unsigned long RSZ; // 24h IPIPE I/F Horizontal resizing parameter
    unsigned long GAIN; // 28h IPIPE I/F Gain parameter
    unsigned long DPCM; // 2Ch IPIPE I/F DPCM configuration
    unsigned long CFG2; // 30h IPIPE I/F Configuration 2
    unsigned long INIRSZ; // 34h IPIPE I/F Initial position of resize
    unsigned long OCLIP; // 38h IPIPE I/F Output clipping value
    unsigned long DTUDF; // 3Ch IPIPE I/F Data underflow error status
    unsigned long CLKDIV; // 40h IPIPE I/F Clock rate configuration
    unsigned long DPC1; // 44h IPIPE I/F Defect pixel correction
    unsigned long DPC2; // 48h IPIPE I/F Defect pixel correction
    unsigned long reserved1; // 4ch
    unsigned long reserved2; // 50h
    unsigned long RSZ3A; // 54h IPIPE I/F Horizontal resizing parameter for H3A
    unsigned long INIRSZ3A; // 58h IPIPE I/F Initial position of resize for H3A
} IPIPEIF_CONTROLLER; 


// Hardware Image Signal Processor IPIPE registers
typedef struct
{
    unsigned long SRC_EN; // 0h IPIPE Enable
    unsigned long SRC_MODE; // 004h One Shot Mode
    unsigned long SRC_FMT; // 008h Input/Output Data Paths
    unsigned long SRC_COL; // 00Ch Color Pattern
    unsigned long SRC_VPS; // 010h Vertical Start Position
    unsigned long SRC_VSZ; // 014h Vertical Processing Size
    unsigned long SRC_HPS; // 018h Horizontal Start Position
    unsigned long SRC_HSZ; // 01Ch Horizontal Processing Size
    unsigned long reserved1; // 020h
    unsigned long DMA_STA; // 024h Status Flags (Reserved)
    unsigned long GCK_MMR; // 028h MMR Gated Clock Control
    unsigned long GCK_PIX; // 02Ch PCLK Gated Clock Control
    unsigned long reserved2; // 030h
    unsigned long DPC_LUT_EN; // 034h LUTDPC (=LUT Defect Pixel Correction): Enable
    unsigned long DPC_LUT_SEL; // 038h LUTDPC: Processing Mode Selection
    unsigned long DPC_LUT_ADR; // 03Ch LUTDPC: Start Address in LUT
    unsigned long DPC_LUT_SIZ; // 040h LUTDPC: Number of available entries in LUT
    unsigned long reserved3; // 044h
    unsigned long reserved4; // 048h
    unsigned long reserved5; // 04Ch
    unsigned long reserved6[8*4]; // 50h-1CCh 
    unsigned long WB2_OFT_R; // 1D0h WB2 (=White Balance): Offset
    unsigned long WB2_OFT_GR; // 1D4h WB2: Offset
    unsigned long WB2_OFT_GB; // 1D8h WB2: Offset
    unsigned long WB2_OFT_B; // 1DCh WB2: Offset
    unsigned long WB2_WGN_R; // 1E0h WB2: Gain
    unsigned long WB2_WGN_GR; // 1E4h WB2: Gain
    unsigned long WB2_WGN_GB; // 1E8h WB2: Gain
    unsigned long WB2_WGN_B; // 1ECh WB2: Gain
    unsigned long reserved7[3*4]; // 1F0h-21Ch
    unsigned long reserved8; // 220h
    unsigned long reserved9; // 224h
    unsigned long reservedA; // 228h
    unsigned long RGB1_MUL_RR; // 22Ch RGB1 (=1st RGB2RGB conv): Matrix Coefficient
    unsigned long RGB1_MUL_GR; // 230h RGB1: Matrix Coefficient
    unsigned long RGB1_MUL_BR; //234h RGB1: Matrix Coefficient
    unsigned long RGB1_MUL_RG; //238h RGB1: Matrix Coefficient
    unsigned long RGB1_MUL_GG; //23Ch RGB1: Matrix Coefficient
    unsigned long RGB1_MUL_BG; //240h RGB1: Matrix Coefficient
    unsigned long RGB1_MUL_RB; //244h RGB1: Matrix Coefficient
    unsigned long RGB1_MUL_GB; //248h RGB1: Matrix Coefficient
    unsigned long RGB1_MUL_BB; //24Ch RGB1: Matrix Coefficient
    unsigned long RGB1_OFT_OR; // 250h RGB1: Offset
    unsigned long RGB1_OFT_OG; // 254h RGB1: Offset
    unsigned long RGB1_OFT_OB; // 258h RGB1: Offset
    unsigned long GMM_CFG; // 25Ch Gamma Correction Configuration
    unsigned long reservedB[3*4]; // 260
    unsigned long reservedC; // 290h
    unsigned long YUV_ADJ; // 294h YUV (RGB2YCbCr conv): Luminance Adjustment (Contrast and Brightness)
    unsigned long YUV_MUL_RY; // 298h YUV: Matrix Coefficient
    unsigned long YUV_MUL_GY; // 29Ch YUV: Matrix Coefficient
    unsigned long YUV_MUL_BY; // 2A0h YUV: Matrix Coefficient
    unsigned long YUV_MUL_RCB; //2A4h YUV: Matrix Coefficient
    unsigned long YUV_MUL_GCB; // 2A8h YUV: Matrix Coefficient
    unsigned long YUV_MUL_BCB; // 2ACh YUV: Matrix Coefficient
    unsigned long YUV_MUL_RCR; // 2B0h YUV: Matrix Coefficient
    unsigned long YUV_MUL_GCR; // 2B4h YUV: Matrix Coefficient
    unsigned long YUV_MUL_BCR; // 2B8h YUV: Matrix Coefficient
    unsigned long YUV_OFT_Y; // 2BCh YUV: Offset
    unsigned long YUV_OFT_CB; // 2C0h YUV: Offset
    unsigned long YUV_OFT_CR; // 2C4h YUV: Offset
    unsigned long YUV_PHS; // 2C8h Chrominance Position (for 422 Down Sampler)
    unsigned long reservedD; // 2CCh
    unsigned long reservedE; // 2D0h
    unsigned long YEE_EN; // 2D4h YEE (=Edge Enhancer): Enable
    unsigned long YEE_TYP; // 2D8h YEE: Method Selection
    unsigned long YEE_SHF; // 7DCh YEE: HPF Shift Length
    unsigned long YEE_MUL_00; // 2E0h YEE: HPF Coefficient
    unsigned long YEE_MUL_01; // 2E4h YEE: HPF Coefficient
    unsigned long YEE_MUL_02; // 2E8h YEE: HPF Coefficient
    unsigned long YEE_MUL_10; // 2ECh YEE: HPF Coefficient
    unsigned long YEE_MUL_11; // 2F0h YEE: HPF Coefficient
    unsigned long YEE_MUL_12; // 2F4h YEE: HPF Coefficient
    unsigned long YEE_MUL_20; // 2F8h YEE: HPF Coefficient
    unsigned long YEE_MUL_21; // 2FCh YEE: HPF Coefficient
    unsigned long YEE_MUL_22; //300h YEE: HPF Coefficient
    unsigned long YEE_THR; //304h YEE: Lower Threshold before Referring to LUT
    unsigned long YEE_E_GAN; //308h YEE: Edge Sharpener Gain
    unsigned long YEE_E_THR_1; // 30Ch YEE: Edge Sharpener HP Value Lower Threshold
    unsigned long YEE_E_THR_2; // 310h YEE: Edge Sharpener HP Value Upper Limit
    unsigned long YEE_G_GAN; // 314h YEE: Edge Sharpener Gain on Gradient
    unsigned long YEE_G_OFT; //318h YEE: Edge Sharpener Offset on Gradient
    unsigned long reservedF; // 31Ch
    unsigned long reserved10[6]; // 320h-37Ch
    unsigned long BOX_EN; // 380h BOX (=Boxcar) Enable
    unsigned long BOX_MODE; // 384h BOX: One Shot Mode
    unsigned long BOX_TYP; // 388h BOX: Block Size (16x16 or 8x8)
    unsigned long BOX_SHF; // 38Ch BOX: Down Shift Value of Input
    unsigned long BOX_SDR_SAD_H; //390h BOX: SDRAM Address MSB
    unsigned long BOX_SDR_SAD_L; // 394h BOX: SDRAM Address LSB
    unsigned long Reserved; //398h
    unsigned long HST_EN; // 39Ch HST (=Histogram): Enable
    unsigned long HST_MODE; // 3A0h HST: One Shot Mode
    unsigned long HST_SEL; // 3A4h HST: Source Select
    unsigned long HST_PARA; // 3A8h HST: Parameters Select
    unsigned long HST_0_VPS; // 3ACh HST: Vertical Start Position
    unsigned long HST_0_VSZ; // 3B0h HST: Vertical Size
    unsigned long HST_0_HPS; // 3B4h HST: Horizontal Start Position
    unsigned long HST_0_HSZ; // 3B8h HST: Horizontal Size
    unsigned long HST_1_VPS; // 3BCh HST: Vertical Start Position
    unsigned long HST_1_VSZ; // 3C0h HST: Vertical Size
    unsigned long HST_1_HPS; // 3C4h HST: Horizontal Start Position
    unsigned long HST_1_HSZ; // 3C8h HST: Horizontal Size
    unsigned long HST_2_VPS; // 3CCh HST: Vertical Start Position
    unsigned long HST_2_VSZ; // 3D0h HST: Vertical Size
    unsigned long HST_2_HPS; // 3D4h HST: Horizontal Start Position
    unsigned long HST_2_HSZ; // 3D8h HST: Horizontal Size
    unsigned long HST_3_VPS; // 3DCh HST: Vertical Start Position
    unsigned long HST_3_VSZ; // 3E0h HST: Vertical Size
    unsigned long HST_3_HPS; // 3E4h HST: Horizontal Start Position
    unsigned long HST_3_HSZ; //3E8h HST: Horizontal Size
    unsigned long HST_TBL; //3ECh HST: Table Select
    unsigned long HST_MUL_R; //3F0h HST: Matrix Coefficient
    unsigned long HST_MUL_GR; // 3F4h HST: Matrix Coefficient
    unsigned long HST_MUL_GB; // 3F8h HST: Matrix Coefficient
    unsigned long HST_MUL_B; //3FCh HST: Matrix Coefficient
	
} IPIPE_CONTROLLER;


// 3A Statistics Generation registers
// Support control loops for auto focus, auto white balance, and auto exposure by collecting metrics about the imaging/video data
typedef struct
{
    unsigned long PID; // 0h Peripheral Revision and Class Information
    unsigned long PCR; // 04h Peripheral Control Register
    unsigned long AFPAX1; // 08h Setup for the AF Engine Paxel Configuration
    unsigned long AFPAX2; // 0Ch Setup for the AF Engine Paxel Configuration
    unsigned long AFPAXSTART; // 10h Start Position for AF Engine Paxels
    unsigned long reserved1; // 14h
    unsigned long AFBUFST; // 18h SDRAM/DDRAM Start address for AF Engine
    unsigned long reserved2; // 1ch
    unsigned long reserved3[2*4]; // 20h - 3Ch
    unsigned long reserved4; // 40h
    unsigned long reserved5; // 44h
    unsigned long reserved6; // 48h
    unsigned long AEWWIN1; // 4Ch Configuration for AE/AWB Windows
    unsigned long AEWINSTART; // 50h Start position for AE/AWB Windows
    unsigned long AEWINBLK; // 54h Start position and height for black line of AE/AWB Windows
    unsigned long AEWSUBWIN; // 58h Configuration for subsample data in AE/AWB window
    unsigned long AEWBUFST; // 5Ch SDRAM/DDRAM Start address for AE/AWB Engine Output Data
    unsigned long RSDR_ADDR; // 60h AE/AWB Engine Configuration
    unsigned long LINE_START; //64h Line start position for ISIF interface
    unsigned long VFV_CFG1; // 68h AF Vertical Focus Configuration 1 Register
    unsigned long VFV_CFG2; // 6Ch AF Vertical Focus Configuration 2 Register
    unsigned long VFV_CFG3; // 70h AF Vertical Focus Configuration 3 Register
    unsigned long VFV_CFG4; // 74h AF Vertical Focus Configuration 4 Register
    unsigned long HFV_THR; // 78h Configures the Horizontal Thresholds for the AF IIR filters
} H3A_CONTROLLER;


// Simple single screen configuration
typedef struct
{
/*	unsigned short Width;
	unsigned short Height;
	unsigned short Stride;
	void *Bitmap; */
} VPFE_SIMPLE_SPEC;

void vpfe_initialize_simple(VPFE_SIMPLE_SPEC *spec);



#endif // DM36X_VPFE_H

