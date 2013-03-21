/* Released under GPLv2 with exception for the OpenSSL library. See license.txt */

#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

void shutdown_ssl(void);
char close_ssl_connection(SSL *ssl_h, int socket_h);
int READ_SSL(SSL *ssl_h, char *whereto, int len);
int WRITE_SSL(SSL *ssl_h, const char *whereto, int len);
int connect_ssl(int socket_h, SSL_CTX *client_ctx, SSL **ssl_h, BIO **s_bio, int timeout);
SSL_CTX * initialize_ctx(char ask_compression);
char * get_fingerprint(SSL *ssl_h);
int connect_ssl_proxy(struct sockaddr *bind_to, struct addrinfo *ai_use_proxy, int timeout, const char *proxy_user, const char *proxy_password, const char *hostname, int portnr, char *tfo);
