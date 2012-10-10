#include <termios.h>
#include "posix.h"

#include <comm/comm.h>

int tcgetattr(int fildes, struct termios *termios_p)
{
	return -1;
}

int tcsetattr(int fildes, int optional_actions, const struct termios *termios_p)
{
	return -1;
}

void cfmakeraw(struct termios *termios_p)
{

}

speed_t cfgetispeed(const struct termios *termios_p)
{
	return -1;
}

speed_t cfgetospeed(const struct termios *termios_p)
{
	return -1;
}

int cfsetispeed(struct termios *termios_p, speed_t speed)
{
	return -1;
}

int cfsetospeed(struct termios *termios_p, speed_t speed)
{
	return -1;
}

int cfsetspeed(struct termios *termios_p, speed_t speed)
{
	return -1;
}


