/* Released under GPLv2 with exception for the OpenSSL library. See license.txt */

#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "gen.h"
#include "io.h"
#include "tcp.h"

int connect_to(struct sockaddr *bind_to, struct addrinfo *ai, int timeout, char *tfo, char *msg, int msg_len, int *msg_accepted)
{
	int     fd;
	int 	rc;
	struct timeval to;
	fd_set wfds;

	/* create socket */
	fd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
	if (fd == -1)
	{
		snprintf(last_error, sizeof last_error, "problem creating socket (%s)", strerror(errno));
		return -1;
	}

	/* go through a specific interface? */
	if (bind_to)
	{
		int set = 1;

		/* set reuse flags */
		if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &set, sizeof set) == -1)
		{
			close(fd);
			snprintf(last_error, sizeof last_error, "error setting sockopt to interface (%s)", strerror(errno));
			return -1;
		}

		if (bind(fd, bind_to, sizeof *bind_to) == -1)
		{
			close(fd);
			snprintf(last_error, sizeof last_error, "error binding to interface (%s)", strerror(errno));
			return -1;
		}
	}

	/* make fd nonblocking */
	if (set_fd_nonblocking(fd) == -1)
	{
		close(fd);
		return -1;
	}

	/* wait for connection */
	FD_ZERO(&wfds);
	FD_SET(fd, &wfds);

	to.tv_sec  = timeout / 1000;
	to.tv_usec = (timeout - (to.tv_sec * 1000)) * 1000;

	/* connect to peer */
#ifdef TCP_TFO
	if (*tfo)
	{
		rc = sendto(fd, msg, msg_len, MSG_FASTOPEN, ai -> ai_addr, ai -> ai_addrlen);
		
		if(rc == msg_len)
			*msg_accepted = 1;
		if(errno == 0)
			return fd;
		if(errno == ENOTSUP)
		{
			printf("TCP TFO Not Supported. Please check if \"/proc/sys/net/ipv4/tcp_fastopen\" is 1. Disabling TFO for now.\n");
			*tfo = 0;
		}
	}
			
	else
#endif
	{
		int rc = connect(fd, ai -> ai_addr, ai -> ai_addrlen);

		if (rc == 0)
		{
			/* connection made, return */
			return fd;
		}

		if (rc == -1)
		{
			// problem connecting
			if (errno != EINPROGRESS)
			{
				snprintf(last_error, sizeof last_error, "problem connecting to host: %s\n", strerror(errno));
				close(fd);
				return -1;
			}
		}
	}

	/* wait for connection */
	rc = select(fd + 1, NULL, &wfds, NULL, &to);
	if (rc == 0)
	{
		snprintf(last_error, sizeof last_error, "connect time out\n");
		close(fd);
		return RC_TIMEOUT;	/* timeout */
	}
	else if (rc == -1)
	{
		close(fd);

		if (errno == EINTR)
			return RC_CTRLC;/* ^C pressed */

		snprintf(last_error, sizeof last_error, "select() failed: %s\n", strerror(errno));

		return -1;	/* error */
	}
	else
	{
		int optval=0;
		socklen_t optvallen=sizeof(optval);

		/* see if the connect succeeded or failed */
		if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &optval, &optvallen) == -1)
		{
			snprintf(last_error, sizeof last_error, "getsockopt failed (%s)\n", strerror(errno));
			close(fd);
			return -1;
		}

		/* no error? */
		if (optval == 0)
			return fd;

		/* don't ask */
		errno = optval;
	}

	close(fd);

	snprintf(last_error, sizeof last_error, "could not connect (%s)\n", strerror(errno));

	return -1;
}

int set_tcp_low_latency(int sock)
{
	int flag = 1;

	if (setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int)) < 0)
	{
		snprintf(last_error, sizeof last_error, "could not set TCP_NODELAY on socket (%s)\n", strerror(errno));
		return -1;
	}

	return 0;
}
