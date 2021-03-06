
#include <assert.h>
#include <kernel/memory.h>

#include "mono.h"
#include "canvas.h"


void mono_filled_polygon_create ( MONO_POLY* poly, int w, int h)
{
	poly->segments = (MONO_POLY_SEGMENT*) exos_mem_alloc( sizeof(MONO_POLY_SEGMENT) * h, 
														  EXOS_MEMF_CLEAR);
	poly->w = w;
	poly->h = h;
	poly->stride = w >> 5;	// Stride in uint32
}

void mono_filled_polygon_destroy ( MONO_POLY* poly)
{
	exos_mem_free( poly->segments);
	poly->segments = 0;
}

static inline int _next_vtx ( int v, int n) { return (( v+1) == n)  ? 0   : v+1; }
static inline int _prev_vtx ( int v, int n) { return (( v-1) == -1) ? n-1 : v-1; }
static inline int _climit( int v, int min, int max)  { v=(v<min)?min:v; return (v>max)?max:v; }

#define DUMB_CLIP	// Define for simple clipping

#ifdef DUMB_CLIP
#define CCONTROL(EXP)  if((y>=0) && (y<poly->h)) { EXP; }
#else
#define CCONTROL(EXP) EXP
#endif 

#define FX_EDGES (18)

void mono_filled_polygon(const MONO_POLY* poly, unsigned int* bitmap, 
                         const unsigned int* pattern, MONO_POLY_COORDS* coords, int ncoords)
{
	int i, y;
	assert(( ncoords>2) && ( ncoords<=8));
    MONO_POLY_SEGMENT* segments = poly->segments;

	// Look for the top & bottom points
	int top_y=coords[0].y, top_v=0;
	int bot_y=coords[0].y, bot_v=0;
	for ( i=1; i<ncoords; i++)
	{
		if ( coords[i].y<top_y)
			top_y=coords[i].y, top_v=i;
		if ( coords[i].y>bot_y)
			bot_y=coords[i].y, bot_v=i;
	}
	// All points in the same line
	if ( top_v == bot_v) 
		bot_v = _next_vtx( bot_v, ncoords);
	// No lines on screen
	if (( bot_y < 0) || ( top_y > (poly->h - 1)))
		return;

	// Interpolate clockwise side
	int vtx = top_v;
	while ( vtx != bot_v)
	{
		int nxt=_next_vtx ( vtx, ncoords);

		int fx_x   = coords[vtx].x << FX_EDGES;
		int fx_dx  = coords[nxt].x - coords[vtx].x;
		int edge_h = coords[nxt].y - coords[vtx].y;
		fx_dx = ( edge_h != 0) ? (fx_dx << FX_EDGES) / edge_h: 0;
		for ( y=coords[vtx].y; y<=coords[nxt].y; y++)
		{
			CCONTROL( segments[y].b = fx_x >> FX_EDGES)
			fx_x += fx_dx;
		}
		vtx = _next_vtx ( vtx, ncoords);
	}

	// Interpolate counter-clockwise side
	vtx = top_v;
	while ( vtx != bot_v)
	{
		int nxt=_prev_vtx ( vtx, ncoords);

		int fx_x   = coords[vtx].x << FX_EDGES;
		int fx_dx  = coords[nxt].x - coords[vtx].x;
		int edge_h = coords[nxt].y - coords[vtx].y;
		fx_dx = ( edge_h != 0) ? (fx_dx << FX_EDGES) / edge_h: 0;
		for ( y=coords[vtx].y; y<=coords[nxt].y; y++)
		{
			CCONTROL( segments[y].a = fx_x >> FX_EDGES)
			fx_x += fx_dx;
		}
		vtx = _prev_vtx ( vtx, ncoords);
	}

	// Fill all the lines
	#ifdef DUMB_CLIP
	top_y=_climit( top_y, 0, poly->h-1);
	bot_y=_climit( bot_y, 0, poly->h-1);
	#endif
	unsigned int* line = bitmap;
	line += poly->stride * top_y;
	for ( y=top_y; y<=bot_y; y++)
	{
		const unsigned int allones = 0xffffffff;
		unsigned int ink = pattern[ y & 0x7];
		int xi = segments[y].a;
		int xe = segments[y].b;
		if ( xi > xe)
			xi = segments[y].b,
			xe = segments[y].a;
		#ifdef DUMB_CLIP
		xi=_climit( xi, 0, poly->w-1);
		xe=_climit( xe, 0, poly->w-1);
		#endif

		unsigned int mask_i =   allones >> (xi & 0x1f);
		unsigned int mask_e = ~(allones >> (xe & 0x1f));
		xi >>= 5, xe >>= 5;
		if ( xi == xe)
		{
			unsigned int mask = mask_i & mask_e;
			line [ xi] = (line[xi] & (~mask)) | (ink & mask);
		}
		else
		{
			line [ xi] = (line[xi] & (~mask_i)) | (ink & mask_i); 
			xi++;
			while ( xi != xe)
				line [ xi] = ink, xi++;
			line [ xe] = (line[xe] & (~mask_e)) | (ink & mask_e);
		}
		line += poly->stride;
	}
}

