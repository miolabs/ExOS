
#include <support/lcd/lcd.h>
#include <kernel/thread.h>
#include "kernel/timer.h"

extern void lcd_dump_screen ( char* pixels);

static unsigned char screen [(128*64)/8];


static char doh [128*64];



static void bilevel_linear_2_lcd ( unsigned char* dst, unsigned char* src, int w, int h);

static unsigned char linear [(128*64)/8];

static void gray8_2_lcdbits ( unsigned char* dst, unsigned char* src, int w, int h, int frame);

void main()
{
    int i;

	lcd_initialize();
	lcdcon_gpo_backlight(1);

    for (i=0; i<(128*64/8); i++)
        screen [i] = 0;

    int x = 0, y = 0;
    for (y=0; y<64; y++)
        for ( x=0; x<128; x++)
        {
            int xx = x - 64; int yy = y - 32;
            int r = xx*xx + yy*yy;
            r >>= 9; if ( r > 7) r = 7;
            doh [(y<<7)+x] = 7 - r;
        }

static unsigned char letter  [] = { 0x00,0x00,0x04,0x07,0x07,0x04,0x00,0x00,
                                 0x00,0x00,0x07,0x00,0x00,0x07,0x00,0x00,
                                 0x00,0x00,0x07,0x00,0x00,0x07,0x00,0x00,
                                 0x00,0x00,0x04,0x07,0x07,0x04,0x00,0x00,
                                 0x00,0x00,0x07,0x00,0x00,0x07,0x00,0x00,
                                 0x00,0x00,0x07,0x00,0x00,0x07,0x00,0x00,
                                 0x00,0x00,0x07,0x00,0x00,0x07,0x00,0x00,
                                 0x00,0x00,0x04,0x07,0x07,0x04,0x00,0x00};

static unsigned char smooth [] =
   "\13\13\13\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\13\13\13"
  "\13\13\13\13\13\13\13\13\13\13\13\13\13\13\13\13\13\13\13\13\13\13\13\13"
  "\13\13\13\13\13\13\0\0\0\0\0\0\0\0\0\377\377\377\377\377\377\377\377\377"
  "\0\0\0\0\0\0\0\0\0\13\13\13\13\13\13\13\13\13\13\13\13lll\177\177\177ggg"
  "\13\13\13\13\13\13\13\13\13\13\13\13\0\0\0\0\0\0\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\0\0\0\0\0\0\13\13\13\13\13\13\36\36"
  "\36\324\324\324\364\364\364\364\364\364\364\364\364\317\317\317!!!\13\13"
  "\13\13\13\13\0\0\0\377\377\377\377\377\377\377\377\377\35\35\35\377\377\377"
  "\377\377\377\377\377\377\0\0\0\13\13\13\13\13\13\261\260\261\364\364\364"
  "\263\263\263^^^\331\331\331\364\364\364\203\203\203\13\13\13\13\13\13\0\0"
  "\0\377\377\377\377\377\377\0\0\0\0\0\0\377\377\377\377\377\377\377\377\377"
  "\0\0\0\13\13\13\13\13\13\261\260\261\362\362\362111\13\13\13\227\227\227"
  "\364\364\364\240\240\240\13\13\13\13\13\13\0\0\0\377\377\377\377\377\377"
  "\377\377\377\0\0\0\377\377\377\377\377\377\377\377\377\0\0\0\13\13\13\13"
  "\13\13\273\273\273\364\364\364\250\250\250\13\13\13\305\306\305\363\363\363"
  "iji\13\13\13\13\13\13\0\0\0\15\15\15\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\0\0\0\0\0\0\13\13\13\13\13\13PPP\354\354\354\364"
  "\364\364\351\351\351\364\364\364\270\270\270\13\13\13\13\13\13\13\13\13\0"
  "\0\0\0\0\0\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\0"
  "\0\0\0\0\0\13\13\13\13\13\13\13\13\13\312\312\312\364\364\364\364\364\364"
  "\363\363\363bbb\13\13\13\13\13\13\13\13\13\0\0\0\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\0\0\0\13\13"
  "\13\13\13\13\271\271\271\364\364\364\323\323\323\267\267\267\364\364\364"
  "\362\362\362kkk\13\13\13\13\13\13\5\5\5\377\377\377\377\377\377\0\0\0\0\0"
  "\0\377\377\377\377\377\377\377\377\377\0\0\0\13\13\13HHH\361\361\361\344"
  "\344\344'''\13\13\13\242\242\242\364\364\364\317\320\317\13\13\13\13\13\13"
  "\377\377\377\377\377\377\377\377\377\0\0\0\0\0\0\0\0\0\377\377\377\377\377"
  "\377\0\0\0\13\13\13xxx\364\364\364\306\307\306\13\13\13\13\13\13'('\361\361"
  "\361\334\334\334!!!\13\13\13\0\0\0\377\377\377\377\377\377\377\377\377\35"
  "\35\35\377\377\377\377\377\377\377\377\377\0\0\0\13\13\13;;;\361\361\361"
  "\360\360\360\222\222\222^^^\276\276\276\364\364\364\276\276\276\13\13\13"
  "\13\13\13\0\0\0\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\0\0\0\0\0\0\13\13\13\13\13\13\231\231\231\363\363\363\364\364"
  "\364\364\364\364\364\364\364\334\334\334111\13\13\13\13\13\13\0\0\0\0\0\0"
  "\0\0\0\377\377\377\377\377\377\377\377\377\0\0\0\0\0\0\0\0\0\13\13\13\13"
  "\13\13\13\13\13""222\203\203\203\213\213\213qqq\13\13\13\13\13\13\13\13\13"
  "\13\13\13\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\13\13\13"
  "\13\13\13\13\13\13\13\13\13\13\13\13\13\13\13\13\13\13\13\13\13\13\13\13"
  "\13\13\13";

    for (y=0; y<15; y++)
        for (x=0; x<20; x++)
            doh [ y*128+x] = smooth [y*20*3+x*3] >> 5;

/*
    for (i=0; i<(128*64/8); i++)
        linear [i] = 0xff; 

    int x = 0, y = 0, lx=0;
    for (y=0; y<64; y++)
    { 
        for ( lx=0; lx<128; lx+=8)
        {
            int bits = 0;
            for ( x=lx; x<lx+8; x++)
            {
                bits <<= 1;
                int xx = x - 64; int yy = y - 32;
                int r = xx*xx + yy*yy;
                r >>= 9; if ( r > 7) r = 7;
                bits |= r & 1;
            }
            linear [(y<<4) + (lx>>3)] = bits;
        }
   }

   linear [16*0]   = 0x00;
   linear [16*1]   = 0x7e;
   linear [16*2]   = 0x81;
   linear [16*3]   = 0x81;
   linear [16*4]   = 0xfe;
   linear [16*5]   = 0x80;
   linear [16*6]   = 0x80;
   linear [16*7]   = 0x00;
*/

    int frame = 0;
    unsigned long time = exos_timer_time();
    while (1)
    {
        volatile unsigned long time_c, time_b, time_a = exos_timer_time();

        gray8_2_lcdbits ( screen, doh, 128, 64, frame);
        //bilevel_linear_2_lcd ( screen, linear, 128, 64);

        time_b = exos_timer_time();

        lcd_dump_screen ( screen);

        time_c = exos_timer_time();

        frame++;

        int elap = exos_timer_elapsed( time);
        time = exos_timer_time();
        int rem = 10 - elap;
        if ( rem > 0)
            exos_thread_sleep( rem);
    }
}

