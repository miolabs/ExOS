//
//  uart.c
//  UARTTest
//
//  Created by GodShadow on 04/09/12.
//  Copyright (c) 2012 MIOLabs. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>
#include <fcntl.h>   /* File control definitions */
#include <errno.h>   /* Error number definitions */
#include <termios.h> /* POSIX terminal control definitions */
#include <sys/ioctl.h>
#include <sys/select.h>
#include <unistd.h>
#include <string.h>

#ifdef __APPLE__
#define kUARTDevice "/dev/cu.usbserial-FTB4ND9F"
#elif defined __EXOS__
#include <exos/serial.h>
#define kUARTDevice "/dev/comm1"
#endif

#define kBaudRate       115200
#define kBufferBytes    256

int OpenUART();
void CloseUART(int fd);

int main(int argc, const char * argv[])
{
    int fd = OpenUART();
    if (fd < 0)
        return -1;
    
    // Main Loop
    fd_set readfds;
        
    FD_ZERO(&readfds);
    FD_SET(fd, &readfds);
    
    bool exit = false;
    char buffer[kBufferBytes];
    
    while (!exit)
    {
        // don't care about writefds and exceptfds:
        int fdCount = 0;
        fdCount = select(fd + 1, &readfds, NULL, NULL, NULL);
    
        if (fdCount > 0)
        {
            if (FD_ISSET(fd, &readfds))
            {
                int length = read(fd, buffer, kBufferBytes);
				write(fd, buffer, length);
                printf("%s\n", buffer);
            }
        }
        else if (fdCount == -1)
            exit = true;
    }
    CloseUART(fd);
}

int OpenUART()
{    
    int fd = -1;
    struct termios originalTTYAttrs;
    struct termios options;    
    
    fd = open(kUARTDevice, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd < 0)
        return -1;
    
    // Note that open() follows POSIX semantics: multiple open() calls to the same file will succeed
    // unless the TIOCEXCL ioctl is issued. This will prevent additional opens except by root-owned
    // processes.
    // See tty(4) ("man 4 tty") and ioctl(2) ("man 2 ioctl") for details.
    
//    if (ioctl(fd, TIOCEXCL) == -1)
//    {
//        printf("Error setting TIOCEXCL on %s - %s(%d).\n",
//               kUARTDevice, strerror(errno), errno);
//        goto error;
//    }

	struct serial_rs485 rs485cfg = (struct serial_rs485) {
		//
		};
	if (ioctl(fd, TIOCSRS485, &rs485cfg) == -1)
	{
		printf("Cannot set rs485 mode");
		goto error;
	}

    // Now that the device is open, clear the O_NONBLOCK flag so subsequent I/O will block.
    // See fcntl(2) ("man 2 fcntl") for details.
    
    if (fcntl(fd, F_SETFL, 0) == -1)
    {
        printf("Error clearing O_NONBLOCK %s - %s(%d).\n",
               kUARTDevice, strerror(errno), errno);
        goto error;
    }
    
    // Get the current options and save them so we can restore the default settings later.
    if (tcgetattr(fd, &originalTTYAttrs) == -1)
    {
        printf("Error getting tty attributes %s - %s(%d).\n",
               kUARTDevice, strerror(errno), errno);
        goto error;
    }
    
    // The serial port attributes such as timeouts and baud rate are set by modifying the termios
    // structure and then calling tcsetattr() to cause the changes to take effect. Note that the
    // changes will not become effective without the tcsetattr() call.
    // See tcsetattr(4) ("man 4 tcsetattr") for details.
    
    options = originalTTYAttrs;
    
    // Print the current input and output baud rates.
    // See tcsetattr(4) ("man 4 tcsetattr") for details.
    
    printf("Current input baud rate is %d\n", (int) cfgetispeed(&options));
    printf("Current output baud rate is %d\n", (int) cfgetospeed(&options));
    
    // Set raw input (non-canonical) mode, with reads blocking until either a single character
    // has been received or a one second timeout expires.
    // See tcsetattr(4) ("man 4 tcsetattr") and termios(4) ("man 4 termios") for details.
    
    cfmakeraw(&options);
    options.c_cc[VMIN] = 1;
    options.c_cc[VTIME] = 10;
    
    // The baud rate, word length, and handshake options can be set as follows:
    cfsetspeed(&options, kBaudRate);    // Set 19200 baud
    options.c_cflag |= (CS8);  // RTS flow control of input    
    
    printf("Input baud rate changed to %d\n", (int) cfgetispeed(&options));
    printf("Output baud rate changed to %d\n", (int) cfgetospeed(&options));
    
    // Cause the new options to take effect immediately.
    if (tcsetattr(fd, TCSANOW, &options) == -1)
    {
        printf("Error setting tty attributes %s - %s(%d).\n",
               kUARTDevice, strerror(errno), errno);
        goto error;
    }
    
    return fd;
    
    // Failure
error:
    CloseUART(fd);
    
    return -1;
}

void CloseUART(int fd)
{
    if (fd > -1)
        close(fd);
}

