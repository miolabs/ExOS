#ifndef __posix_netinet_in_h
#define __posix_netinet_in_h

#include <inttypes.h>
#include <sys/socket.h>

typedef uint16_t in_port_t;
typedef uint32_t in_addr_t;

struct in_addr
{
	in_addr_t s_addr;
};

struct sockaddr_in
{
	sa_family_t sin_family;		// AF_INET
	in_port_t sin_port;			// Port number
	struct in_addr sin_addr;	// IP addr
};

enum
{
	IPPROTO_IP,
	IPPORTO_IPV6,
	IPPROTO_ICMP,
	IPPROTO_RAW,
	IPPROTO_TCP,
	IPPROTO_UDP,
};

#define INADDR_ANY	((in_addr_t)0)
#define INADDR_BROADCAST ((in_addr_t)0xffffffff)

//#define INET_ADDRSTRLEN

#include <arpa/inet.h>

#endif // __posix_netinet_in_h

