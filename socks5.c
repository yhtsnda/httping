#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <time.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "error.h"
#include "io.h"
#include "res.h"
#include "tcp.h"

int socks5connect(struct addrinfo *ai, int timeout, const char *socks5_username, const char *socks5_password, const char *host, int port)
{
	struct sockaddr_in sai;
	uint32_t addr = 0;
	unsigned char io_buffer[256] = { 0 };
	int io_len = 0;
	int fd = connect_to(NULL, ai, timeout, NULL, NULL, 0, NULL);

	if (fd == -1)
		error_exit("socks5connect: failed to connect to socks5 server");

	/* inform socks server about the auth. methods we support */
	if (socks5_username != NULL)
	{
		io_buffer[0] = 0x05;	/* version */
		io_buffer[1] = 2;	/* 2 authentication methods */
		io_buffer[2] = 0x00;	/* method 1: no authentication */
		io_buffer[3] = 0x02;	/* method 2: username/password */
		io_len = 4;
	}
	else
	{
		io_buffer[0] = 0x05;	/* version */
		io_buffer[1] = 1;	/* 2 authentication methods */
		io_buffer[2] = 0x00;	/* method 1: no authentication */
		io_len = 3;
	}

	if (mywrite(fd, (char *)io_buffer, io_len, timeout) == -1)
		error_exit("socks5connect: failed transmitting authentication methods to socks5 server");

	/* wait for reply telling selected authentication method */
	if (myread(fd, (char *)io_buffer, 2, timeout) == -1)
		error_exit("socks5ntp: socks5 server does not reply with selected auth. mode");

	if (io_buffer[0] != 0x05)
		error_exit("socks5connect: reply with requested authentication method does not say version 5 (%02x)", io_buffer[0]);

	if (io_buffer[1] == 0x00)
	{
		// printf("socks5connect: \"no authentication at all\" selected by server\n");
	}
	else if (io_buffer[1] == 0x02)
	{
		// printf("socks5connect: selected username/password authentication\n");
	}
	else
	{
		error_exit("socks5connect: socks5 refuses our authentication methods: %02x", io_buffer[1]);
	}

	/* in case the socks5 server asks us to authenticate, do so */
	if (io_buffer[1] == 0x02)
	{
		int io_len = 0;

		if (socks5_username == NULL || socks5_password == NULL)
			error_exit("socks5connect: socks5 server requests username/password authentication");

		io_buffer[0] = 0x01;	/* version */
		io_len = snprintf((char *)&io_buffer[1], sizeof io_buffer - 1, "%c%s%c%s", (int)strlen(socks5_username), socks5_username, (int)strlen(socks5_password), socks5_password);

		if (mywrite(fd, (char *)io_buffer, io_len + 1, timeout) == -1)
			error_exit("socks5connect: failed transmitting username/password to socks5 server");

		if (myread(fd, (char *)io_buffer, 2, timeout) == -1)
			error_exit("socks5connect: failed receiving authentication reply");

		if (io_buffer[1] != 0x00)
			error_exit("socks5connect: password authentication failed");
	}

	/* ask socks5 server to associate with sntp server */
	io_buffer[0] = 0x05;	/* version */
	io_buffer[1] = 0x01;	/* connect to */
	io_buffer[2] = 0x00;	/* reserved */
	io_buffer[3] = 0x01;	/* ipv4 */

	resolve_host_ipv4(host, &sai); // FIXME check rc
	addr = ntohl(sai.sin_addr.s_addr);

	io_buffer[4] = (addr >> 24) & 255;
	io_buffer[5] = (addr >> 16) & 255;
	io_buffer[6] = (addr >>  8) & 255;
	io_buffer[7] = (addr      ) & 255;

	io_buffer[8] = (port >> 8) & 255;
	io_buffer[9] = (port     ) & 255;

	if (mywrite(fd, (char *)io_buffer, 10, timeout) == -1)
		error_exit("socks5connect: failed to transmit associate request");

	if (myread(fd, (char *)io_buffer, 10, timeout) == -1)
		error_exit("socks5connect: command reply receive failure");

	/* verify reply */
	if (io_buffer[0] != 0x05)
		error_exit("socks5connect: bind request replies with version other than 0x05 (%02x)", io_buffer[0]);

	if (io_buffer[1] != 0x00)
		error_exit("socks5connect: failed to connect (%02x)", io_buffer[1]);

	if (io_buffer[3] != 0x01)
		error_exit("socks5connect: only accepting bind-replies with IPv4 address (%02x)", io_buffer[3]);

	return fd;
}
