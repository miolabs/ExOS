#ifndef GFX_CANVAS_H
#define GFX_CANVAS_H

#include <kernel/types.h>

typedef enum
{
	PIX_NONE = 0,
	//
	PIX_1_MONOCHROME = 1,
	PIX_8_GREY = 2,
	PIX_32_ARGB = 3,
} PIXEL_TYPES;

typedef struct
{
	unsigned char* Pixels;
	short Width, Height;
	short StrideBytes;
	short PixelType;		// See enum PIXEL_TYPES
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


#endif // GFX_CANVAS_H


