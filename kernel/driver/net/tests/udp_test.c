#include <sys/types.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <netdb.h> 
#include <stdio.h> 
#include <stdlib.h> 

#include <arpa/inet.h>  
#include <errno.h>
#include <unistd.h>  

struct Test 
{ 
	char name[20]; 
	int  num; 
} test_t; 

static char buf[1024];

int main(void) 
{ 
	struct Test test_name;

	int sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0) return -1;

	struct sockaddr_in server = (struct sockaddr_in) {
		.sin_family = AF_INET,
		.sin_addr.s_addr = INADDR_ANY,
		.sin_port = htons(1818) };
	socklen_t length = sizeof(server);
	
	if (bind(sock, (struct sockaddr *)&server, length) < 0)
		return -2; // error binding

	socklen_t fromlen = sizeof(struct sockaddr_in);
	while (1)
	{
		struct sockaddr_in from; 
		int n = recvfrom(sock, buf, 1024, 0, (struct sockaddr *)&from, &fromlen);
		if (n < 0) return -3; // recvfrom

		int lenght = n;
		int done = 0;
		for(int i = 0; i < 20; i++)
		{
			n = sendto(sock, buf, lenght, 0, (struct sockaddr *)&from, fromlen);
			if (n < 0) break; 
			done++;
		}
		length = sprintf(buf, "%d packets sent", done);
		n = sendto(sock, buf, length, 0, (struct sockaddr *)&from, fromlen);
        if (n < 0) return -4;
	}
}


