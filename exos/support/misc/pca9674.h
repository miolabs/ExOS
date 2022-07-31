#ifndef PCA9674_H
#define PCA9674_H

typedef union
{
	struct
	{
		unsigned Revision:3;
		unsigned Feature:6;
		unsigned Category:7;
		unsigned Manufacturer:8;
	};
	unsigned char Bytes[3];
} PCA9674_DEVICE_ID;

// prototypes
int pca9674_initialize(int port);
int pca9674_write(int port, unsigned char value);
int pca9674_read(int port, unsigned char *pvalue);

#endif // PCA9674_H

