#ifndef LCD_LCD_H
#define LCD_LCD_H

typedef struct
{
	unsigned short Left, Top, Right, Bottom;
} LCD_RECT;

typedef enum
{
	LCD_BUFFER_RASTER = 0,
	LCD_BUFFER_STRIPE,
	LCD_BUFFER_PLANAR,
} LCD_BUFFER_TYPE;

typedef enum
{
	LCD_BUFFER_LUT = 1<<0,
} LCD_BUFFER_FLAGS;

typedef struct
{
	unsigned char Depth;
	unsigned char BufferType;
	unsigned char Flags;
	unsigned char Reserved;
} LCD_BUFFER_PROPERTIES;

typedef enum
{
	LCD_DISP_MONOCHROME = 0,
	LCD_DISP_COLOR = 1<<0,
	LCD_DISP_ACTIVE_MATRIX = 1<<1,
} LCD_DISPLAY_TYPE;

typedef struct
{
	unsigned short DpiX;
	unsigned short DpiY;
	LCD_DISPLAY_TYPE DisplayType;
} LCD_DISPLAY_PROPERTIES;

typedef struct
{
	LCD_DISPLAY_PROPERTIES Display;
	LCD_BUFFER_PROPERTIES Buffer;
	unsigned short Width;
	unsigned short Height;
} LCD_PROPERTIES;

// prototypes
void lcd_initialize();
void lcd_update(LCD_RECT *region);
void lcd_clear();

typedef enum
{
	LCDCON_GPO_IDLE = 0,
	LCDCON_GPO_RESET = (1<<0),
	LCDCON_GPO_CS = (1<<1),
	LCDCON_GPO_A0 = (1<<2),
} LCDCON_GPO;

// controller driver prototypes
void lcdcon_initialize(LCD_PROPERTIES *lcd);
void lcdcon_gpo_initialize();
void lcdcon_gpo(LCDCON_GPO gpo);
void lcdcon_gpo_backlight(int enable);

#endif // LCD_LCD_H