static inline void bit_exchange ( unsigned * a, unsigned * b, unsigned mask, int shift)
{
    unsigned r;
    r  = (*a & mask) | ((*b & mask) << shift);
    *b = ((*a >> shift) & mask) | (*b & (~mask));
    *a = r;
}

static inline void tile_rotate ( unsigned char* dst, unsigned long* src, int src_stride)
{
    unsigned d0, d1, d2, d3, d4, d5, d6, d7;
    d0 = *src; src += src_stride;
    d1 = *src; src += src_stride;
    d2 = *src; src += src_stride;
    d3 = *src; src += src_stride;
    d4 = *src; src += src_stride;
    d5 = *src; src += src_stride;
    d6 = *src; src += src_stride;
    d7 = *src; src += src_stride;

    bit_exchange ( &d0, &d4, 0x0f0f0f0f, 4);
    bit_exchange ( &d1, &d5, 0x0f0f0f0f, 4);
    bit_exchange ( &d2, &d6, 0x0f0f0f0f, 4);
    bit_exchange ( &d3, &d7, 0x0f0f0f0f, 4);

    bit_exchange ( &d0, &d2, 0x33333333, 2);
    bit_exchange ( &d1, &d3, 0x33333333, 2);
    bit_exchange ( &d4, &d6, 0x33333333, 2);
    bit_exchange ( &d5, &d7, 0x33333333, 2);

    bit_exchange ( &d0, &d1, 0x55555555, 1);
    bit_exchange ( &d2, &d3, 0x55555555, 1);
    bit_exchange ( &d4, &d5, 0x55555555, 1);
    bit_exchange ( &d6, &d7, 0x55555555, 1);

    dst [0]  = d7;       dst [1] = d6;        dst [2] = d5;        dst [3] = d4;
    dst [4]  = d3;       dst [5] = d2;        dst [6] = d1;        dst [7] = d0;
    dst [8]  = d7 >> 8;  dst [9]  = d6 >> 8;  dst [10] = d5 >> 8;  dst [11] = d4 >> 8;
    dst [12] = d3 >> 8;  dst [13] = d2 >> 8;  dst [14] = d1 >> 8;  dst [15] = d0 >> 8;
    dst [16] = d7 >> 16; dst [17] = d6 >> 16; dst [18] = d5 >> 16; dst [19] = d4 >> 16;
    dst [20] = d3 >> 16; dst [21] = d2 >> 16; dst [22] = d1 >> 16; dst [23] = d0 >> 16;
    dst [24] = d7 >> 24; dst [25] = d6 >> 24; dst [26] = d5 >> 24; dst [27] = d4 >> 24;
    dst [28] = d3 >> 24; dst [29] = d2 >> 24; dst [30] = d1 >> 24; dst [31] = d0 >> 24;
}

