#include "eeprom.h"
#include <kernel/panic.h>
#include <support/i2c_hal.h>


bool eeprom_read(const eeprom_context_t *context, void *data, unsigned addr, unsigned length)
{
	ASSERT(context != NULL, KERNEL_ERROR_NULL_POINTER);
	const eeprom_driver_t *driver = context->Driver;
	ASSERT(driver != NULL, KERNEL_ERROR_NULL_POINTER);
	unsigned pagesize = context->Geometry.PageSize != 0 ? context->Geometry.PageSize : 256;

	unsigned done = 0;
	while(done < length)
	{
		unsigned max_length = pagesize - (addr & (pagesize - 1));
		unsigned partial = (length - done) < max_length ? (length - done) : max_length;
		if (!driver->Read(context, data + done, addr, partial))
			return false;
		addr += partial;
		done += partial;
	}
	return true;
} 

bool eeprom_write(const eeprom_context_t *context, void *data, unsigned addr, unsigned length)
{
	ASSERT(context != NULL, KERNEL_ERROR_NULL_POINTER);
	const eeprom_driver_t *driver = context->Driver;
	ASSERT(driver != NULL, KERNEL_ERROR_NULL_POINTER);
	unsigned pagesize = context->Geometry.PageSize != 0 ? context->Geometry.PageSize : 256;

	unsigned done = 0;
	while(done < length)
	{
		unsigned max_length = pagesize - (addr & (pagesize - 1));
		unsigned partial = (length - done) < max_length ? (length - done) : max_length;
		if (!driver->Write(context, data + done, addr, partial))
			return false;
		addr += partial;
		done += partial;
		exos_thread_sleep(5);
	}
	return true;
}

bool eeprom_clear_all(const eeprom_context_t *context)
{
	ASSERT(context != NULL, KERNEL_ERROR_NULL_POINTER);
	const eeprom_driver_t *driver = context->Driver;
	ASSERT(driver != NULL, KERNEL_ERROR_NULL_POINTER);

	if (driver->Clear == NULL)
		return false;
	if (!driver->Clear(context))
		return false;
	
	exos_thread_sleep(5);
	return true;
}

#ifdef EEPROM_I2C_MODULE

static bool _read_small(const eeprom_context_t *context, unsigned char *data, unsigned addr, unsigned length)
{
	i2c_context_t i2c;
	unsigned char cmd[] = { addr & 0xFF };
	hal_i2c_init_context(&i2c, context->Geometry.Address | (addr >> 8), cmd, 1);
	bool done = hal_i2c_read(EEPROM_I2C_MODULE, &i2c, data, length);
	return done;
}

static bool _write_small(const eeprom_context_t *context, unsigned char *data, unsigned addr, unsigned length)
{
	i2c_context_t i2c;
	unsigned char cmd[] = { addr & 0xFF };
	hal_i2c_init_context(&i2c, context->Geometry.Address | (addr >> 8), cmd, 1);
	bool done = hal_i2c_write(EEPROM_I2C_MODULE, &i2c, data, length);
	return done;
}

static bool _read(const eeprom_context_t *context, unsigned char *data, unsigned addr, unsigned length)
{
	i2c_context_t i2c;
	unsigned char cmd[] = { addr >> 8, addr & 0xFF };
	hal_i2c_init_context(&i2c, context->Geometry.Address, cmd, 2);
	bool done = hal_i2c_read(EEPROM_I2C_MODULE, &i2c, data, length);
	return done;
}

static bool _write(const eeprom_context_t *context, unsigned char *data, unsigned addr, unsigned length)
{
	i2c_context_t i2c;
	unsigned char cmd[] = { addr >> 8, addr & 0xFF };
	hal_i2c_init_context(&i2c, context->Geometry.Address, cmd, 2);
	bool done = hal_i2c_write(EEPROM_I2C_MODULE, &i2c, data, length);
	return done;
}

static const eeprom_driver_t _driver_i2c_small = { .Read = _read_small, .Write = _write_small };
static const eeprom_driver_t _driver_i2c = { .Read = _read, .Write = _write };

void eeprom_i2c_context_create(eeprom_context_t *context, const eeprom_geometry_t *geo)
{
	ASSERT(context != NULL, KERNEL_ERROR_NULL_POINTER);
	ASSERT(geo != NULL, KERNEL_ERROR_NULL_POINTER);
	
	if (geo->Size < 1024)
		eeprom_context_create(context, geo, &_driver_i2c_small, NULL);
	else
		eeprom_context_create(context, geo, &_driver_i2c, NULL);
}

#endif


