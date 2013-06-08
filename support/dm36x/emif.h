#ifndef DM36X_EMIF_H
#define DM36X_EMIF_H

typedef volatile struct
{
	unsigned long Reserved0;
	unsigned long SDRSTAT;
	unsigned long SDCR;
	unsigned long SDRCR;
	unsigned long SDTIMR;
	unsigned long SDTIMR2;
	unsigned long Reserved18;
	unsigned long SDCR2;
	unsigned long PBBPR;
	unsigned long Reserved24[39];
	unsigned long IRR;
	unsigned long IMR;
	unsigned long IMSR;
	unsigned long IMCR;
	unsigned long ReservedD0[5];
	unsigned long DDRPHYCR1;
} EMIF_CONTROLLER;

#define SDCR_BOOTUNLOCK (1<<23)
#define SDCR_DDR_DDQS (1<<22)
#define SDCR_DDR2EN (1<<20)
#define SDCR_DDREN (1<<17)
#define SDCR_SDRAMEN (1<<16)
#define SDCR_TIMUNLOCK (1<<15)
#define SDCR_NM (1<<14)
#define SDCR_CL_BIT 9
#define SDCR_IBANK_BIT 4
#define SDCR_PAGESIZE_BIT 0

typedef enum
{
	SDCR_IBANK_ONE = 0,
	SDCR_IBANK_TWO,
	SDCR_IBANK_FOUR,
	SDCR_IBANK_EIGHT,
} SDCR_IBANK_CODE;

typedef enum
{
	SDCR_PAGESIZE_1024 = 2,
} SDCR_PAGESIZE_CODE;

#define SDRCR_SR_PD (1<<23)
#define SDRCR_MCLKSTOPEN (1<<30)
#define SDRCD_LPMODEN (1<<31) 

#define SDTIMR_T_WTR_BIT 0
#define SDTIMR_T_RRD_BIT 3
#define SDTIMR_T_RC_BIT 6
#define SDTIMR_T_RAS_BIT 11
#define SDTIMR_T_WR_BIT 16
#define SDTIMR_T_RCD_BIT 19
#define SDTIMR_T_RP_BIT 22
#define SDTIMR_T_RFC_BIT 25

#define SDTIMR2_T_CKE_BIT 0
#define SDTIMR2_T_RTP_BIT 5
#define SDTIMR2_T_XSRD_BIT 8
#define SDTIMR2_T_XSNR_BIT 16
#define SDTIMR2_T_XP_BIT 25
#define SDTIMR2_T_RASMAX_BIT 27

#define DDRPHYCR1_CONFIG_EXT_STRBEN (1<<7)
#define DDRPHYCR1_CONFIG_PWRDNEN (1<<6)

typedef struct 
{
	unsigned int tref;
	unsigned int trfc;
	unsigned int trp;
	unsigned int trcd;
	unsigned int twr;
	unsigned int tras;
	unsigned int trc;
	unsigned int trrd;
	unsigned int twtr;
	unsigned int trasmax;
	unsigned int txp;
	unsigned int txsnr;
	unsigned int txsrd;
	unsigned int trtp;
	unsigned int tcke;
} EMIF_SPEC;

// prototypes
void emif_initialize_ddr2(EMIF_SPEC *spec, unsigned int tclk);


#endif // DM36X_EMIF_H
