/* Released under GPLv2 with exception for the OpenSSL library. See license.txt */

int connect_to(struct sockaddr *bind_to, struct addrinfo *ai, int timeout, char *tfo, char *msg, int msg_len, int *msg_accepted);
int set_tcp_low_latency(int sock);
