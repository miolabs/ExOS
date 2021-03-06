#ifndef GFX_FONT_H
#define GFX_FONT_H

#include <modules/gfx/canvas.h>
#include <modules/gfx/mono.h>

typedef struct
{
    unsigned int g1st, g2nd;
    unsigned int amount;
} KERNING;

typedef struct
{
    unsigned int id;        // Char code
    unsigned short x, y;    // Pos in bitmap page
    unsigned short w, h;    // Width, height
    short offsx, offsy;		// Extra offset (glyphs are compacted in bitmap)
    short advx;				// Offs to next letter?
    char  page;				// Bitmap number
    char  channel;			// to be understood...
} GLYPH;

typedef struct
{
	const GLYPH*   glyphs;		// Glyph table; NOT in ASCII order
	const KERNING* kernings;	// Recommended kernings for this font
	// If the font is ASCII or similar (8 bits max. codes), fast acces tables
	// wil be available
	const short*   fast_glyphs;
	const short*   fast_kernings;

	unsigned short font_size;
	unsigned short height;
	unsigned short base_line;
	unsigned short pages; // Number of bitmaps
	unsigned short unicode; // 0 or 1
} FONT_MAP;

typedef struct
{
	const FONT_MAP* map;
	const SPRITE*   bitmaps[1];	// 1 supported, can be several for Chinese, etc...
} FONT;
    
typedef enum
{
	FONT_MONOSPACE = 0,
	FONT_PROPORTIONAL = 1,
	FONT_KERNING = 2,
} FONT_FLAGS;

// Y coordinate refers to font baseline
void font_draw(const CANVAS* canvas, const char* text, const FONT* font, 
	FONT_FLAGS print_flags, int x, int y);

// Calculate pixel length of a given text 
int font_calc_len(const FONT* font, const char* text, FONT_FLAGS print_flags);

#endif // GFX_FONT_H


