#ifndef LPC15_QEI_H
#define LPC15_QEI_H

#include "cpu.h"

#define QEI_CON_RESP	(1<<0)
#define QEI_CON_RESPI	(1<<1)
#define QEI_CON_RESV	(1<<2)
#define QEI_CON_RESI	(1<<3)

#define QEI_CONF_DIRINV		(1<<0)
#define QEI_CONF_SIGMODE	(1<<1)
#define QEI_CONF_CAPMODE	(1<<2)
#define QEI_CONF_INVINX		(1<<3)
#define QEI_CONF_CRESPI		(1<<4)
#define QEI_CONF_INXGATE_BIT	16

#define QEI_STAT_DIR	(1<<0)

#define QEI_INTF_INX	(1<<0)
#define	QEI_INTF_TIM	(1<<1)
#define QEI_INTF_VELC	(1<<2)
#define QEI_INTF_DIR	(1<<3)
#define QEI_INTF_ERR	(1<<4)
#define QEI_INTF_ENCLK	(1<<5)
#define QEI_INTF_POS0	(1<<6)
#define QEI_INTF_POS1	(1<<7)
#define QEI_INTF_POS2	(1<<8)
#define QEI_INTF_REV0	(1<<9)
#define QEI_INTF_POS0REV	(1<<10)
#define QEI_INTF_POS1REV	(1<<11)
#define QEI_INTF_POS2REV	(1<<12)
#define QEI_INTF_REV1	(1<<13)
#define QEI_INTF_REV2	(1<<14)
#define QEI_INTF_MAXPOS	(1<<15)

typedef enum
{
	QEI_INITF_NONE = 0,
	QEI_INITF_USE_INDEX = (1<<0),
	QEI_INITF_HIGH_RES = (1<<1),
	QEI_INITF_BACKWARDS = (1<<2),
} QEI_INIT_FLAGS;

void qei_initialize(int ppr, QEI_INIT_FLAGS flags);
unsigned int qei_read();

#endif // LPC15_QEI_H


