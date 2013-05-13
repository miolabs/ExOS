#include <kernel/types.h>

#ifndef CANVAS_H
#define CANVAS_H

typedef enum
{
	PIX_1_MONOCHROME = 1,
	PIX_8_GREY = 2,
	PIX_32_ARGB = 3,
} PIXEL_TYPES;

typedef struct
{
	unsigned char* pixels;
	short w, h;
	short stride_bytes;
	int   pix_type;		// See enum PIXEL_TYPES
} CANVAS;

// Small utility to help with canvas

typedef struct { int xi, xe, yi, ye; } BOX;

static inline int _segment_intersection ( int* resi, int* rese, 
											int ai, int ae, int bi, int be)
{
	// Order segments
	if ( bi < ai)
	{
		__SWAP(int,ai,bi);
		__SWAP(int,ae,be);
	}
	if ( ae < bi) 
		return 0;
	*resi = bi;
	*rese = __MIN(ae,be);
	return 1;
}

static inline int _box_intersection ( BOX* res, const BOX* a, const BOX* b)
{
	int did0 = _segment_intersection ( &res->xi, &res->xe, a->xi, a->xe, b->xi, b->xe);
	int did1 = _segment_intersection ( &res->yi, &res->ye, a->yi, a->ye, b->yi, b->ye);
	return did0 && did1;
}


#endif // CANVAS_H


