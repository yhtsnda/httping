/* Released under GPLv2 with exception for the OpenSSL library. See license.txt */

#define incopy(a)       *((struct in_addr *)a)

int resolve_host(char *host, struct addrinfo **ai, char use_ipv6, int portnr);
struct addrinfo * select_resolved_host(struct addrinfo *ai, char use_ipv6);
void get_addr(struct addrinfo *ai_use, struct sockaddr_in6 *addr);
