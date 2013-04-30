/* Released under GPLv2 with exception for the OpenSSL library. See license.txt */
/* $Revision$ */

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

#include "error.h"
#include "gen.h"
#include "io.h"
#include "tcp.h"

void failure_close(int fd)
{
	struct linger sl;

	sl.l_onoff = 1;
	sl.l_linger = 0;

	if (setsockopt(fd, SOL_SOCKET, SO_LINGER, &sl, sizeof sl) == -1)
		set_error("could not set TCP_NODELAY on socket (%s)", strerror(errno));

	close(fd);
}

int connect_to(struct sockaddr *bind_to, struct addrinfo *ai, double timeout, char *tfo, char *msg, int msg_len, char *msg_accepted, int max_mtu)
{
	int     fd;
	int 	rc;
	struct timeval to;
	fd_set wfds;

	/* create socket */
	fd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
	if (fd == -1)
	{
		set_error("problem creating socket (%s)", strerror(errno));
		return RC_INVAL;
	}

	/* go through a specific interface? */
	if (bind_to)
	{
		int set = 1;

		/* set reuse flags */
		if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &set, sizeof set) == -1)
		{
			failure_close(fd);
			set_error("error setting sockopt to interface (%s)", strerror(errno));
			return RC_INVAL;
		}

		if (bind(fd, bind_to, sizeof *bind_to) == -1)
		{
			failure_close(fd);
			set_error("error binding to interface (%s)", strerror(errno));
			return RC_INVAL;
		}
	}

	/* make fd nonblocking */
	if (set_fd_nonblocking(fd) == -1)
	{
		failure_close(fd);
		return RC_INVAL;
	}

	if (max_mtu >= 0)
	{
		if (setsockopt(fd, IPPROTO_TCP, TCP_MAXSEG, &max_mtu, sizeof max_mtu) == -1)
		{
			failure_close(fd);
			set_error("error setting MTU size (%s)", strerror(errno));
			return RC_INVAL;
		}
	}

	/* wait for connection */
	FD_ZERO(&wfds);
	FD_SET(fd, &wfds);

	to.tv_sec  = (long)(timeout / 1000.0);
	to.tv_usec = (long)(timeout * 1000.0) % 1000000;

	/* connect to peer */
#ifdef TCP_TFO
	if (tfo && *tfo)
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
			goto old_connect;
		}
	}
			
	else
#else
	(void)tfo;
	(void)msg;
	(void)msg_len;
	(void)msg_accepted;
#endif
	{
		int rc = -1;

old_connect:
		rc = connect(fd, ai -> ai_addr, ai -> ai_addrlen);

		if (rc == 0)
		{
			/* connection made, return */
			return fd;
		}

		if (rc == -1)
		{
			/* problem connecting */
			if (errno != EINPROGRESS)
			{
				set_error("problem connecting to host: %s", strerror(errno));
				failure_close(fd);
				return RC_INVAL;
			}
		}
	}

	/* wait for connection */
	rc = select(fd + 1, NULL, &wfds, NULL, &to);
	if (rc == 0)
	{
		set_error("connect time out");
		failure_close(fd);
		return RC_TIMEOUT;	/* timeout */
	}
	else if (rc == -1)
	{
		failure_close(fd);

		if (errno == EINTR)
			return RC_CTRLC;/* ^C pressed */

		set_error("select() failed: %s", strerror(errno));

		return RC_INVAL;	/* error */
	}
	else
	{
		int optval=0;
		socklen_t optvallen = sizeof optval;

		/* see if the connect succeeded or failed */
		if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &optval, &optvallen) == -1)
		{
			set_error("getsockopt failed (%s)", strerror(errno));
			failure_close(fd);
			return RC_INVAL;
		}

		/* no error? */
		if (optval == 0)
			return fd;

		/* don't ask */
		errno = optval;
	}

	failure_close(fd);

	set_error("could not connect (%s)", strerror(errno));

	return RC_INVAL;
}

int set_tcp_low_latency(int sock)
{
	int flag = 1;

	if (setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int)) < 0)
	{
		set_error("could not set TCP_NODELAY on socket (%s)", strerror(errno));
		return -1;
	}

	return 0;
}