static void bilevel_linear_2_lcd ( unsigned char* dst, unsigned char* src, int w, int h)
{
    int bar, x;
    const int bars = h >> 3;
    for (bar=0; bar<bars; bar++)
    {
        unsigned char* dst_bar_ptr = &dst [ bar * w];
        unsigned char* src_bar_ptr = &src [ bar * 8 * (w>>3)];
        for (x=0; x<w; x+=32)
            tile_rotate ( &dst_bar_ptr[x], 
                          (unsigned long*) &src_bar_ptr[ x >> 3], w >> 5);
    }
}


static void gray8_2_lcdbits ( unsigned char* dst, unsigned char* src, int w, int h, int frame)
{
    int bar, x;
    int stride = w;
    const static unsigned char dither_tab [ 8 * 8] = 
    {
//Color 0    1    2    3    4    5    6    7
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01, // Frame 0
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01, // Frame 1
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01, // Frame 2
        0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x01, // Frame 3
        0x00,0x00,0x00,0x00,0x01,0x01,0x01,0x01, // Frame 4
        0x00,0x00,0x00,0x01,0x01,0x01,0x01,0x01, // Frame 5
        0x00,0x00,0x01,0x01,0x01,0x01,0x01,0x01, // Frame 6
        0x00,0x00,0x01,0x01,0x01,0x01,0x01,0x01  // frame 7 
    };

    const unsigned char* dither = &dither_tab [ (frame & 0x7) << 3];
    for (bar=0; bar<(h>>3); bar++)
    {
        unsigned char* dst_bar_ptr = &dst [ bar * w];
        unsigned char* src_bar_ptr = &src [ bar * 8 * w];
        for (x=0; x<w; x++)
        {
            unsigned char* drill = &src_bar_ptr[x];
            int c, bitpack = 0;
            c = *drill; drill += stride;
            bitpack |= dither [ c];
            c = *drill; drill += stride;
            bitpack |= dither [ c] << 1;
            c = *drill; drill += stride;
            bitpack |= dither [ c] << 2;
            c = *drill; drill += stride;
            bitpack |= dither [ c] << 3;
            c = *drill; drill += stride;
            bitpack |= dither [ c] << 4;
            c = *drill; drill += stride;
            bitpack |= dither [ c] << 5;
            c = *drill; drill += stride;
            bitpack |= dither [ c] << 6;
            c = *drill;
            bitpack |= dither [ c] << 7;
            dst_bar_ptr [ x] = bitpack;
        }
    }
}



