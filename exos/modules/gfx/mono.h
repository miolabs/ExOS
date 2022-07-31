#ifndef GFX_MONO_H
#define GFX_MONO_H

#include <modules/gfx/canvas.h>

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
	short Width, Height;
	const unsigned int* Bitmap;
	const unsigned int* Mask;
	short BitmapStride, MaskStride;	// Stride in words of uint32
	short PixelType, AlphaPixelType;  // enum PIXEL_TYPES
} SPRITE;

void mono_draw_sprite(const CANVAS *canvas,
	const SPRITE *spr, int x, int y);

void mono_draw_sprite_part(const CANVAS *canvas, const SPRITE *spr,
	int x, int y, const BOX *part);

#endif // LCD_MONO_H


