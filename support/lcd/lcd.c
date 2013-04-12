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

void lcd_update(LCD_RECT *region)
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
