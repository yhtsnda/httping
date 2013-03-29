/* Released under GPLv2 with exception for the OpenSSL library. See license.txt */

#ifndef __GEN_H__
#define __GEN_H__

#define RC_OK		0
#define RC_SHORTREAD	-1
#define RC_SHORTWRITE	-1
#define RC_TIMEOUT	-2
#define RC_CTRLC	-3
#define RC_INVAL	-4

#ifdef NO_SSL
	#define SSL	void
	#define SSL_CTX	void
	#define BIO	void
#endif

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

#define min(x, y) ((x) < (y) ? (x) : (y))
#define max(x, y) ((x) > (y) ? (x) : (y))

typedef struct
{
	double cur, min, avg, max, sd;
	int n;
} stats_t;

int enc_b64(char *source, int source_lenght, char *target);

void init_statst(stats_t *data);
void update_statst(stats_t *data, double in);
double calc_sd(stats_t *in);

#endif
