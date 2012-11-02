#include "socket.h"
#include <errno.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <kernel/memory.h>
#include <net/udp_io.h>
#include <net/tcp_io.h>

int socket(int domain, int type, int protocol)
{
	if (domain != AF_INET)
		return posix_set_error(EAFNOSUPPORT);

	EXOS_IO_ENTRY *entry = NULL;
	switch (type)
	{
		case SOCK_DGRAM:
			if (protocol != 0) return posix_set_error(EPROTONOSUPPORT);
			entry = (EXOS_IO_ENTRY *)exos_mem_alloc(sizeof(UDP_IO_ENTRY), EXOS_MEMF_CLEAR);
			if (entry == NULL) return posix_set_error(ENOMEM);
			net_udp_io_create((UDP_IO_ENTRY *)entry, EXOS_IOF_WAIT);
			break;
		case SOCK_STREAM:
			if (protocol != 0) return posix_set_error(EPROTONOSUPPORT);
			entry = (EXOS_IO_ENTRY *)exos_mem_alloc(sizeof(TCP_IO_ENTRY), EXOS_MEMF_CLEAR);
			if (entry == NULL) return posix_set_error(ENOMEM);
			net_tcp_io_create((TCP_IO_ENTRY *)entry, EXOS_IOF_WAIT);
			break;
		default:
			return posix_set_error(EPROTOTYPE);
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
	if (io == NULL) return posix_set_error(EBADF);
	if (io->Type != EXOS_IO_SOCKET) return posix_set_error(ENOTSOCK);

	if (address->sa_family != AF_INET ||
		address_len != sizeof(struct sockaddr_in))
		return posix_set_error(EAFNOSUPPORT);

	struct sockaddr_in *ip_addr = (struct sockaddr_in *)address;
	IP_PORT_ADDR ipp = (IP_PORT_ADDR) { 
		.Address = (IP_ADDR)ip_addr->sin_addr.s_addr,
		.Port = ntohs(ip_addr->sin_port) };
	int error = net_io_bind((NET_IO_ENTRY *)io, &ipp);

	// TODO: translate error codes
	return error;
}

ssize_t recv(int socket, void *buffer, size_t length, int flags)
{
	return recvfrom(socket, buffer, length, flags, NULL, 0);
}

ssize_t recvfrom(int socket, void *buffer, size_t length, int flags, struct sockaddr *address, socklen_t *address_len)
{
	EXOS_IO_ENTRY *io = posix_get_file_descriptor(socket);
	if (io == NULL) return posix_set_error(EBADF);
	if (io->Type != EXOS_IO_SOCKET) return posix_set_error(ENOTSOCK);

	struct sockaddr_in *ip_addr = (struct sockaddr_in *)address;
	IP_PORT_ADDR ipp;
	int done;
	if (flags & MSG_PEEK)
	{
		return -1;	// TODO
	}
	else
	{
		done = net_io_receive((NET_IO_ENTRY *)io, buffer, length, &ipp);
	}
	if (done >= 0 && 
		ip_addr != NULL)
	{
		*ip_addr = (struct sockaddr_in) {
			.sin_family = AF_INET,
			.sin_addr.s_addr = ipp.Address.Value,
			.sin_port = htons(ipp.Port) };
		if (address_len != NULL) *address_len == sizeof(struct sockaddr_in);
	}
	else return posix_set_error(EWOULDBLOCK);

	return (ssize_t)done;
}

ssize_t send(int socket, const void *buffer, size_t length, int flags)
{
	return sendto(socket, buffer, length, flags, NULL, 0);
}

ssize_t sendto(int socket, const void *buffer, size_t length, int flags, const struct sockaddr *dest_addr, socklen_t dest_len)
{
	EXOS_IO_ENTRY *io = posix_get_file_descriptor(socket);
	if (io == NULL) return posix_set_error(EBADF);
	if (io->Type != EXOS_IO_SOCKET) return posix_set_error(ENOTSOCK);

	if (dest_addr->sa_family != AF_INET ||
		dest_len != sizeof(struct sockaddr_in))
		return posix_set_error(EAFNOSUPPORT);

	int done;
	struct sockaddr_in *ip_addr = (struct sockaddr_in *)dest_addr;
	if (ip_addr != NULL)
	{
		IP_PORT_ADDR ipp = (IP_PORT_ADDR) { 
			.Address = (IP_ADDR)ip_addr->sin_addr.s_addr,
			.Port = ntohs(ip_addr->sin_port) };
		done = net_io_send((NET_IO_ENTRY *)io, (void *)buffer, length, &ipp);
	}
	else
	{
		done = net_io_send((NET_IO_ENTRY *)io, (void *)buffer, length, NULL);
	}
	return (done >= 0) ? (ssize_t)done : posix_set_error(EIO);
}

int listen(int socket, int backlog)
{
	EXOS_IO_ENTRY *io = posix_get_file_descriptor(socket);
	if (io == NULL) return posix_set_error(EBADF);
	if (io->Type != EXOS_IO_SOCKET) return posix_set_error(ENOTSOCK);

	int error = net_io_listen((NET_IO_ENTRY *)io);
	return error;	// TODO: translate error codes
}

int accept(int sock_fd, struct sockaddr *address, socklen_t *address_len)
{
	EXOS_IO_ENTRY *io = posix_get_file_descriptor(sock_fd);
	if (io == NULL) return posix_set_error(EBADF);
	if (io->Type != EXOS_IO_SOCKET) return posix_set_error(ENOTSOCK);

	NET_IO_ENTRY *socket_io = (NET_IO_ENTRY *)io;
	if (socket_io->ProtocolType == NET_IO_STREAM)
	{
		int conn_fd = socket(AF_INET, SOCK_STREAM, 0);
		if (conn_fd >= 0)
		{
			unsigned buffer_size = socket_io->BufferSize != 0 ? 
				socket_io->BufferSize : EXOS_POSIX_DEFAULT_SOCKET_STREAM_SIZE;

			void *buffer = exos_mem_alloc(buffer_size * 2, EXOS_MEMF_CLEAR);
			if (buffer != NULL)
			{
				EXOS_IO_STREAM_BUFFERS desc = (EXOS_IO_STREAM_BUFFERS) {
					.RcvBuffer = buffer, .RcvBufferSize = buffer_size,
					.SndBuffer = buffer + buffer_size, .SndBufferSize = buffer_size };

				NET_IO_ENTRY *conn_socket_io = (NET_IO_ENTRY *)posix_get_file_descriptor(conn_fd);
				int error = net_io_accept(socket_io, conn_socket_io, &desc);
				if (error == 0)
				{
					// TODO: translate error codes
					return conn_fd;
				}

				exos_mem_free(buffer);
				posix_set_error(EAGAIN);
			}
			else posix_set_error(ENOMEM);	// could not allocate buffer for new socket

			close(conn_fd);
		}
		return -1;
	}
	return posix_set_error(EOPNOTSUPP);	// only supported for tcp sockets
}

int shutdown(int socket, int how)
{
	EXOS_IO_ENTRY *io = posix_get_file_descriptor(socket);
	if (io == NULL) return posix_set_error(EBADF);
	if (io->Type != EXOS_IO_SOCKET) return posix_set_error(ENOTSOCK);

	EXOS_IO_STREAM_BUFFERS desc;
	NET_IO_ENTRY *socket_io = (NET_IO_ENTRY *)io;
	int error = net_io_close(socket_io, &desc);
	if (error == -1) return posix_set_error(ENOTCONN);

	if (socket_io->ProtocolType == NET_IO_STREAM &&
		desc.RcvBuffer != NULL)
	{
		// NOTE: free both buffers at once, see accept()
		exos_mem_free(desc.RcvBuffer);
	}

	return 0;
}





