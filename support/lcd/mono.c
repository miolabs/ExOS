
#include "mono.h"
#include <assert.h>
#include <kernel/memory.h>


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

void mono_filled_polygon ( const MONO_POLY* poly, unsigned int* bitmap, 
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

typedef struct { int xi, xe, yi, ye; } MONO_BOX;

static inline void _swapi ( int* a, int* b)  { int aux=*b; *b=*a; *a=aux; }
static inline int  _min   ( int a, int b)    { return (a<b)?a:b; }

static inline int _segment_intersection ( int* resi, int* rese, 
											int ai, int ae, int bi, int be)
{
	// Order segments
	if ( bi < ai)
	{
		_swapi(&ai,&bi);
		_swapi(&ae,&be);
	}
	if ( ae < bi) 
		return 0;
	*resi = bi;
	*rese = _min(ae,be);
	return 1;
}

static inline int _box_intersection ( MONO_BOX* res, MONO_BOX* a, MONO_BOX* b)
{
	int did0 = _segment_intersection ( &res->xi, &res->xe, a->xi, a->xe, b->xi, b->xe);
	int did1 = _segment_intersection ( &res->yi, &res->ye, a->yi, a->ye, b->yi, b->ye);
	return did0 && did1;
}

void mono_draw_sprite ( unsigned int* canvas, int w, int h,
                        const MONO_SPR* spr, int x, int y)
{
	MONO_BOX scr = { -x,-y,-x+w-1,-y+h-1};
	MONO_BOX sub = { 0,0,spr->w-1,spr->h-1};
	MONO_BOX res;

	int did = _box_intersection ( &res, &scr, &sub);
/*
	if ( sprspan_i == sprspan_e)
	{
	}
	else
	{

	}
*/
}



