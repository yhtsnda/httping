/* Released under GPLv2 with exception for the OpenSSL library. See license.txt */

#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <openssl/bio.h>
#include <openssl/conf.h>
#include <openssl/engine.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/md5.h>
#include <openssl/ssl.h>
#include <openssl/x509.h>

#include "gen.h"
#include "mssl.h"

BIO *bio_err = NULL;

void shutdown_ssl(void)
{
	BIO_free(bio_err);

	ERR_free_strings();

	ERR_remove_state(0);
	ENGINE_cleanup();
	CONF_modules_free();
	EVP_cleanup();
	CRYPTO_cleanup_all_ex_data();
}

char close_ssl_connection(SSL *ssl_h, int socket_h)
{
	int rc = SSL_shutdown(ssl_h);

	if (!rc)
	{
		shutdown(socket_h, 1);

		rc = SSL_shutdown(ssl_h);
	}

	/* rc == 0 means try again but it seems to be fine
	 * to ignore that is what I read from the manpage
	 */
	if (rc == -1)
		return -1;
	else 
		return 0;
}

int READ_SSL(SSL *ssl_h, char *whereto, int len)
{
	int cnt=len;

	while(len>0)
	{
		int rc;

		rc = SSL_read(ssl_h, whereto, len);
		if (rc == -1)
		{
			if (errno != EINTR && errno != EAGAIN)
			{
				sprintf(last_error, "READ_SSL: io-error: %s", strerror(errno));
				return -1;
			}
		}
		else if (rc == 0)
		{
			return 0;
		}
		else
		{
			whereto += rc;
			len -= rc;
		}
	}

	return cnt;
}

int WRITE_SSL(SSL *ssl_h, const char *wherefrom, int len)
{
	int cnt=len;

	while(len>0)
	{
		int rc;

		rc = SSL_write(ssl_h, wherefrom, len);
		if (rc == -1)
		{
			if (errno != EINTR && errno != EAGAIN)
			{
				sprintf(last_error, "WRITE_SSL: io-error: %s", strerror(errno));
				return -1;
			}
		}
		else if (rc == 0)
		{
			return 0;
		}
		else
		{
			wherefrom += rc;
			len -= rc;
		}
	}

	return cnt;
}

int connect_ssl(int socket_h, SSL_CTX *client_ctx, SSL **ssl_h, BIO **s_bio, int timeout)
{
	int dummy = -1;

	struct timeval tv;
	tv.tv_sec  = timeout / 1000;
	tv.tv_usec = (timeout - (tv.tv_sec * 1000)) * 1000;

	if (setsockopt(socket_h, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof tv) == -1)
	{
		sprintf(last_error, "problem setting receive timeout (%s)", strerror(errno));
		return -1;
	}

	if (setsockopt(socket_h, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof tv) == -1)
	{
		sprintf(last_error, "problem setting transmit timeout (%s)", strerror(errno));
		return -1;
	}

	*ssl_h = SSL_new(client_ctx);

	*s_bio = BIO_new_socket(socket_h, BIO_NOCLOSE);
	SSL_set_bio(*ssl_h, *s_bio, *s_bio);

	dummy = SSL_connect(*ssl_h);
	if (dummy <= 0)
	{
		sprintf(last_error, "problem starting SSL connection: %d", SSL_get_error(*ssl_h, dummy));
		return -1;
	}

	return 0;
}

SSL_CTX * initialize_ctx(char ask_compression)
{
	if (!bio_err)
	{
		SSL_library_init();
		SSL_load_error_strings();
		ERR_load_crypto_strings();

		/* error write context */
		bio_err = BIO_new_fp(stderr, BIO_NOCLOSE);
	}

	/* create context */
	const SSL_METHOD *meth = SSLv23_method();

	SSL_CTX *ctx = SSL_CTX_new(meth);

#if ! defined(__FreeBSD__)
	if (!ask_compression)
		SSL_CTX_set_options(ctx, SSL_OP_NO_COMPRESSION);
#endif

	return ctx;
}

char * get_fingerprint(SSL *ssl_h)
{
	char *string = NULL;

	unsigned char fp_digest[EVP_MAX_MD_SIZE];
	X509 *x509_data = SSL_get_peer_certificate(ssl_h);

	if (x509_data)
	{
		unsigned int fp_digest_size = sizeof fp_digest;

		memset(fp_digest, 0x00, fp_digest_size);

		if (X509_digest(x509_data, EVP_md5(), fp_digest, &fp_digest_size))
		{
			string = (char *)malloc(MD5_DIGEST_LENGTH * 3 + 1);
			if (string)
			{
				int loop, pos =0;

				for(loop=0; loop<MD5_DIGEST_LENGTH; loop++)
				{
					if (loop)
						pos += sprintf(&string[pos], ":%02x", fp_digest[loop]);
					else
						pos = sprintf(&string[pos], "%02x", fp_digest[loop]);
				}
			}
		}

		X509_free(x509_data);
	}

	return string;
}
