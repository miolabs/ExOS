#include "socket.h"
#include <errno.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <kernel/memory.h>
#include <net/udp_io.h>

int socket(int domain, int type, int protocol)
{
	if (domain != AF_INET)
		return EAFNOSUPPORT;

	EXOS_IO_ENTRY *entry = NULL;
	switch (type)
	{
		case SOCK_DGRAM:
			if (protocol != 0) return EPROTONOSUPPORT;
			entry = (EXOS_IO_ENTRY *)exos_mem_alloc(sizeof(UDP_IO_ENTRY), EXOS_MEMF_CLEAR);
			if (entry == NULL) return ENOMEM;
			net_udp_create_io((UDP_IO_ENTRY *)entry, EXOS_IOF_WAIT);
			break;
		default:
			return EPROTOTYPE;
	}

	int fd = posix_add_file_descriptor(entry);
	if (fd < 0) 
	{
		exos_mem_free(entry);
		return fd;
	}
	return fd;
}

int bind(int socket, const struct sockaddr *address, socklen_t address_len)
{
	EXOS_IO_ENTRY *io = posix_get_file_descriptor(socket);
	if (io == NULL) return EBADF;
	if (io->Type != EXOS_IO_SOCKET) return ENOTSOCK;

	if (address->sa_family != AF_INET ||
		address_len != sizeof(struct sockaddr_in))
		return EAFNOSUPPORT;

	// FIXME: check that this was a DGRAM socket

	struct sockaddr_in *udp_addr = (struct sockaddr_in *)address;
	IP_PORT_ADDR ip = (IP_PORT_ADDR) { 
		.Address = (IP_ADDR)udp_addr->sin_addr.s_addr,
		.Port = ntohs(udp_addr->sin_port) };
	int error = net_io_bind((NET_IO_ENTRY *)io, &ip);

	// TODO: translate error codes
	return error;
}

ssize_t recv(int socket, void *buffer, size_t length, int flags)
{
}

ssize_t recvfrom(int socket, void *buffer, size_t length, int flags, struct sockaddr *address, socklen_t *address_len)
{
	EXOS_IO_ENTRY *io = posix_get_file_descriptor(socket);
	if (io == NULL) return EBADF;
	if (io->Type != EXOS_IO_SOCKET) return posix_set_error(ENOTSOCK);

	struct sockaddr_in *udp_addr = (struct sockaddr_in *)address;
	IP_PORT_ADDR ip;
	int done;
	if (flags & MSG_PEEK)
	{
		return -1;	// TODO
	}
	else
	{
		done = net_io_receive((NET_IO_ENTRY *)io, buffer, length, &ip);
	}
	if (done >= 0)
	{
		*udp_addr = (struct sockaddr_in) {
			.sin_family = AF_INET,
			.sin_addr.s_addr = ip.Address.Value,
			.sin_port = htons(ip.Port) };
	}
	else return posix_set_error(EWOULDBLOCK);

	return (ssize_t)done;
}

ssize_t send(int socket, const void *buffer, size_t length, int flags)
{
}

ssize_t sendto(int socket, const void *buffer, size_t length, int flags, const struct sockaddr *dest_addr, socklen_t dest_len)
{
	EXOS_IO_ENTRY *io = posix_get_file_descriptor(socket);
	if (io == NULL) return EBADF;
	if (io->Type != EXOS_IO_SOCKET) return ENOTSOCK;

	if (dest_addr->sa_family != AF_INET ||
		dest_len != sizeof(struct sockaddr_in))
		return EAFNOSUPPORT;

	struct sockaddr_in *udp_addr = (struct sockaddr_in *)dest_addr;
	IP_PORT_ADDR ip = (IP_PORT_ADDR) { 
		.Address = (IP_ADDR)udp_addr->sin_addr.s_addr,
		.Port = ntohs(udp_addr->sin_port) };
	
	int done = net_io_send((NET_IO_ENTRY *)io, (void *)buffer, length, &ip);
	return (done >= 0) ? (ssize_t)done : EIO;
}




