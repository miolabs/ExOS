#ifndef DM36X_AEMIF_H
#define DM36X_AEMIF_H

typedef volatile struct
{
	unsigned long Reserved00;
	unsigned long AWCCR;
	unsigned long Reserved08;
	unsigned long Reserved0C;
	unsigned long A1CR;
	unsigned long A2CR;
	unsigned long Reserved18;
	unsigned long Reserved1C;
	unsigned long Reserved20[8];
	unsigned long EIRR;
	unsigned long EIMR;
	unsigned long EIMSR;
	unsigned long EIMCR;
	unsigned long Reserved50;
	unsigned long Reserved54;
	unsigned long Reserved58;
	unsigned long ONENANDCTL;
	unsigned long NANDFCR;
	unsigned long NANDFSR;
	unsigned long Reserved68;
	unsigned long Reserved6C;
	unsigned long NANDF1ECC;
	unsigned long NANDF2ECC;
	unsigned long Reserved78;
	unsigned long Reserved7C;
	unsigned long Reserver80[12];
	unsigned long ReservedB0;
	unsigned long ReservedB4;
	unsigned long ReservedB8;
	unsigned long NAND4BITECCLOAD;
	unsigned long NAND4BITECC1;
	unsigned long NAND4BITECC2;
	unsigned long NAND4BITECC3;
	unsigned long NAND4BITECC4;
	unsigned long NANDERRADD1;
	unsigned long NANDERRADD2;
	unsigned long NANDERRVAL1;
	unsigned long NANDERRVAL2;
} AEMIF_CONTROLLER;

#define CR_TA_BIT 2
#define CR_RHOLD_BIT 4
#define CR_RSTROBE_BIT 7
#define CR_RSETUP_BIT 13
#define CR_WHOLD_BIT 17
#define CR_WSTROBE_BIT 20
#define CR_WSETUP_BIT 26
#define CR_EW (1<<30)
#define CR_SS (1<<31)

typedef enum
{
	CR_ASIZE_8BIT = 0,
	CR_ASIZE_16BIT = 1,
} CR_ASIZE;

#define NANDFCR_CS2NAND (1<<0)
#define NANDFCR_CS3NAND (1<<1)
#define NANDFCR_CS2ECC (1<<8)
#define NANDFCR_CS3ECC (1<<9)

#define NANDFSR_WAITST0 (1<<0)
#define NANDFSR_WAITST1 (1<<1)
#define NANDFSR_WAITST2 (1<<2)
#define NANDFSR_WAITST3 (1<<3)

typedef struct
{
	unsigned char trp;		// read pulse width
	unsigned char trea_max;	// read enable access time
	unsigned char tcea_max;	// CE low to output valid (access time)
	unsigned char tchz_max;	// CE high to output Z
	unsigned char trc;		// read cycle time
	unsigned char trhz_max;	// read CE high to output Z
	unsigned char tclr;		// cmd latch to read

	unsigned char twp;		// write pulse width
	unsigned char tcls;		// CLE setup
	unsigned char tclh;		// CLE hold time
	unsigned char tals;		// ALE setup
	unsigned char talh;		// ALE hold time
	unsigned char tcs;		// CS setup
	unsigned char tch;		// CS hold time
	unsigned char tds;		// data setup
	unsigned char tdh;		// data hold time
	unsigned char twc;		// write cycle time
} AEMIF_SPEC;

extern volatile unsigned char *__aemif_nand_data8;
#define AEMIF_NAND_DATA8 (*__aemif_nand_data8)
extern volatile unsigned char *__aemif_nand_cle8;
#define AEMIF_NAND_CLE8 (*__aemif_nand_cle8)
extern volatile unsigned char *__aemif_nand_ale8;
#define AEMIF_NAND_ALE8 (*__aemif_nand_ale8)

extern volatile unsigned long *__aemif_nand_data;
#define AEMIF_NAND_DATA (*__aemif_nand_data)
extern volatile unsigned long *__aemif_nand_cle;
#define AEMIF_NAND_CLE (*__aemif_nand_cle)
extern volatile unsigned long *__aemif_nand_ale;
#define AEMIF_NAND_ALE (*__aemif_nand_ale)

void aemif_initialize_nand(AEMIF_SPEC *spec, int cs, unsigned int tclk);

#endif // DM36X_AEMIF_H
