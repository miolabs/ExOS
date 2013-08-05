#include <comm/comm.h>

//#include <sys/types.h> 
//#include <sys/socket.h> 
//#include <netinet/in.h> 
//#include <netdb.h> 
//#include <stdio.h> 
//#include <stdlib.h> 
//
//#include <arpa/inet.h>  
//#include <errno.h>
//#include <unistd.h>  

static char buffer[1024];

int main(void) 
{ 
	comm_initialize();	// initialize hardware and begin driver initialization

	COMM_IO_ENTRY io;
	comm_io_create(&io, NULL, 0, EXOS_IOF_WAIT);

	if (comm_io_open(&io, 115200))
	{
		while(1)
		{
			int n = exos_io_read((EXOS_IO_ENTRY *)&io, buffer, 1024);
		}
	}
}


