#ifndef MISC_EEPROM_H
#define MISC_EEPROM_H

#include <stdbool.h>

typedef struct __eeprom_context eeprom_context_t; 

typedef struct
{
	bool (*Clear)(const eeprom_context_t *context);
	bool (*Read)(const eeprom_context_t *context, unsigned char *data, unsigned addr, unsigned length);
	bool (*Write)(const eeprom_context_t *context, unsigned char *data, unsigned addr, unsigned length);
	void (*Lock)(const eeprom_context_t *context, bool lock);
} eeprom_driver_t;

typedef struct
{
	unsigned short Size;
	unsigned char PageSize;
	unsigned char Address;
} eeprom_geometry_t;

struct __eeprom_context
{
	eeprom_geometry_t Geometry;
	const eeprom_driver_t *Driver;
	void *DriverContext;
};

static inline void eeprom_geometry_create(eeprom_geometry_t *geo, unsigned char addr, unsigned size, unsigned page_size)
{
	*geo = (eeprom_geometry_t) { .Address = addr, .Size = size, .PageSize = page_size };
}

static inline void eeprom_context_create(eeprom_context_t *context, const eeprom_geometry_t *geo, const eeprom_driver_t *driver, void *driver_context)
{
	*context = (eeprom_context_t) { .Geometry = *geo, .Driver = driver, .DriverContext = driver_context };
}

// prototypes
bool eeprom_clear_all(const eeprom_context_t *context);
bool eeprom_read(const eeprom_context_t *context, void *data, unsigned addr, unsigned length);
bool eeprom_write(const eeprom_context_t *context, void *data, unsigned addr, unsigned length);
void eeprom_context_create(eeprom_context_t *context, const eeprom_geometry_t *geo, const eeprom_driver_t *driver, void *driver_context);

void eeprom_i2c_context_create(eeprom_context_t *context, const eeprom_geometry_t *geo);
void eeprom_i2c_lock(const eeprom_context_t *context, bool lock);

#endif // MISC_EEPROM_H


