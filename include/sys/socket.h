#ifndef __posix_sys_socket_h
#define __posix_sys_socket_h

#include <stdint.h>
#include <sys/types.h>
//#include <sys/uio.h>

typedef uint32_t socklen_t;
typedef uint32_t sa_family_t;

struct sockaddr
{
	sa_family_t sa_family;
	char sa_data[];
};

struct sockaddr_storage
{
	sa_family_t sa_family;
};

struct msghdr
{
	void *msg_name;
	socklen_t msg_namelen;
	struct iovec *msg_iov;
	int mag_iovlen;
	void *msg_control;
	socklen_t msg_controllen;
	int msg_flags;
};

struct cmsghdr
{
	socklen_t cmsg_len;
	int cmsg_level;
	int cmsg_type;
};

enum
{
	SOL_SOCKET = 0,
};

// TODO: SCM_RIGHTS

// TODO: CMSG_ macros

struct linger
{
	int l_onoff;	// Indicates whether linger option is enabled
	int l_linger;	// Linger time, in seconds
};

enum
{
	SOCK_DGRAM,
	SOCK_RAW,
	SOCK_SEQPACKET,
	SOCK_STREAM,
};

enum
{
	SO_ACCEPTCONN,
	SO_BROADCAST,
	SO_DEBUG,
	SO_DONTROUTE,
	SO_ERROR,
	SO_KEEPALIVE,
	SO_LINGER,
	SO_OOBINLINE,
	SO_RCVBUF,
	SO_RCVLOWAT,
	SO_RCVTIMEO,
	SO_REUSEADDR,
	SO_SNDBUF,
	SO_SNDLOWAT,
	SO_SNDTIMEO,
	SO_TYPE,
};

enum
{
	MSG_CTRUNC,
	MSG_DONTROUTE,
	MSG_EOR,
	MSG_OOB,
	MSG_NOSIGNAL,
	MSG_PEEK,
	MSG_TRUNC,
	MSG_WAITALL,
};

enum
{
	AF_INET,
	AF_INET6,
	AF_UNIX,
	AF_UNSPEC,
};

enum
{
	SHUT_RD = 1,
	SHUT_WR = 2,
	SHUT_RDWR = (SHUT_RD | SHUT_WR),
};

int accept(int socket, struct sockaddr *address, socklen_t *address_len);
int bind(int socket, const struct sockaddr *address, socklen_t address_len);
int connect(int socket, const struct sockaddr *address, socklen_t address_len);
int getpeername(int socket, struct sockaddr *address, socklen_t *address_len);
int getsockname(int socket, struct sockaddr *address, socklen_t *address_len);
int getsockopt(int socket, int level, int option_name, void *option_value, socklen_t *option_len);
int listen(int socket, int backlog);
ssize_t recv(int socket, void *buffer, size_t length, int flags);
ssize_t recvfrom(int socket, void *buffer, size_t length, int flags, struct sockaddr *address, socklen_t *address_len);
ssize_t recvmsg(int socket, struct msghdr *message, int flags);
ssize_t send(int socket, const void *buffer, size_t length, int flags);
ssize_t sendto(int socket, const void *buffer, size_t length, int flags, const struct sockaddr *dest_addr, socklen_t dest_len);
ssize_t sendmsg(int socket, const struct msghdr *message, int flags);
int setsockopt(int socket, int level, int option_name, const void *option_value, socklen_t option_len);
int shutdown(int socket, int how);
int sockatmark(int s);
int socket(int domain, int type, int protocol);
int socketpair(int domain, int type, int protocol, int socket_vector[2]);

#endif // __posix_sys_socket_h

