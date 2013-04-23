#ifndef LCD_MONO_H
#define LCD_MONO_H

typedef struct { short a, b; } MONO_POLY_SEGMENT; 
typedef struct { short x, y; } MONO_POLY_COORDS;

// Synopsis:
//            Draws a solid convex polygon
// Args:
// 
// - coord:   Array of consecutive polygon coordinates
// - ncoords: Number of coordinates; minimum 2, maximum 8
// - pattern: 8 values of 32 bits, containing the pixel pattern to use

typedef struct 
{ 
	MONO_POLY_SEGMENT *segments;
	int w, h, stride;
} MONO_POLY;

void mono_filled_polygon_create ( MONO_POLY* poly, int w, int h);

void mono_filled_polygon_destroy ( MONO_POLY* poly);

void mono_filled_polygon ( const MONO_POLY* poly, unsigned int* bitmap, const unsigned int* pattern,
							MONO_POLY_COORDS* coords, int ncoords);

// Sprites

typedef struct
{
	short          w, h;
	const unsigned int* bitmap;
	const unsigned int* mask;
	short          stride_bitmap, stride_mask;	// Stride in words of uint32
} MONO_SPR;

void mono_draw_sprite ( unsigned int* canvas, int w, int h,
                        const MONO_SPR* spr, int x, int y);

#endif //LCD_MONO_H


