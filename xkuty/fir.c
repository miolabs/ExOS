
#include <assert.h>
#include "fir.h"

static inline int fir_out_of_range ( unsigned int old_avg, unsigned int newval, int treshold)
{
	int dif = (int)old_avg;
	dif -= (int)newval;
	if ( dif < 0) dif = -dif;
	return dif > treshold;
}

unsigned int fir_filter ( FIR* fir, unsigned int newval)
{
	unsigned int res = 0;
	assert(fir->num_regs <= FIR_LEN);
	// Get average
	if ( fir->stored > 0)
	{
		unsigned int old_avg =  fir->total / fir->stored;
		if ( fir->discard && (fir->chained_discards < fir->max_chained_discards))
		{
			if ( fir_out_of_range ( old_avg, newval, fir->treshold))
			{	
				fir->chained_discards++;
				return old_avg;
			}
		}

		if( fir->chained_discards > 0) fir->chained_discards--;
		
		if ( fir->stored == fir->num_regs)
			fir->total -= fir->smp[ fir->pos];
		fir->smp[fir->pos] = newval;
		fir->pos++;    if( fir->pos == fir->num_regs) fir->pos = 0;
		fir->stored++; if( fir->stored > fir->num_regs) fir->stored = fir->num_regs;
		fir->total += newval;

		res =  fir->total / fir->stored;
	}
	else
		fir->stored=1, fir->pos=1, fir->smp[0]=newval,fir->total=newval, res=newval;

	return res;
}



