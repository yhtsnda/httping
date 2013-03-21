/* Released under GPLv2 with exception for the OpenSSL library. See license.txt */

#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "gen.h"
#include "res.h"
#include "error.h"

int resolve_host(const char *host, struct addrinfo **ai, char use_ipv6, int portnr)
{
	char servname[10];
	struct addrinfo myaddr;
	memset(&myaddr, 0, sizeof myaddr);
	/* myaddr.ai_flags = AI_PASSIVE; */
	myaddr.ai_socktype = SOCK_STREAM;
	myaddr.ai_protocol = IPPROTO_TCP;
	myaddr.ai_family = use_ipv6 ? AF_INET6 : AF_INET;
	snprintf(servname, sizeof servname, "%d", portnr);

	return getaddrinfo(host, servname, &myaddr, ai);
}

struct addrinfo * select_resolved_host(struct addrinfo *ai, char use_ipv6)
{
	struct addrinfo *p = ai;
	while(p)
	{
		if (p -> ai_family == AF_INET6 && use_ipv6)
			return p;

		if (p -> ai_family == AF_INET)
			return p;

		p = p -> ai_next;
	}

	return NULL;
}

void get_addr(struct addrinfo *ai_use, struct sockaddr_in6 *addr)
{
	memcpy(addr, ai_use->ai_addr, ai_use->ai_addrlen);
}

#define incopy(a)       *((struct in_addr *)a)

int resolve_host_ipv4(const char *host, struct sockaddr_in *addr)
{
	struct hostent *hostdnsentries;

	hostdnsentries = gethostbyname(host);

	if (hostdnsentries == NULL)
	{
		switch(h_errno)
		{
			case HOST_NOT_FOUND:
				set_error("The specified host is unknown.");
				break;

			case NO_ADDRESS:
				set_error("The requested name is valid but does not have an IP address.");
				break;

			case NO_RECOVERY:
				set_error("A non-recoverable name server error occurred.");
				break;

			case TRY_AGAIN:
				set_error("A temporary error occurred on an authoritative name server. Try again later.");
				break;

			default:
				set_error("Could not resolve %s for an unknown reason (%d)", host, h_errno);
		}

		return -1;
	}

	/* create address structure */
	addr -> sin_family = hostdnsentries -> h_addrtype;
	addr -> sin_addr = incopy(hostdnsentries -> h_addr_list[0]);

	return 0;
}
