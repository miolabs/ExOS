#include "lcd.h"

static LCD_PROPERTIES _properties;

#ifdef LCD_DPI
#define LCD_DPIX LCD_DPI
#define LCD_DPIY LCD_DPI
#endif

//static unsigned char _bitmap[LCD_WIDTH * LCD_HEIGHT >> 3];

void lcd_initialize()
{
	_properties = (LCD_PROPERTIES) {
		.Display = (LCD_DISPLAY_PROPERTIES) {
			.DpiX = LCD_DPIX, .DpiY = LCD_DPIY } };
	lcdcon_initialize(&_properties);	
}

void lcd_lock_bitmap(LCD_RECT *region)
{
	
}

void lcd_clear()
{
}

void lcd_fill()
{
}

void lcd_set_offset(int offset)
{
	//splc501c_set_offset(offset);
}

__attribute__((__weak__))
void lcdcon_gpo_backlight(int enable)
{
	// do nothing
}


// Monochrome bitmap conversion utility
// Converts "vertical byte screens" to classic sequential ordering
//  ||||||| -> -------
//  |||||||    -------

static inline void _bit_exchange ( unsigned * a, unsigned * b, unsigned mask, int shift)
{
    unsigned r;
    r  = (*a & mask) | ((*b & mask) << shift);
    *b = ((*a >> shift) & mask) | (*b & (~mask));
    *a = r;
}

static inline void _tile_rotate ( unsigned char* dst, unsigned int* src, int src_stride)
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

    _bit_exchange ( &d0, &d4, 0x0f0f0f0f, 4);
    _bit_exchange ( &d1, &d5, 0x0f0f0f0f, 4);
    _bit_exchange ( &d2, &d6, 0x0f0f0f0f, 4);
    _bit_exchange ( &d3, &d7, 0x0f0f0f0f, 4);

    _bit_exchange ( &d0, &d2, 0x33333333, 2);
    _bit_exchange ( &d1, &d3, 0x33333333, 2);
    _bit_exchange ( &d4, &d6, 0x33333333, 2);
    _bit_exchange ( &d5, &d7, 0x33333333, 2);

    _bit_exchange ( &d0, &d1, 0x55555555, 1);
    _bit_exchange ( &d2, &d3, 0x55555555, 1);
    _bit_exchange ( &d4, &d5, 0x55555555, 1);
    _bit_exchange ( &d6, &d7, 0x55555555, 1);

    dst [0]  = d7 >> 24;  dst [1] = d6 >> 24;   dst [2] = d5 >> 24;   dst [3] = d4 >> 24;
    dst [4]  = d3 >> 24;  dst [5] = d2 >> 24;   dst [6] = d1 >> 24;   dst [7] = d0 >> 24;
    dst [8]  = d7 >> 16;  dst [9]  = d6 >> 16;  dst [10] = d5 >> 16;  dst [11] = d4 >> 16;
    dst [12] = d3 >> 16;  dst [13] = d2 >> 16;  dst [14] = d1 >> 16;  dst [15] = d0 >> 16;
    dst [16] = d7 >> 8;   dst [17] = d6 >> 8;   dst [18] = d5 >> 8;   dst [19] = d4 >> 8;
    dst [20] = d3 >> 8;   dst [21] = d2 >> 8;   dst [22] = d1 >> 8;   dst [23] = d0 >> 8;
    dst [24] = d7 >> 0;   dst [25] = d6 >> 0;   dst [26] = d5 >> 0;   dst [27] = d4 >> 0;
    dst [28] = d3 >> 0;   dst [29] = d2 >> 0;   dst [30] = d1 >> 0;   dst [31] = d0 >> 0;
}

void lcd_bilevel_linear_2_vertical_bytes ( unsigned char* dst, const unsigned int* src, int w, int h)
{
    int bar, x;
    const int bars = h >> 3;
    for (bar=0; bar<bars; bar++)
    {
        unsigned char* dst_bar_ptr = &dst [ bar * w];
        unsigned int*  src_bar_ptr = &src [ bar * 8 * (w>>5)];
        for (x=0; x<w; x+=32)
            _tile_rotate ( &dst_bar_ptr[x], 
                           &src_bar_ptr[x>>5], w >> 5);
    }
}