#ifndef DM36X_LIBMEM_DRIVER_H
#define DM36X_LIBMEM_DRIVER_H

#include <libmem.h>

int libmem_register_ddr_driver(libmem_driver_handle_t *h, uint8_t *start, size_t size);


#endif // DM36X_LIBMEM_DRIVER_H