// ---------------------------------------------------------------------------

#define SPAN_HEAD    (1<<1)
#define SPAN_TAIL    (1<<2)
#define SPAN_HEAD_TAIL (SPAN_HEAD | SPAN_TAIL)

#define WORD_ENDED(V)  (((V>>5)<<5)+31)

// Mask skipper 
// In order to avoid code specialization (masked, not masked) we use this macro
// which allows to stop the pointer in runtime
#define MSK(OFFS)  ((tsprx+(OFFS)) & mask_skipper)

static inline void _mono_draw_sprite_box ( const CANVAS* canvas,
                            const SPRITE* spr, int x, int y, BOX* sprbox)
{
	int spr_1st_line = sprbox->yi;
	int sprspan_x    = sprbox->xi >> 5;
	int shift        = x & 0x1f;
	// On screen result
	sprbox->xi += x;
	sprbox->xe += x;
	sprbox->yi += y;
	sprbox->ye += y;

	const unsigned int ones = 0xffffffff;
	int scrspan_i  = sprbox->xi >> 5;
	int scrspan_e  = sprbox->xe >> 5;
	int mod_i      = sprbox->xi & 0x1f;
	int mod_e      = sprbox->xe & 0x1f;

	int span_flags = 0;
	if ( mod_i != 0)
		span_flags |= SPAN_HEAD;
	if ( mod_e != 0x1f)
		span_flags |= SPAN_TAIL;
	int central_e = ( span_flags & SPAN_TAIL) ? scrspan_e : scrspan_e+1;

	unsigned int* scrline  = (unsigned int* )(canvas->Pixels + (canvas->StrideBytes * sprbox->yi));
	const unsigned int* sprline  = spr->Bitmap + spr->BitmapStride * spr_1st_line;
	const unsigned int* maskline = spr->Mask + spr->MaskStride * spr_1st_line;
	unsigned int mask_i = ones >> mod_i;
	unsigned int mask_e = ones << (31 - mod_e);
	int stride_mask = spr->MaskStride;
	// Unmasked case
	static const unsigned int default_mask = 0xffffffff;
	int mask_skipper = -1;
	if (spr->Mask == 0)
		maskline = &default_mask, mask_skipper = 0, stride_mask = 0;

	//if ((( span_flags & SPAN_HEAD_TAIL) == SPAN_HEAD_TAIL)  &&  ( scrspan_i == scrspan_e))
	if ( scrspan_i == scrspan_e)
	{
		// Narrow sprite special case    
		if ( shift <= mod_i)
			for ( y=sprbox->yi;y<=sprbox->ye;  y++)
			{
				int tsprx = sprspan_x;
				unsigned int sprmask = maskline [MSK(0)] >> shift;
				unsigned int sprpix  = sprline  [ tsprx] >> shift;
				unsigned int mask    = mask_i & mask_e & sprmask;
				scrline[scrspan_i] =  ( sprpix & mask) | ( scrline[scrspan_i] & (~mask)); 
				scrline  += canvas->StrideBytes >> 2;
				sprline  += spr->BitmapStride;
				maskline += stride_mask;
			}
		else
			for ( y=sprbox->yi;y<=sprbox->ye;  y++)
			{
				int tsprx = sprspan_x;
				unsigned int sprpix, sprmask;
				if ( shift != 0)
					sprpix  = (sprline  [tsprx+1] >> shift) | (sprline  [ tsprx] << (32-shift)),
					sprmask = (maskline [MSK( 1)] >> shift) | (maskline [MSK(0)] << (32-shift));
				else
					sprpix  = sprline  [tsprx],
					sprmask = maskline [MSK(0)];
				unsigned int mask    = mask_i & mask_e & sprmask;
				scrline[scrspan_i] =  ( sprpix & mask) | ( scrline[scrspan_i] & (~mask)); 
				scrline  += canvas->StrideBytes >> 2;
				sprline  += spr->BitmapStride;
				maskline += stride_mask;
			}
	}
	else	// Wide sprite
	{
		// General blit case
		for ( y=sprbox->yi; y<=sprbox->ye; y++)
		{
			int tscrx = scrspan_i;
			int tsprx = sprspan_x;
			unsigned int sprpix, sprmask, mask;
			if (( span_flags & SPAN_HEAD) != 0)
			{
				if ( shift <= mod_i)
					sprpix  = sprline  [ tsprx] >> shift,
					sprmask = maskline [MSK(0)] >> shift;
				else
				{
					if ( shift != 0)
						sprpix  = (sprline  [tsprx+1] >> shift) | (sprline  [ tsprx] << (32-shift)),
						sprmask = (maskline [MSK( 1)] >> shift) | (maskline [MSK(0)] << (32-shift));
					else
						sprpix  = sprline  [tsprx+0],
						sprmask = maskline [MSK(0)];
				}
				mask = mask_i & sprmask;
				scrline[ tscrx ] = ( sprpix & mask) | ( scrline[tscrx] & (~mask));
                tscrx++;
				if (( shift > mod_i) || (shift == 0))
					tsprx++;
			}
			
			// Central spans
			if ( shift != 0)
			{
				while ( tscrx < central_e)
				{ 
					sprpix  = (sprline  [tsprx+1] >> shift) | (sprline  [ tsprx] << (32-shift));
					sprmask = (maskline [MSK( 1)] >> shift) | (maskline [MSK(0)] << (32-shift));
					scrline[tscrx] = ( sprpix & sprmask) | ( scrline[tscrx] & (~sprmask));
					tscrx++, tsprx++;
				}
			}
			else
			{
				while ( tscrx < central_e)
				{
					sprpix = sprline [tsprx], sprmask = maskline [MSK(0)];
					scrline[tscrx] = ( sprpix & sprmask) | ( scrline[tscrx] & (~sprmask));
					tscrx++, tsprx++;
				}
			}

			if (( span_flags & SPAN_TAIL) != 0)
			{
				if ( shift < mod_e)
				{
					if ( shift != 0)
						sprpix  = (sprline  [tsprx+1] >> shift) | (sprline  [ tsprx] << (32-shift)),
						sprmask = (maskline [MSK( 1)] >> shift) | (maskline [MSK(0)] << (32-shift));
					else
						sprpix  = sprline  [tsprx+0],
						sprmask = maskline [MSK(0)];
				}
				else
				{
					sprpix  = sprline  [tsprx];
					sprmask = maskline [MSK(0)];
					if ( shift) 
						sprpix <<= (32-shift), sprmask <<= (32-shift);
				}
				mask =  mask_e & sprmask;
				scrline[tscrx] = ( sprpix & mask) | ( scrline[tscrx] & (~mask));
			}

			// Next line
            scrline  += canvas->StrideBytes >> 2;
			sprline  += spr->BitmapStride;
			maskline += stride_mask;
		}	// for ( y...)
	}
}

