#ifndef FIR_H
#define FIR_H


#define FIR_LEN  10

typedef struct
{
	char  num_regs, stored, pos;
	char  discard, chained_discards, max_chained_discards;
    unsigned short treshold;
	unsigned int   total;
	unsigned short smp[FIR_LEN];
} FIR;

static inline void fir_init( FIR* fir, int num_regs, 
							int discard, unsigned short discard_treshold, int max_chained_discards) 
{  
	fir->num_regs             = num_regs;
	fir->stored               = 0;
	fir->discard              = discard;
	fir->chained_discards     = 0; 
	fir->treshold             = discard_treshold;
	fir->max_chained_discards = max_chained_discards;
}

unsigned int fir_filter(FIR* fir, unsigned int newval);

#endif // FIR_H


