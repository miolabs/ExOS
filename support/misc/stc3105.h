#ifndef STC3105_H
#define STC3105_H

#define STC3105_DEFAULT_ADDRESS (0b1110000)

typedef enum
{
	STC3105F_NONE = 0,
	STC3105F_POWER_SAVE_MODE = (1<<0),
} STC3105_FLAGS;

typedef enum
{
	STC3105_REG_MODE = 0,
	STC3105_REG_CTRL,
	STC3105_REG_CHARGE_LOW,
	STC3105_REG_CHARGE_HIGH,
	STC3105_REG_COUNTER_LOW,
	STC3105_REG_COUNTER_HIGH,
	STC3105_REG_CURRENT_LOW,
	STC3105_REG_CURRENT_HIGH,
	STC3105_REG_VOLTAGE_LOW,
	STC3105_REG_VOLTAGE_HIGH,
	STC3105_REG_SOC_BASE_LOW,
	STC3105_REG_SOC_BASE_HIGH,
	STC3105_REG_ALARM_SOC_LOW,
	STC3105_REG_ALARM_SOC_HIGH,
	STC3105_REG_ALARM_VOLTAGE,
	STC3105_REG_CURRENT_THRES,
	STC3105_REG_RELAX_COUNT,
	STC3105_REG_ID = 24,
	STC3105_REG_RAM0 = 32,
	STC3105_REG_RAM1,
	STC3105_REG_RAM2,
	STC3105_REG_RAM3,
	STC3105_REG_RAM4,
	STC3105_REG_RAM5,
	STC3105_REG_RAM6,
	STC3105_REG_RAM7,
	STC3105_REG_RAM8,
	STC3105_REG_RAM9,
	STC3105_REG_RAM10,
	STC3105_REG_RAM11,
	STC3105_REG_RAM12,
	STC3105_REG_RAM13,
	STC3105_REG_RAM14,
	STC3105_REG_RAM15
} STC3105_REG;

typedef enum
{
	STC3105_MODE_PWR_SAVE = (1<<2),
    STC3105_MODE_ALM_ENA = (1<<3),
    STC3105_MODE_GG_RUN = (1<<4),
} STC3105_MODE;

typedef enum
{
	STC3105_CTRL_IO0DATA = (1<<0),
	STC3105_CTRL_GG_RST = (1<<1),
	STC3105_CTRL_GG_EOC = (1<<2),
	STC3105_CTRL_VM_EOC = (1<<3),
	STC3105_CTRL_PORDET = (1<<4),
	STC3105_CTRL_ALM_SOC = (1<<5),
	STC3105_CTRL_ALM_VOLT = (1<<6),
} STC3105_CTRL;

typedef enum
{
	STC3105_UPD_NONE = 0,
	STC3105_UPD_VOLTAGE = (1<<0),
	STC3105_UPD_CURRENT = (1<<1),
	STC3105_UPD_CHARGE = (1<<2),
} STC3105_UPDATE;

// prototypes
int stc3105_initialize(int module, int i2c_addr, STC3105_FLAGS config);
STC3105_UPDATE stc3105_update(unsigned short *pvbat, unsigned short *pcurr, unsigned short *psoc);

#endif // STC3105_H