void mono_draw_sprite(const CANVAS* canvas, const SPRITE* spr, int x, int y)
{
	assert((spr->PixelType == PIX_1_MONOCHROME) && (canvas->PixelType == PIX_1_MONOCHROME));
	BOX scr = { -x, -x+canvas->Width-1,  -y, -y+canvas->Height-1};
	BOX sub = { 0,  spr->Width-1, 0, spr->Height-1};
	BOX res;
	if (!_box_intersection(&res, &scr, &sub))
		return;

	_mono_draw_sprite_box(canvas, spr,  x,  y, &res);
}


void mono_draw_sprite_part(const CANVAS* canvas, const SPRITE* spr,
                             int x, int y, const BOX* part)
{
	assert((spr->PixelType == PIX_1_MONOCHROME) && (canvas->PixelType == PIX_1_MONOCHROME));

	// Check "part" is inside the bitmap
	BOX bitm = { 0,  spr->Width-1, 0, spr->Height-1};
	BOX bpart;
	if ( !_box_intersection ( &bpart, part, &bitm))
		return;

	// 	Set the sprite square on screen
	BOX scr = { -x, -x+canvas->Width-1,  -y, -y+canvas->Height-1};
	BOX sub = { 0, bpart.xe - bpart.xi, 
	            0, bpart.ye - bpart.yi };
	BOX res;
	if ( !_box_intersection ( &res, &scr, &sub))
		return;

	res.xi += bpart.xi;
	res.xe += bpart.xi;
	res.yi += bpart.yi;
	res.ye += bpart.yi;
	_mono_draw_sprite_box (canvas, spr, x - bpart.xi,  y - bpart.yi, &res);
}
