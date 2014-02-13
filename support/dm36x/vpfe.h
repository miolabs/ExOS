#ifndef DM36X_VPFE_H
#define DM36X_VPFE_H

// ISIF registers
typedef volatile struct
{
        unsigned long SYNCEN;	// 0h
        unsigned long MODESET;	// 4h
        unsigned long HDW;		// 8h
        unsigned long VDW;		// Ch
        unsigned long PPLN;		// 10h
        unsigned long LPFR;		// 14h
        unsigned long SPH;		// 18h
        unsigned long LNH;		// 1Ch
        unsigned long SLV0;		// 20h
        unsigned long SLV1;		// 24h
        unsigned long LNV;		// 28h
        unsigned long CULH;		// 2Ch
        unsigned long CULV;		// 30h
        unsigned long HSIZE;	// 34h
        unsigned long SDOFST;	// 38h
        unsigned long CADU;		// 3Ch
        unsigned long CADL;		// 40h
        
		unsigned long filler;		// 44h
		unsigned long filler;		// 48h
		unsigned long filler;		// 4Ch
		unsigned long filler;		// 50h
		unsigned long filler;		// 54h
		unsigned long filler;		// 58h
		unsigned long filler;		// 5Ch
		unsigned long filler;		// 60h
		unsigned long filler;		// 64h
		unsigned long filler;		// 68h
		unsigned long filler;		// 6Ch
		unsigned long filler;		// 70h
		unsigned long filler;		// 74h
		unsigned long filler;		// 78h
		unsigned long filler;		// 7Ch
		unsigned long filler;		// 80h

        unsigned long REC656IF;	// 84h
        unsigned long CCDCFG;	// 88h

		unsigned long filler;		// 8Ch
		unsigned long filler;		// 90h
		unsigned long filler;		// 94h
		unsigned long filler;		// 98h
		unsigned long filler;		// 9Ch
		unsigned long filler;		// A0h
		unsigned long filler;		// A4h
		unsigned long filler;		// A8h
		unsigned long filler;		// ACh
		unsigned long filler;		// B0h
		unsigned long filler;		// B4h
		unsigned long filler;		// B8h
		unsigned long filler;		// BCh
		unsigned long filler;		// C0h
		unsigned long filler;		// C4h
		unsigned long filler;		// C8h
		unsigned long filler;		// CCh
		unsigned long filler;		// D0h
		unsigned long filler;		// D4h
		unsigned long filler;		// D8h
		unsigned long filler;		// DCh
		unsigned long filler;		// E0h
		unsigned long filler;		// E4h
		unsigned long filler;		// E8h
		unsigned long filler;		// ECh
		unsigned long filler;		// F0h
		unsigned long filler;		// F4h
		unsigned long filler;		// F8h
		unsigned long filler;		// FCh
		unsigned long filler;		// 100h
		unsigned long filler;		// 104h
		unsigned long filler;		// 108h
		unsigned long filler;		// 10Ch
		unsigned long filler;		// 110h

        unsigned long FMTCFG;   //114h Not in manual??

		unsigned long filler;		// 118h

        unsigned long FMTSPH;	// 11Ch
        unsigned long FMTLNH;	// 120h
        unsigned long FMTSLV;	// 124h
        unsigned long FMTLNV;	// 128h

}     unsigned long ISIF_CONTROLLER;




// Simple single screen configuration
typedef struct
{
/*	unsigned short Width;
	unsigned short Height;
	unsigned short Stride;
	void *Bitmap;*/
} VPFE_SIMPLE_SPEC;

void vpfe_initialize_simple(VPFE_SIMPLE_SPEC *spec);

#endif // DM36X_VPFE_H
