#ifndef __posix_termios_h
#define __posix_termios_h

typedef unsigned char cc_t;
typedef unsigned long speed_t;
typedef unsigned short tcflag_t;

enum
{
	VEOF, VEOL, VERASE, VINTR, VKILL, VMIN, VQUIT, VSTART, VSTOP, VSUSP, VTIME,
	NCCS
};

struct termios
{
	tcflag_t c_iflag;
	tcflag_t c_oflag;
	tcflag_t c_cflag;
	tcflag_t c_lflag;
	speed_t __baudrate; 
	cc_t c_cc[NCCS];
};

// Control Modes
// for use in the c_cflag field
enum 
{
	CSIZE = 0xF,
	CS5 = 5,
	CS6 = 6,
	CS7 = 7,
	CS8 = 8,
	CSTOPB = (1<<4),
	CREAD = (1<<5),
	PARENB = (1<<6),
	PARODD = (1<<7),
	HUPCL = (1<<8),
	CLOCAL = (1<<9),
};


// Attribute Selection
// for use with tcsetattr()
enum
{
	TCSANOW = 0,
	TCSADRAIN = 1,
	TCSAFLUSH = 2,
};

int tcgetattr(int fd, struct termios *termios_p);
int tcsetattr(int fd, int optional_actions, const struct termios *termios_p);
void cfmakeraw(struct termios *termios_p);

speed_t cfgetispeed(const struct termios *termios_p);
speed_t cfgetospeed(const struct termios *termios_p);
int cfsetispeed(struct termios *termios_p, speed_t speed);
int cfsetospeed(struct termios *termios_p, speed_t speed);
int cfsetspeed(struct termios *termios_p, speed_t speed);

#endif // __posix_termios_h

