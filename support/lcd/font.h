#ifndef FONT_H
#define FONT_H

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
    short offsx, offy;  // Extra offset (glyphs are compacted in bitmap)
    short advx;         // Offs to next letter?
    char  page;         // Bitmap number
    char  channel;      // to be understood...
} GLYPH;

typedef struct
{
    const GLYPH*   glyphs;
    const KERNING* kernings;
    unsigned short font_size; //
    unsigned short height;    //
    unsigned short base_line; //
    unsigned short pages;    // Number of bitmaps
    unsigned int   unicode; // 0 or 1
    
} FONT;

void font_draw ( const FONT* font, int x, int y);


#endif //FONT_H


