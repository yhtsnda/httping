/* Released under GPLv2 with exception for the OpenSSL library. See license.txt */

int get_HTTP_headers(int socket_h, SSL *ssl_h, char **headers, int *overflow, int timeout);
int dumb_get_HTTP_headers(int socket_h, char **headers, int timeout);
