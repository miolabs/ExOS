#ifndef HAL_SSP_HAL_H
#define HAL_SSP_HAL_H

#ifndef HAL_SSP_MODULE_COUNT
#define HAL_SSP_MODULE_COUNT 2
#endif

typedef enum
{
	HAL_SSP_CLK_IDLE_HIGH = (1<<0),
	HAL_SSP_CLK_PHASE_NEG = (1<<1),	// sample on falling edge of clk
	HAL_SSP_LENGTH_16BIT = (1<<2),
	HAL_SSP_IDLE_HIGH = (1<<3),	// output 0xFF in unidirectional reading
} HAL_SSP_FLAGS;

typedef enum
{
	HAL_SSP_MODE_SPI = 0,
} HAL_SSP_MODE;

// prototypes
void hal_ssp_initialize(int module, int bitrate, HAL_SSP_MODE mode, HAL_SSP_FLAGS flags);
void hal_ssp_transmit(int module, unsigned char *outbuf, unsigned char *inbuf, int length);	// NOTE: set outbuf or inbuf to NULL to make unidirectional
void hal_ssp_sel_control(int module, int asserted);

#endif // HAL_I2C_HAL_H