#ifndef NORDIC_ACI_SUPPORT_H
#define NORDIC_ACI_SUPPORT_H

typedef struct
{
	unsigned char a;
	unsigned char Data[32];
} ACI_SETUP_DATA;


int aci_support_setup();


#endif // NORDIC_ACI_SUPPORT_H



