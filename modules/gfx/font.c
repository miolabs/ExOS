
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "font.h"



static inline int kerning_width ( const KERNING* kernings, int nextchar)
{
	int ref = kernings->g1st;
	while ( kernings->g1st == ref)
	{
		if ( kernings->g2nd == nextchar)
		 return kernings->amount;
		kernings++;
	}
	return 0;
}

int  font_calc_len ( const FONT* font, const char* text, int print_flags)
{
	const FONT_MAP* map = font->map;
	int w = 0;
	if (( print_flags & FONT_PROPORTIONAL) == 0)
		return strlen(text) * map->font_size;

	int do_kerning = map->fast_kernings && ( print_flags & FONT_KERNING);
	assert( map->fast_glyphs);	// Just ASCII for now
	int i = 0;
	while( text[i])
	{
		int code  = text[i];
		int iglyph = map->fast_glyphs [ code];
		if ( iglyph != -1)	// If there is a glyph for this code
		{
			const GLYPH* glyph = &map->glyphs [ iglyph];
			w += glyph->advx;
			if ( do_kerning)
			{
				assert( map->fast_kernings);
				int kern_idx = map->fast_kernings[code];
				if ( kern_idx != -1)				
					w += kerning_width ( &map->kernings [ kern_idx], text[i+1]);
			}
		}
		i++;
	}
	return w;
}

void font_draw ( CANVAS* canvas, const char* text, const FONT* font, 
				 int print_flags, int x, int y)
{
	const FONT_MAP* map = font->map;

	assert( canvas->pix_type == PIX_1_MONOCHROME);
	assert( map->fast_glyphs);	// Just ASCII for now
	assert( map->fast_kernings);

	const SPRITE*   spr = font->bitmaps[0];
	int do_kerning = map->fast_kernings && ( print_flags & FONT_KERNING);

	y -= font->map->base_line;
	int i = 0;
	while( text[i])
	{
		int code  = text[i];
		int iglyph = map->fast_glyphs [ code];
		if ( iglyph != -1)	// If there is a glyph for this code
		{
			const GLYPH* glyph = &map->glyphs [ iglyph];

			BOX box = { glyph->x,  glyph->x + glyph->w - 1,
						glyph->y,  glyph->y + glyph->h - 1};
			mono_draw_sprite_part ( canvas, font->bitmaps[0], 
									x+glyph->offsx, y+glyph->offsy, &box);	

			int adv = map->font_size;
			if ( print_flags & FONT_PROPORTIONAL)
			{
				adv = glyph->advx;
				if ( do_kerning)
				{
					int kern_idx = map->fast_kernings[code];
					if ( kern_idx != -1)				
						adv += kerning_width ( &map->kernings [ kern_idx], text[i+1]);
				}
			}
			x += adv;
		}
		i++;
	}
}

