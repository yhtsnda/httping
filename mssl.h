/* Released under GPLv2 with exception for the OpenSSL library. See license.txt */

void shutdown_ssl(void);
char close_ssl_connection(SSL *ssl_h, int socket_h);
int READ_SSL(SSL *ssl_h, char *whereto, int len);
int WRITE_SSL(SSL *ssl_h, const char *whereto, int len);
int connect_ssl(int socket_h, SSL_CTX *client_ctx, SSL **ssl_h, BIO **s_bio, int timeout);
SSL_CTX * initialize_ctx(void);
char * get_fingerprint(SSL *ssl_h);
