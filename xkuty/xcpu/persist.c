#include "persist.h"
#include <support/misc/iap.h>
#include <support/misc/eeprom.h>
#include <kernel/mutex.h>
#include <kernel/panic.h>

static EXOS_MUTEX _i2c_mutex;

int persist_load(XCPU_PERSIST_DATA *data)
{
	if (sizeof(XCPU_PERSIST_DATA) > XCPU_PERSIST_MAX_SIZE)
		kernel_panic(KERNEL_ERROR_UNKNOWN);

	if (!eeprom_initialize())
		return 0;

	exos_mutex_create(&_i2c_mutex);

	EEPROM_RESULT res = eeprom_read((void *)data, 0, sizeof(XCPU_PERSIST_DATA));
	if (res != EEPROM_RES_OK)
		return 0;

	return data->Magic == XCPU_PERSIST_MAGIC;	// FIXME: checksum data or whatever
}

int persist_save(const XCPU_PERSIST_DATA *data)
{
	EEPROM_RESULT res = eeprom_write((void *)data, 0, sizeof(XCPU_PERSIST_DATA));
	return res == EEPROM_RES_OK;
}

void persist_enter_bootloader()
{
	iap_reinvoke_isp();
}

void eeprom_lock_i2c()
{
	exos_mutex_lock(&_i2c_mutex);
}

void eeprom_unlock_i2c()
{
	exos_mutex_unlock(&_i2c_mutex);
}

