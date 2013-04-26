/* Released under GPLv2 with exception for the OpenSSL library. See license.txt */
/* $Revision$ */

int connect_to(struct sockaddr *bind_to, struct addrinfo *ai, int timeout, char *tfo, char *msg, int msg_len, char *msg_accepted, int max_mtu);
int set_tcp_low_latency(int sock);
void failure_close(int fd);
