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

int resolve_host(char *host, struct addrinfo **ai, char use_ipv6, int portnr)
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
			return ai;
	}

	return NULL;
}

void get_addr(struct addrinfo *ai_use, struct sockaddr_in6 *addr)
{
	memcpy(addr, ai_use->ai_addr, ai_use->ai_addrlen);
}
