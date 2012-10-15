#include <termios.h>
#include "posix.h"
#include <kernel/timer.h>

#include <comm/comm.h>
#define POSIX_VTIME_TICKS (EXOS_TICK_FREQ / 10)

void cfmakeraw(struct termios *termios_p)
{
	*termios_p = (struct termios) {
		.c_iflag = 0, // currently none supported
		.c_oflag = 0, // currently none supported
		.c_cflag = CS8 | CREAD | CLOCAL,
		.c_lflag = 0 };
}

static inline int _rndiv(int a, int b)
{
	return (a + (b / 2)) / b;
}

int tcgetattr(int fd, struct termios *termios_p)
{
	COMM_IO_ENTRY *io = (COMM_IO_ENTRY *)posix_get_file_descriptor(fd);
	if (io == NULL) return posix_set_error(EBADF);
	if (io->Type != EXOS_IO_COMM) return posix_set_error(ENOTTY);

	cfmakeraw(termios_p);
	termios_p->__baudrate = io->Baudrate;
	termios_p->c_cc[VMIN] = 1; // currently ignored
	termios_p->c_cc[VTIME] = _rndiv(io->Timeout, POSIX_VTIME_TICKS);
	return 0;
}

int tcsetattr(int fd, int optional_actions, const struct termios *termios_p)
{
	COMM_IO_ENTRY *io = (COMM_IO_ENTRY *)posix_get_file_descriptor(fd);
	if (io == NULL) return posix_set_error(EBADF);
	if (io->Type != EXOS_IO_COMM) return posix_set_error(ENOTTY);

	switch(optional_actions)
	{
		case TCSAFLUSH:
			// comm_io_flush(io);
		case TCSADRAIN:
			// comm_io_wait_output(io);
			break;
	}

	int error = comm_io_set_baudrate(io, termios_p->__baudrate);
	if (error != 0) return posix_set_error(EINVAL);

	exos_io_set_timeout((EXOS_IO_ENTRY *)io, 
		termios_p->c_cc[VTIME] * POSIX_VTIME_TICKS);

	return 0; 
}

speed_t cfgetispeed(const struct termios *termios_p)
{
	return termios_p->__baudrate;
}

speed_t cfgetospeed(const struct termios *termios_p)
{
	return termios_p->__baudrate;
}

int cfsetispeed(struct termios *termios_p, speed_t speed)
{
	termios_p->__baudrate = speed;
}

int cfsetospeed(struct termios *termios_p, speed_t speed)
{
	termios_p->__baudrate = speed;
}

int cfsetspeed(struct termios *termios_p, speed_t speed)
{
	termios_p->__baudrate = speed;
}


