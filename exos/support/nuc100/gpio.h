#ifndef NUC100_GPIO_H
#define NUC100_GPIO_H

#define GPIO_PORT_COUNT 5

typedef struct
{
	union
	{
		unsigned long PMD;	// pin mode
		struct
		{
			unsigned PMD0:2;
			unsigned PMD1:2;
			unsigned PMD2:2;
			unsigned PMD3:2;
			unsigned PMD4:2;
			unsigned PMD5:2;
			unsigned PMD6:2;
			unsigned PMD7:2;
			unsigned PMD8:2;
			unsigned PMD9:2;
			unsigned PMD10:2;
			unsigned PMD11:2;
			unsigned PMD12:2;
			unsigned PMD13:2;
			unsigned PMD14:2;
			unsigned PMD15:2;
		};
	};
	struct
	{
		unsigned short Reserved;
		unsigned short OFFD;	// 0 = input enabled; 1 = inpout is disabled (off)
	};
	unsigned long DOUT;
	unsigned long DMASK;	// 0 = write to DOUT is permitted; 1 = write is masked
	unsigned long PIN;
	unsigned long DBEN;		// 0 = disable de-bounce; 1 = enable
	unsigned long IMD;		// 0 = edge trigger; 1 = level trigger
	struct
	{
		unsigned short IF_EN;	// falling edge / high level
		unsigned short IR_EN;	// rising edge / low level
	} IEN;
	unsigned long ISRC;
} GPIO_PORT;

typedef struct
{
	int PIN[16];
} GPIO_PORT_PINS;

#define GPIO_PMD_INPUT 0
#define GPIO_PMD_OUTPUT 1
#define GPIO_PMD_OPEN_DRAIN 2
#define GPIO_PMD_QUADI_BIDIR 3

#endif // NUC100_GPIO_H


