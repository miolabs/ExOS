#ifndef __posix_exos_serial_h
#define __posix_exos_serial_h

#define TIOCGRS485      0x542E
#define TIOCSRS485      0x542F

struct serial_rs485
{
	unsigned char flags;
};

#endif // __posix_exos_serial_h

