#include "eeprom.h"
#include <kernel/panic.h>
#include <support/i2c_hal.h>


bool eeprom_read(const eeprom_context_t *context, void *data, unsigned addr, unsigned length)
{
	ASSERT(context != NULL, KERNEL_ERROR_NULL_POINTER);
	const eeprom_driver_t *driver = context->Driver;
	ASSERT(driver != NULL, KERNEL_ERROR_NULL_POINTER);
	unsigned pagesize = context->Geometry.PageSize != 0 ? context->Geometry.PageSize : 256;

	if (driver->Lock != NULL)
		driver->Lock(context, true);

	unsigned done = 0;
	while(done < length)
	{
		unsigned max_length = pagesize - (addr & (pagesize - 1));
		unsigned partial = (length - done) < max_length ? (length - done) : max_length;
		if (!driver->Read(context, data + done, addr, partial))
			break;
		addr += partial;
		done += partial;
	}

	if (driver->Lock != NULL)
		driver->Lock(context, false);

	return done == length;
} 

bool eeprom_write(const eeprom_context_t *context, void *data, unsigned addr, unsigned length)
{
	ASSERT(context != NULL, KERNEL_ERROR_NULL_POINTER);
	const eeprom_driver_t *driver = context->Driver;
	ASSERT(driver != NULL, KERNEL_ERROR_NULL_POINTER);
	unsigned pagesize = context->Geometry.PageSize != 0 ? context->Geometry.PageSize : 256;

	if (driver->Lock != NULL)
		driver->Lock(context, true);

	unsigned done = 0;
	while(done < length)
	{
		unsigned max_length = pagesize - (addr & (pagesize - 1));
		unsigned partial = (length - done) < max_length ? (length - done) : max_length;
		if (!driver->Write(context, data + done, addr, partial))
			break;
		addr += partial;
		done += partial;
		exos_thread_sleep(5);
	}

	if (driver->Lock != NULL)
		driver->Lock(context, false);

	return done == length;
}

bool eeprom_clear_all(const eeprom_context_t *context)
{
	ASSERT(context != NULL, KERNEL_ERROR_NULL_POINTER);
	const eeprom_driver_t *driver = context->Driver;
	ASSERT(driver != NULL, KERNEL_ERROR_NULL_POINTER);

	bool done = false;
	if (driver->Clear != NULL)
	{
		if (driver->Lock != NULL)
			driver->Lock(context, true);

		if (driver->Clear(context))
		{
			done = true;
			
			exos_thread_sleep(5);
		}

		if (driver->Lock != NULL)
			driver->Lock(context, false);
	}
	return done;
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

__weak
void eeprom_i2c_lock(const eeprom_context_t *context, bool lock)
{
	// nothing
}

static const eeprom_driver_t _driver_i2c_small = { .Read = _read_small, .Write = _write_small, .Lock = eeprom_i2c_lock };
static const eeprom_driver_t _driver_i2c = { .Read = _read, .Write = _write, .Lock = eeprom_i2c_lock  };

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


