#include "driver.h"
#include <string.h>

static int
libmem_write_impl(libmem_driver_handle_t *h, uint8_t *dest, const uint8_t *src, size_t size)
{
  memcpy(dest, src, size);
  return LIBMEM_STATUS_SUCCESS;
}

static int
libmem_fill_impl(libmem_driver_handle_t *h, uint8_t *dest, uint8_t c, size_t size)
{
  memset(dest, c, size);
  return LIBMEM_STATUS_SUCCESS;
}

static int
libmem_erase_impl(libmem_driver_handle_t *h, uint8_t *start, size_t size, 
                  uint8_t **erase_start, size_t *erase_size)
{
	// TODO: Implement memory erase operation.
	if (erase_start)
	{
		// TODO: Set erase_start to point to the start of the memory block that
		//       has been erased. For now we'll just return the requested start in
		//       order to keep the caller happy.
		*erase_start = start;
	}
	if (erase_size)
	{
		// TODO: Set erase_size to the size of the memory block that has been
		//       erased. For now we'll just return the requested size in order to
		//       keep the caller happy.
		*erase_size = size;
	}
	return LIBMEM_STATUS_SUCCESS;
}

static int
libmem_lock_impl(libmem_driver_handle_t *h, uint8_t *dest, size_t size)
{
	// TODO: Implement memory lock operation
	return LIBMEM_STATUS_SUCCESS;
}

static int
libmem_unlock_impl(libmem_driver_handle_t *h, uint8_t *dest, size_t size)
{
	// TODO: Implement memory unlock operation.
	return LIBMEM_STATUS_SUCCESS;
}

static int
libmem_flush_impl(libmem_driver_handle_t *h)
{
  // TODO: Implement memory flush operation.
  return LIBMEM_STATUS_SUCCESS;
}

static const libmem_driver_functions_t driver_functions =
{
  libmem_write_impl,
  libmem_fill_impl,
  libmem_erase_impl,
  libmem_lock_impl,
  libmem_unlock_impl,
  libmem_flush_impl
};

int libmem_register_ddr_driver(libmem_driver_handle_t *h, uint8_t *start, size_t size)
{
  libmem_register_driver(h, start, size, 0, 0, &driver_functions, 0);
  return LIBMEM_STATUS_SUCCESS;
}

