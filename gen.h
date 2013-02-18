/* Released under GPLv2 with exception for the OpenSSL library. See license.txt */

#define RC_OK		0
#define RC_SHORTREAD	-1
#define RC_TIMEOUT	-2
#define RC_CTRLC	-3

#ifdef NO_SSL
	#define SSL	void
	#define SSL_CTX	void
	#define BIO	void
#endif

#define ERROR_BUFFER_SIZE	4096

#ifdef TCP_TFO
	#ifndef MSG_FASTOPEN
		#define MSG_FASTOPEN	0x20000000
	#endif

	#ifndef TCP_FASTOPEN
		#define TCP_FASTOPEN	23
	#endif
	#ifndef TCPI_OPT_SYN_DATA
		#define TCPI_OPT_SYN_DATA	32
	#endif
#endif
