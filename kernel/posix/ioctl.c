#include "posix.h"
#include <sys/ioctl.h>
#include <exos/serial.h>
#include <stdarg.h>

int ioctl(int fd, int cmd, ...)
{
	va_list args;
	va_start(args, cmd);
	int done = 0;

	io_entry_t *io = posix_get_file_descriptor(fd);
	if (io == NULL) return posix_set_error(EBADF);

	// FIXME: let driver to return error if not a TTY
//	if (io->Type != EXOS_IO_COMM) return posix_set_error(ENOTTY);

/*
	switch(cmd)
	{
		case TIOCSRS485:
			if (copy_from_user(&rs485conf,
				(struct serial_rs485 *) arg,
				sizeof(rs485conf)))
					return -EFAULT;

			break;

		case TIOCGRS485:
			if (copy_to_user((struct serial_rs485 *) arg,
				...,
				sizeof(rs485conf)))
					return -EFAULT;
			break;

	}
*/
	return posix_set_error(EINVAL);
}
