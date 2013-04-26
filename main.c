/* Released under GPLv2 with exception for the OpenSSL library. See license.txt */
/* $Revision$ */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <math.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#ifndef NO_SSL
#include <openssl/ssl.h>
#include "mssl.h"
#endif
#include <arpa/inet.h>
#include <time.h>
#include <sys/time.h>
#ifdef NC
#include <ncurses.h>
#endif

#include "gen.h"
#include "http.h"
#include "io.h"
#include "str.h"
#include "tcp.h"
#include "res.h"
#include "utils.h"
#include "error.h"
#include "socks5.h"
#ifdef NC
#include "nc.h"
#endif

volatile int stop = 0;

int quiet = 0;
char machine_readable = 0;
char json_output = 0;
char show_ts = 0;

const char *c_error = "";
const char *c_normal = "";
const char *c_very_normal = "";
const char *c_red = "";
const char *c_blue = "";
const char *c_green = "";
const char *c_yellow = "";
const char *c_magenta = "";
const char *c_cyan = "";
const char *c_white = "";
const char *c_bright = "";

char nagios_mode = 0;
char ncurses_mode = 0;

int fd = -1;

void version(void)
{
	fprintf(stderr, "HTTPing v" VERSION ", (C) 2003-2013 folkert@vanheusden.com\n");
#ifndef NO_SSL
	fprintf(stderr, " * SSL support included (-l)\n");
#endif

#ifdef NC
#ifdef FW
	fprintf(stderr, " * ncurses interface with FFT included (-K)\n");
#else
	fprintf(stderr, " * ncurses interface included (-K)\n");
#endif
#endif

#ifdef TCP_TFO
	fprintf(stderr, " * TFO (TCP fast open) support included (-F)\n");
#endif
	fprintf(stderr, "\n");
}

void help_long(void)
{
	fprintf(stderr, "--aggregate x[,y[,z]]  show an aggregate each x[/y[/z[/etc]]] seconds\n");
	fprintf(stderr, "--ai / --adaptive-interval  execute pings at multiples of interval relative to start, default on in ncurses output mode\n");
	fprintf(stderr, "--audible-ping         -a\n");
	fprintf(stderr, "--basic-auth           -A\n");
	fprintf(stderr, "--bind-to              -y\n");
	fprintf(stderr, "--colors               -Y\n");
	fprintf(stderr, "--cookie               -C\n");
	fprintf(stderr, "--count                -c\n");
	fprintf(stderr, "--data-limit           -L\n");
#if defined(NC) && defined(FW)
	fprintf(stderr, "--draw-phase           draw phase (fourier transform) in gui\n");
#endif
	fprintf(stderr, "--nagios-mode-2        -n\n");
	fprintf(stderr, "--flood                -f\n");
	fprintf(stderr, "--get-request          -G\n");
	fprintf(stderr, "--graph-limit x        do not scale to values above x\n");
	fprintf(stderr, "--hostname             -h\n");
	fprintf(stderr, "--host-port            -x\n");
	fprintf(stderr, "--interval             -i\n");
	fprintf(stderr, "--ipv6                 -6\n");
	fprintf(stderr, "--nagios-mode-1        -n\n");
	fprintf(stderr, "--nagios-mode-2        -n\n");
#ifdef NC
	fprintf(stderr, "--ncurses              -K\n");
#endif
	fprintf(stderr, "--no-cache             -Z\n");
	fprintf(stderr, "--no-graph             -D\n");
	fprintf(stderr, "--ok-result-codes      -o (only for -m)\n");
	fprintf(stderr, "--parseable-output     -m\n");
	fprintf(stderr, "--password             -P\n");
	fprintf(stderr, "--persistent-connections  -Q\n");
	fprintf(stderr, "--port                 -p\n");
	fprintf(stderr, "--proxy-buster x       adds \"&x=[random value]\" to the request URL\n");
	fprintf(stderr, "--proxy-user           \n");
	fprintf(stderr, "--proxy-password       \n");
	fprintf(stderr, "--proxy-password-file  \n");
	fprintf(stderr, "--quiet                -q\n");
	fprintf(stderr, "--referer              -R\n");
	fprintf(stderr, "--resolve-once         -r\n");
	fprintf(stderr, "--result-string        -e\n");
	fprintf(stderr, "--show-kb              -X\n");
	fprintf(stderr, "--show-statusodes      -s\n");
	fprintf(stderr, "--show-transfer-speed  -b\n");
	fprintf(stderr, "--show-xfer-speed-compressed  -B\n");
	fprintf(stderr, "--split-time           -S\n");
#ifdef TCP_TFO
	fprintf(stderr, "--tcp-fast-open        -F\n");
#endif
	fprintf(stderr, "--threshold-red        from what ping value to show the value in red (must be bigger than yellow)\n");
	fprintf(stderr, "--threshold-show       from what ping value to show the results\n");
	fprintf(stderr, "--threshold-yellow     from what ping value to show the value in yellow\n");
	fprintf(stderr, "--timeout              -t\n");
	fprintf(stderr, "--timestamp / --ts     put a timestamp before the measured values, use -v to include the date and -vv to show in microseconds\n");
	fprintf(stderr, "--url                  -g\n");
	fprintf(stderr, "--user-agent           -I\n");
	fprintf(stderr, "--username             -U\n");
#ifndef NO_SSL
	fprintf(stderr, "--use-ssl              -l\n");
	fprintf(stderr, "--show-fingerprint     -z\n");
#endif
	fprintf(stderr, "--version              -V\n");
	fprintf(stderr, "--help                 -H\n");
}

void usage(const char *me)
{
	char *dummy = NULL, has_color = 0;
	char host[256] = { 0 };

	fprintf(stderr, "\n-g url         url (e.g. -g http://localhost/)\n");
	fprintf(stderr, "-h hostname    hostname (e.g. localhost)\n");
	fprintf(stderr, "-p portnr      portnumber (e.g. 80)\n");
	fprintf(stderr, "-x host:port   hostname+portnumber of proxyserver\n");
	fprintf(stderr, "-5             proxy is a socks5 server\n");
	fprintf(stderr, "-c count       how many times to connect\n");
	fprintf(stderr, "-i interval    delay between each connect\n");
	fprintf(stderr, "-t timeout     timeout (default: 30s)\n");
	fprintf(stderr, "-Z             ask any proxies on the way not to cache the requests\n");
	fprintf(stderr, "-Q             use a persistent connection. adds a 'C' to the output if httping had to reconnect\n");
	fprintf(stderr, "-6             use IPv6\n");
	fprintf(stderr, "-s             show statuscodes\n");
	fprintf(stderr, "-S             split time in connect-time and processing time\n");
	fprintf(stderr, "-G             do a GET request instead of HEAD (read the\n");
	fprintf(stderr, "               contents of the page as well)\n");
	fprintf(stderr, "-b             show transfer speed in KB/s (use with -G)\n");
	fprintf(stderr, "-B             like -b but use compression if available\n");
	fprintf(stderr, "-L x           limit the amount of data transferred (for -b)\n");
	fprintf(stderr, "               to 'x' (in bytes)\n");
	fprintf(stderr, "-X             show the number of KB transferred (for -b)\n");
#ifndef NO_SSL
	fprintf(stderr, "-l             connect using SSL\n");
	fprintf(stderr, "-z             show fingerprint (SSL)\n");
#endif
	fprintf(stderr, "-f             flood connect (no delays)\n");
	fprintf(stderr, "-a             audible ping\n");
	fprintf(stderr, "-m             give machine parseable output (see\n");
	fprintf(stderr, "               also -o and -e)\n");
	fprintf(stderr, "-M             json output, cannot be combined with -m\n");
	fprintf(stderr, "-o rc,rc,...   what http results codes indicate 'ok'\n");
	fprintf(stderr, "               comma seperated WITHOUT spaces inbetween\n");
	fprintf(stderr, "               default is 200, use with -e\n");
	fprintf(stderr, "-e str         string to display when http result code\n");
	fprintf(stderr, "               doesn't match\n");
	fprintf(stderr, "-I str         use 'str' for the UserAgent header\n");
	fprintf(stderr, "-R str         use 'str' for the Referer header\n");
	fprintf(stderr, "-r             resolve hostname only once (usefull when\n");
	fprintf(stderr, "               pinging roundrobin DNS: also takes the first\n");
	fprintf(stderr, "               DNS lookup out of the loop so that the first\n");
	fprintf(stderr, "               measurement is also correct)\n");
	fprintf(stderr, "-W             do not abort the program if resolving failed: keep retrying\n");
	fprintf(stderr, "-n warn,crit   Nagios-mode: return 1 when avg. response time\n");
	fprintf(stderr, "               >= warn, 2 if >= crit, otherwhise return 0\n");
	fprintf(stderr, "-N x           Nagios mode 2: return 0 when all fine, 'x'\n");
	fprintf(stderr, "               when anything failes\n");
	fprintf(stderr, "-y ip[:port]   bind to ip-address (and thus interface) [/port]\n");
	fprintf(stderr, "-q             quiet, only returncode\n");
	fprintf(stderr, "-A             Activate Basic authentication\n");
	fprintf(stderr, "-U username    needed for authentication\n");
	fprintf(stderr, "-P password    needed for authentication\n");
	fprintf(stderr, "-T x           read the password fom the file 'x' (replacement for -P)\n");
	fprintf(stderr, "-C cookie=value Add a cookie to the request\n");
	fprintf(stderr, "-Y             add colors\n");
	fprintf(stderr, "-E             fetch proxy settings from environment variables\n");
	fprintf(stderr, "-K             ncurses mode\n");
#ifdef TCP_TFO
	fprintf(stderr, "-F             \"TCP fast open\" (TFO), reduces the latency of TCP connects\n");
#endif
	fprintf(stderr, "-v             verbose mode\n");
	fprintf(stderr, "-V             show the version\n\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "-J             list long options\n");
	fprintf(stderr, "               NOTE: not all functionality has a \"short\" switch, so not all are listed here! Please check -J too.\n");
	fprintf(stderr, "\n");

	dummy = getenv("TERM");
	if (dummy)
	{
		if (strstr(dummy, "ANSI") || strstr(dummy, "xterm") || strstr(dummy, "screen"))
			has_color = 1;
	}

	if (gethostname(host, sizeof host))
		strcpy(host, "localhost");

	fprintf(stderr, "Example:\n");
	fprintf(stderr, "\t%s %s%s -s -Z\n\n", me, host, has_color ? " -Y" : "");
}

void emit_statuslines(double run_time)
{
#ifdef NC
	if (ncurses_mode)
	{
		time_t t = time(NULL);
		char *t_str = ctime(&t);
		char *dummy = strchr(t_str, '\n');

		if (dummy)
			*dummy = 0x00;

		status_line("%s, run time: %.3fs, press ctrl + c to stop", t_str, run_time);
	}
#else
	(void)run_time;
#endif
}

void emit_headers(char *in)
{
#ifdef NC
	static char shown = 0;
	int len_in = -1;

	if (!shown && ncurses_mode && in != NULL && (len_in = strlen(in) - 4) > 0)
	{
		int pos = 0, pos_out = 0;
		char *copy = (char *)malloc(len_in + 1), *dummy = NULL;

		for(pos=0; pos<len_in; pos++)
		{
			if (in[pos] != '\r')
				copy[pos_out++] = in[pos];
		}

		copy[pos_out] = 0x00;

		/* in case more than the headers were sent */
		dummy = strstr(copy, "\n\n");
		if (dummy)
			*dummy = 0x00;

		slow_log("\n%s", copy);

		free(copy);

		shown = 1;
	}
#else
	(void)in;
#endif
}

void emit_json(char ok, int seq, double start_ts, stats_t *t_resolve, stats_t *t_connect, stats_t *t_request, int http_code, const char *msg, int header_size, int data_size, double Bps, const char *host, const char *ssl_fp, double toff_diff_ts, char tfo_succes)
{
	if (seq > 1)
		printf(", \n");
	printf("{ ");
	printf("\"status\" : \"%d\", ", ok);
	printf("\"seq\" : \"%d\", ", seq);
	printf("\"start_ts\" : \"%f\", ", start_ts);
	printf("\"resolve_ms\" : \"%e\", ", t_resolve -> cur);
	printf("\"connect_ms\" : \"%e\", ", t_connect -> cur);
	printf("\"request_ms\" : \"%e\", ", t_request -> cur);
	printf("\"total_ms\" : \"%e\", ", t_resolve -> cur + t_connect -> cur + t_request -> cur);
	printf("\"http_code\" : \"%d\", ", http_code);
	printf("\"msg\" : \"%s\", ", msg);
	printf("\"header_size\" : \"%d\", ", header_size);
	printf("\"data_size\" : \"%d\", ", data_size);
	printf("\"bps\" : \"%f\", ", Bps);
	printf("\"host\" : \"%s\", ", host);
	printf("\"ssl_fingerprint\" : \"%s\", ", ssl_fp ? ssl_fp : "");
	printf("\"time_offset\" : \"%f\", ", toff_diff_ts);
	printf("\"tfo_succes\" : \"%s\" ", tfo_succes ? "true" : "false");
	printf("}");
}

char *get_ts_str(int verbose)
{
	struct timeval tv;

	(void)gettimeofday(&tv, NULL);

	struct tm *tvm = localtime(&tv.tv_sec);

	char buffer[4096] = { 0 };

	if (verbose == 1)
		sprintf(buffer, "%04d/%02d/%02d ", tvm -> tm_year + 1900, tvm -> tm_mon + 1, tvm -> tm_mday);
	else if (verbose >= 2)
		sprintf(buffer, "%.6f", get_ts());

	if (verbose <= 1)
		sprintf(&buffer[strlen(buffer)], "%02d:%02d:%02d.%03d", tvm -> tm_hour, tvm -> tm_min, tvm -> tm_sec, (int)(tv.tv_usec / 1000));

	return strdup(buffer);
}

void emit_error(int verbose, int seq, double start_ts)
{
	char *ts = show_ts ? get_ts_str(verbose) : NULL;

#ifdef NC
	if (ncurses_mode)
	{
		slow_log("\n%s%s", ts ? ts : "", get_error());
		update_terminal();
	}
	else
#endif
	if (!quiet && !machine_readable && !nagios_mode && !json_output)
		printf("%s%s%s%s\n", ts ? ts : "", c_error, get_error(), c_normal);

	if (json_output)
		emit_json(0, seq, start_ts, NULL, NULL, NULL, -1, get_error(), -1, -1, -1, "", "", -1, 0);

	clear_error();

	free(ts);

	fflush(NULL);
}

void handler(int sig)
{
#ifdef NC
	if (sig == SIGWINCH)
		win_resize = 1;
	else
#endif
	{
		if (!json_output)
			fprintf(stderr, "Got signal %d\n", sig);

		stop = 1;
	}
}

char * read_file(const char *file)
{
	char buffer[4096] = { 0 }, *lf = NULL;
	FILE *fh = fopen(file, "rb");
	if (!fh)
		error_exit("Cannot open password-file %s", file);

	if (!fgets(buffer, sizeof buffer, fh))
		error_exit("Problem reading password from file %s", file);

	fclose(fh);

	lf = strchr(buffer, '\n');
	if (lf)
		*lf = 0x00;

	return strdup(buffer);
}

char * create_request_header(const char *get, char use_proxy_host, char get_instead_of_head, char persistent_connections, const char *hostname, const char *useragent, const char *referer, char ask_compression, char no_cache, const char *auth_usr, const char *auth_password, const char *cookie, const char *proxy_buster, const char *proxy_user, const char *proxy_password)
{
	char *request = (char *)malloc(strlen(get) + 8192);
	char pb[128] = { 0 };

	if (proxy_buster)
	{
		if (strchr(get, '?'))
			pb[0] = '&';
		else
			pb[0] = '?';

		snprintf(pb + 1, sizeof pb - 1, "%s=%ld", proxy_buster, lrand48());
	}

	if (use_proxy_host)
		sprintf(request, "%s %s%s HTTP/1.%c\r\n", get_instead_of_head?"GET":"HEAD", get, pb, persistent_connections?'1':'0');
	else
	{
		const char *dummy = get, *slash = NULL;
		if (strncasecmp(dummy, "http://", 7) == 0)
			dummy += 7;
		else if (strncasecmp(dummy, "https://", 7) == 0)
			dummy += 8;

		slash = strchr(dummy, '/');
		if (slash)
			sprintf(request, "%s %s HTTP/1.%c\r\n", get_instead_of_head?"GET":"HEAD", slash, persistent_connections?'1':'0');
		else
			sprintf(request, "%s / HTTP/1.%c\r\n", get_instead_of_head?"GET":"HEAD", persistent_connections?'1':'0');
	}

	if (hostname)
		sprintf(&request[strlen(request)], "Host: %s\r\n", hostname);

	if (useragent)
		sprintf(&request[strlen(request)], "User-Agent: %s\r\n", useragent);
	else
		sprintf(&request[strlen(request)], "User-Agent: HTTPing v" VERSION "\r\n");

	if (referer)
		sprintf(&request[strlen(request)], "Referer: %s\r\n", referer);

	if (ask_compression)
		sprintf(&request[strlen(request)], "Accept-Encoding: gzip,deflate\r\n");

	if (no_cache)
	{
		sprintf(&request[strlen(request)], "Pragma: no-cache\r\n");
		sprintf(&request[strlen(request)], "Cache-Control: no-cache\r\n");
	}

	/* Basic Authentification */
	if (auth_usr)
	{ 
		char auth_string[256] = { 0 };
		char b64_auth_string[512] = { 0 };

		sprintf(auth_string, "%s:%s", auth_usr, auth_password); 
		enc_b64(auth_string, strlen(auth_string), b64_auth_string);
		sprintf(&request[strlen(request)], "Authorization: Basic %s\r\n", b64_auth_string);
	}

	/* proxy authentication */
	if (proxy_user)
	{ 
		char ppa_string[256] = { 0 };
		char b64_ppa_string[512] = { 0 };

		sprintf(ppa_string, "%s:%s", proxy_user, proxy_password); 
		enc_b64(ppa_string, strlen(ppa_string), b64_ppa_string);
		sprintf(&request[strlen(request)], "Proxy-Authorization: Basic %s\r\n", b64_ppa_string);
	}

	/* Cookie Insertion */
	if (cookie)
		sprintf(&request[strlen(request)], "Cookie: %s;\r\n", cookie);

	if (persistent_connections)
		sprintf(&request[strlen(request)], "Connection: keep-alive\r\n");

	strcat(request, "\r\n");

	return request;
}

void interpret_url(const char *in, char **path, char **hostname, int *portnr, char use_ipv6, char use_ssl, char **complete_url, char **auth_user, char **auth_password)
{
	char in_use[65536] = { 0 }, *dummy = NULL;

	if (strlen(in) >= sizeof in_use)
		error_exit("Url too big, HTTPing has a %d bytes limit", sizeof in_use - 1);

	/* make url complete, if not already */
	if (strncasecmp(in, "http://", 7) == 0 || strncasecmp(in, "https://", 8) == 0) /* complete url? */
	{
		snprintf(in_use, sizeof in_use - 1, "%s", in);

		if (strchr(&in[8], '/') == NULL)
			in_use[strlen(in_use)] = '/';
	}
	else if (strchr(in, '/')) /* hostname + location without 'http://'? */
		sprintf(in_use, "http://%s", in);
	else if (use_ssl)
		sprintf(in_use, "https://%s/", in);
	else
		sprintf(in_use, "http://%s/", in);

	/* sanity check */
	if (strncasecmp(in_use, "http://", 7) == 0 && use_ssl)
		error_exit("using \"http://\" with SSL enabled (-l)");

	*complete_url = strdup(in_use);

	/* fetch hostname */
	if (strncasecmp(in_use, "http://", 7) == 0)
		*hostname = strdup(&in_use[7]);
	else /* https */
		*hostname = strdup(&in_use[8]);

	dummy = strchr(*hostname, '/');
	if (dummy)
		*dummy = 0x00;

	/* fetch port number */
	if (use_ssl || strncasecmp(in, "https://", 8) == 0)
		*portnr = 443;
	else
		*portnr = 80;

	if (!use_ipv6)
	{
		char *at = strchr(*hostname, '@');
		char *colon = strchr(*hostname, ':');
		char *colon2 = colon ? strchr(colon + 1, ':') : NULL;

		if (colon2)
		{
			*colon2 = 0x00;
			*portnr = atoi(colon2 + 1);

			if (at)
			{
				*colon = 0x00;
				*at = 0x00;

				*auth_user = strdup(*hostname);
				*auth_password = strdup(colon + 1);
			}
		}
		else if (colon)
		{
			if (colon < at)
			{
				*colon = 0x00;
				*at = 0x00;

				*auth_user = strdup(*hostname);
				*auth_password = strdup(colon + 1);
			}
			else if (at)
			{
				*at = 0x00;
				*auth_user = strdup(*hostname);
			}
			else
			{
				*colon = 0x00;
				*portnr = atoi(colon + 1);
			}
		}
	}

	/* fetch path */
	dummy = strchr(&in_use[8], '/');
	if (dummy)
		*path = strdup(dummy);
	else
		*path = strdup("/");
}

typedef struct {
	int interval, last_ts;
	double value, sd, min, max;
	int n_values;
} aggregate_t;

void set_aggregate(char *in, int *n_aggregates, aggregate_t **aggregates)
{
	char *dummy = in;

	*n_aggregates = 0;

	for(;dummy;)
	{
		(*n_aggregates)++;

		*aggregates = (aggregate_t *)realloc(*aggregates, *n_aggregates * sizeof(aggregate_t));

		memset(&(*aggregates)[*n_aggregates - 1], 0x00, sizeof(aggregate_t));

		(*aggregates)[*n_aggregates - 1].interval = atoi(dummy);
		(*aggregates)[*n_aggregates - 1].max = -MY_DOUBLE_INF;
		(*aggregates)[*n_aggregates - 1].min =  MY_DOUBLE_INF;

		dummy = strchr(dummy, ',');
		if (dummy)
			dummy++;
	}
}

void do_aggregates(double cur_ms, int cur_ts, int n_aggregates, aggregate_t *aggregates, int verbose, char show_ts)
{
	int index=0;

	/* update measurements */
	for(index=0; index<n_aggregates; index++)
	{
		aggregates[index].value += cur_ms;

		if (cur_ms < aggregates[index].min)
			aggregates[index].min = cur_ms;

		if (cur_ms > aggregates[index].max)
			aggregates[index].max = cur_ms;

		aggregates[index].sd += cur_ms * cur_ms;

		aggregates[index].n_values++;
	}

	/* emit */
	for(index=0; index<n_aggregates && cur_ts > 0; index++)
	{
		aggregate_t *a = &aggregates[index];

		if (cur_ts - a -> last_ts >= a -> interval)
		{
			char line[4096] = { 0 };
			int pos = 0;
			double avg = a -> n_values ? a -> value / (double)a -> n_values : -1.0;
			char *ts = get_ts_str(verbose);

			pos += snprintf(&line[pos], sizeof line - pos, "%s", show_ts ? ts : "");
			pos += snprintf(&line[pos], sizeof line - pos, "AGG[%d]: %d values, min/avg/max%s = %.1f/%.1f/%.1f", a -> interval, a -> n_values, verbose ? "/sd" : "", a -> min, avg, a -> max);

			free(ts);

			if (verbose)
			{
				double sd = -1.0;

				if (a -> n_values)
					sd = sqrt((a -> sd / (double)a -> n_values) - pow(avg, 2.0));

				pos += snprintf(&line[pos], sizeof line - pos, "/%.1f", sd);
			}

			pos += snprintf(&line[pos], sizeof line - pos, " ms");

#ifdef NC
			if (ncurses_mode)
				slow_log("\n%s", line);
			else
#endif
			printf("%s\n", line);

			aggregates[index].value =
			aggregates[index].sd    = 0.0;
			aggregates[index].min =  MY_DOUBLE_INF;
			aggregates[index].max = -MY_DOUBLE_INF;
			aggregates[index].n_values = 0;
			aggregates[index].last_ts = cur_ts;
		}
	}
}

void fetch_proxy_settings(char **proxy_user, char **proxy_password, char **proxy_host, int *proxy_port, char use_ssl, char use_ipv6)
{
	char *str = getenv(use_ssl ? "https_proxy" : "http_proxy");

	if (!str)
	{
		/* FIXME from wgetrc/curlrc? */
	}

	if (str)
	{
		char *path = NULL, *url = NULL;

		interpret_url(str, &path, proxy_host, proxy_port, use_ipv6, use_ssl, &url, proxy_user, proxy_password);

		free(url);
		free(path);
	}
}

void parse_nagios_settings(const char *in, double *nagios_warn, double *nagios_crit)
{
	char *dummy = strchr(in, ',');
	if (!dummy)
		error_exit("-n: missing parameter\n");

	*nagios_warn = atof(in);

	*nagios_crit = atof(dummy + 1);
}

void parse_bind_to(const char *in, struct sockaddr_in *bind_to_4, struct sockaddr_in6 *bind_to_6, struct sockaddr_in **bind_to)
{
	char *dummy = strchr(in, ':');

	if (dummy)
	{
		*bind_to = (struct sockaddr_in *)bind_to_6;
		memset(bind_to_6, 0x00, sizeof *bind_to_6);
		bind_to_6 -> sin6_family = AF_INET6;

		if (inet_pton(AF_INET6, in, &bind_to_6 -> sin6_addr) != 1)
			error_exit("cannot convert ip address '%s' (for -y)\n", in);
	}
	else
	{
		*bind_to = (struct sockaddr_in *)bind_to_4;
		memset(bind_to_4, 0x00, sizeof *bind_to_4);
		bind_to_4 -> sin_family = AF_INET;

		if (inet_pton(AF_INET, in, &bind_to_4 -> sin_addr) != 1)
			error_exit("cannot convert ip address '%s' (for -y)\n", in);
	}
}

void set_colors(void)
{
	c_red = "\033[31;40m";
	c_blue = "\033[34;40m";
	c_green = "\033[32;40m";
	c_yellow = "\033[33;40m";
	c_magenta = "\033[35;40m";
	c_cyan = "\033[36;40m";
	c_white = "\033[37;40m";

	c_bright = "\033[1;40m";
	c_normal = "\033[0;37;40m";

	c_very_normal = "\033[0m";

	c_error = "\033[1;4;40m";
}

time_t parse_date_from_response_headers(const char *in)
{
	char *date = NULL, *komma = NULL;
	if (in == NULL)
		return -1;

	date = strstr(in, "\nDate:");
	komma = date ? strchr(date, ',') : NULL;
	if (date && komma)
	{
		struct tm tm;
		memset(&tm, 0x00, sizeof tm);

		/* 22 Feb 2013 09:13:56 */
		if (strptime(komma + 1, "%d %b %Y %H:%M:%S %Z", &tm))
			return mktime(&tm);
	}

	return -1;
}

int calc_page_age(const char *in, const time_t their_ts)
{
	int age = -1;

	if (in != NULL && their_ts > 0)
	{
		char *date = strstr(in, "\nLast-Modified:");
		char *komma = date ? strchr(date, ',') : NULL;
		if (date && komma)
		{
			struct tm tm;
			memset(&tm, 0x00, sizeof tm);

			/* 22 Feb 2013 09:13:56 */
			if (strptime(komma + 1, "%d %b %Y %H:%M:%S %Z", &tm))
				age = their_ts - mktime(&tm);
		}
	}

	return age;
}

const char *get_location(const char *host, int port, char use_ssl, char *reply)
{
	if (reply)
	{
		char *copy = strdup(reply);
		char *head = strstr(copy, "\nLocation:");
		char *lf = head ? strchr(head + 1, '\n') : NULL;

		if (head)
		{
			char buffer[4096];
			char *dest = head + 11;

			if (lf)
				*lf = 0x00;

			if (memcmp(dest, "http", 4) == 0)
				snprintf(buffer, sizeof buffer, "%s", dest);
			else
				snprintf(buffer, sizeof buffer, "http%s://%s:%d%s", use_ssl ? "s" : "", host, port, dest);

			free(copy);

			return strdup(buffer);
		}

		free(copy);
	}

	return NULL;
}

char check_compressed(const char *reply)
{
	if (reply != NULL)
	{
		char *encoding = strstr(reply, "\nContent-Encoding:");

		if (encoding)
		{
			char *dummy = strchr(encoding + 1, '\r');
			if (dummy) *dummy = 0x00;

			dummy = strchr(encoding + 1, '\n');
			if (dummy) *dummy = 0x00;

			if (strstr(encoding, "gzip") == 0 || strstr(encoding, "deflate") == 0)
				return 1;
		}
	}

	return 0;
}

int nagios_result(int ok, int nagios_mode, int nagios_exit_code, double avg_httping_time, double nagios_warn, double nagios_crit)
{
	if (nagios_mode == 1)
	{
		if (ok == 0)
		{
			printf("CRITICAL - connecting failed: %s", get_error());
			return 2;
		}
		else if (avg_httping_time >= nagios_crit)
		{
			printf("CRITICAL - average httping-time is %.1f\n", avg_httping_time);
			return 2;
		}
		else if (avg_httping_time >= nagios_warn)
		{
			printf("WARNING - average httping-time is %.1f\n", avg_httping_time);
			return 1;
		}

		printf("OK - average httping-time is %.1f (%s)|ping=%f\n", avg_httping_time, get_error(), avg_httping_time);

		return 0;
	}
	else if (nagios_mode == 2)
	{
		const char *err = get_error();

		if (ok && err[0] == 0x00)
		{
			printf("OK - all fine, avg httping time is %.1f|ping=%f\n", avg_httping_time, avg_httping_time);
			return 0;
		}

		printf("%s: - failed: %s", nagios_exit_code == 1?"WARNING":(nagios_exit_code == 2?"CRITICAL":"ERROR"), err);
		return nagios_exit_code;
	}

	return -1;
}

void proxy_to_host_and_port(char *in, char **proxy_host, int *proxy_port)
{
	char *dummy = strchr(in, ':');

	*proxy_host = in;

	if (dummy)
	{
		*dummy=0x00;
		*proxy_port = atoi(dummy + 1);
	}
}

void stats_close(int *fd, stats_t *t_close, char is_failure)
{
	double t_start = get_ts(), t_end = -1;;

	if (is_failure)
		failure_close(*fd);
	else
		close(*fd);

	*fd = -1;

	t_end = get_ts();

	update_statst(t_close, (t_end - t_start) * 1000.0);
}

int main(int argc, char *argv[])
{
	char do_fetch_proxy_settings = 0;
	char *hostname = NULL;
	char *proxy_host = NULL, *proxy_user = NULL, *proxy_password = NULL;
	int proxy_port = 8080;
	int portnr = 80;
	char *get = NULL, *request = NULL;
	int req_len = 0;
	int c = 0;
	int count = -1, curncount = 0;
	double wait = 1.0;
	char wait_set = 0;
	int audible = 0;
	int ok = 0, err = 0;
	int timeout=30;
	char show_statuscodes = 0;
	char use_ssl = 0;
	const char *ok_str = "200";
	const char *err_str = "-1";
	const char *useragent = NULL;
	const char *referer = NULL;
	const char *auth_password = NULL;
	const char *auth_usr = NULL;
	const char *cookie = NULL;
	char resolve_once = 0;
	char have_resolved = 0;
	double nagios_warn=0.0, nagios_crit=0.0;
	int nagios_exit_code = 2;
	double avg_httping_time = -1.0;
	int get_instead_of_head = 0;
	char *buffer = NULL;
	int buffer_size = 131072;
	char show_Bps = 0, ask_compression = 0;
	double Bps_min = 1 << 30, Bps_max = -Bps_min;
	long long int Bps_avg = 0;
	int Bps_limit = -1;
	char show_bytes_xfer = 0, show_fp = 0;
	SSL *ssl_h = NULL;
	BIO *s_bio = NULL;
	struct sockaddr_in *bind_to = NULL;
	struct sockaddr_in bind_to_4;
	struct sockaddr_in6 bind_to_6;
	char split = 0, use_ipv6 = 0;
	char persistent_connections = 0, persistent_did_reconnect = 0;
	char no_cache = 0;
	char use_tfo = 0;
	char abort_on_resolve_failure = 1;
	double offset_yellow = -1, offset_red = -1;
	char colors = 0;
	int verbose = 0;
	double offset_show = -1.0;
	char add_host_header = 1;
	char *proxy_buster = NULL;
	char proxy_is_socks5 = 0;
	char *url = NULL, *complete_url = NULL;
	int n_aggregates = 0;
	aggregate_t *aggregates = NULL;
	char *au_dummy = NULL, *ap_dummy = NULL;
	stats_t t_connect, t_request, t_total, t_resolve, t_ssl, t_close, stats_to, tcp_rtt_stats;
	double total_took = 0;
	char first_resolve = 1;
	double graph_limit = MY_DOUBLE_INF;
	char nc_graph = 1;
	char adaptive_interval = 0;
	double show_slow_log = MY_DOUBLE_INF;
	char use_tcp_nodelay = 1;
	int max_mtu = -1;

	init_statst(&t_resolve);
	init_statst(&t_connect);
	init_statst(&t_request);
	init_statst(&t_total);
	init_statst(&t_ssl);
	init_statst(&t_close);

	init_statst(&stats_to);
#if defined(linux) || defined(__FreeBSD__)
	init_statst(&tcp_rtt_stats);
#endif

	static struct option long_options[] =
	{
		{"aggregate",   1, NULL, 9 },
		{"url",		1, NULL, 'g' },
		{"hostname",	1, NULL, 'h' },
		{"port",	1, NULL, 'p' },
		{"host-port",	1, NULL, 'x' },
		{"count",	1, NULL, 'c' },
		{"persistent-connections",	0, NULL, 'Q' },
		{"interval",	1, NULL, 'i' },
		{"timeout",	1, NULL, 't' },
		{"ipv6",	0, NULL, '6' },
		{"show-statusodes",	0, NULL, 's' },
		{"split-time",	0, NULL, 'S' },
		{"get-request",	0, NULL, 'G' },
		{"show-transfer-speed",	0, NULL, 'b' },
		{"show-xfer-speed-compressed",	0, NULL, 'B' },
		{"data-limit",	1, NULL, 'L' },
		{"show-kb",	0, NULL, 'X' },
		{"no-cache",	0, NULL, 'Z' },
#ifndef NO_SSL
		{"use-ssl",	0, NULL, 'l' },
		{"show-fingerprint",	0, NULL, 'z' },
#endif
		{"flood",	0, NULL, 'f' },
		{"audible-ping",	0, NULL, 'a' },
		{"parseable-output",	0, NULL, 'm' },
		{"ok-result-codes",	1, NULL, 'o' },
		{"result-string",	1, NULL, 'e' },
		{"user-agent",	1, NULL, 'I' },
		{"referer",	1, NULL, 'S' },
		{"resolve-once",0, NULL, 'r' },
		{"nagios-mode-1",	1, NULL, 'n' },
		{"nagios-mode-2",	1, NULL, 'n' },
		{"bind-to",	1, NULL, 'y' },
		{"quiet",	0, NULL, 'q' },
		{"username",	1, NULL, 'U' },
		{"password",	1, NULL, 'P' },
		{"cookie",	1, NULL, 'C' },
		{"colors",	0, NULL, 'Y' },
		{"offset-yellow",	1, NULL, 1   },
		{"threshold-yellow",	1, NULL, 1   },
		{"offset-red",	1, NULL, 2   },
		{"threshold-red",	1, NULL, 2   },
		{"offset-show",	1, NULL, 3   },
		{"show-offset",	1, NULL, 3   },
		{"show-threshold",	1, NULL, 3   },
		{"timestamp",	0, NULL, 4   },
		{"ts",		0, NULL, 4   },
		{"no-host-header",	0, NULL, 5 },
		{"proxy-buster",	1, NULL, 6 },
		{"proxy-user",	1, NULL, 7 },
		{"proxy-password",	1, NULL, 8 },
		{"proxy-password-file",	1, NULL, 10 },
		{"graph-limit",	1, NULL, 11 },
		{"adaptive-interval",	0, NULL, 12 },
		{"ai",	0, NULL, 12 },
		{"slow-log",	0, NULL, 13 },
		{"draw-phase",	0, NULL, 14 },
		{"no-tcp-nodelay",	0, NULL, 15 },
		{"max-mtu", 1, NULL, 16 },
#ifdef NC
		{"ncurses",	0, NULL, 'K' },
#ifdef FW
		{"no-graph",	0, NULL, 'D' },
#endif
#endif
		{"version",	0, NULL, 'V' },
		{"help",	0, NULL, 'H' },
		{NULL,		0, NULL, 0   }
	};

	signal(SIGPIPE, SIG_IGN);

	buffer = (char *)malloc(buffer_size);

	while((c = getopt_long(argc, argv, "DKEA5MvYWT:JZQ6Sy:XL:bBg:h:p:c:i:Gx:t:o:e:falqsmV?I:R:rn:N:zP:U:C:F", long_options, NULL)) != -1)
	{
		switch(c)
		{
			case 16:
				max_mtu = atoi(optarg);
				break;

			case 15:
				use_tcp_nodelay = 0;
				break;

			case 14:
				draw_phase = 1;
				break;

			case 13:
				show_slow_log = atof(optarg);
				break;

			case 12:
				adaptive_interval = 1;
				break;

			case 11:
				graph_limit = atof(optarg);
				break;

#ifdef NC
			case 'K':
				ncurses_mode = 1;
				adaptive_interval = 1;
				if (!wait_set)
					wait = 0.5;
				break;
#ifdef FW
			case 'D':
				nc_graph = 0;
				break;
#endif
#endif

			case 'E':
				do_fetch_proxy_settings = 1;
				break;

			case 'A':
				fprintf(stderr, "\n *** -A is no longer required ***\n\n");
				break;

			case 'M':
				json_output = 1;
				break;

			case 'v':
				verbose++;
				break;

			case 1:
				offset_yellow = atof(optarg);
				break;

			case 2:
				offset_red = atof(optarg);
				break;

			case 3:
				offset_show = atof(optarg);
				break;

			case 4:
				show_ts = 1;
				break;

			case 5:
				add_host_header = 0;
				break;

			case 6:
				proxy_buster = optarg;
				break;

			case '5':
				proxy_is_socks5 = 1;
				break;

			case 7:
				proxy_user = optarg;
				break;

			case 8:
				proxy_password = optarg;
				break;

			case 9:
				set_aggregate(optarg, &n_aggregates, &aggregates);
				break;

			case 10:
				proxy_password = read_file(optarg);
				break;

			case 'Y':
				colors = 1;
				break;

			case 'W':
				abort_on_resolve_failure = 0;
				break;

			case 'T':
				auth_password = read_file(optarg);
				break;

			case 'J':
				help_long();
				return 0;

			case 'Z':
				no_cache = 1;
				break;

			case '6':
				use_ipv6 = 1;
				break;

			case 'S':
				split = 1;
				break;

			case 'Q':
				persistent_connections = 1;
				break;

			case 'y':
				parse_bind_to(optarg, &bind_to_4, &bind_to_6, &bind_to);
				break;

			case 'z':
				show_fp = 1;
				break;

			case 'X':
				show_bytes_xfer = 1;
				break;

			case 'L':
				Bps_limit = atoi(optarg);
				break;

			case 'B':
				show_Bps = 1;
				ask_compression = 1;
				break;

			case 'b':
				show_Bps = 1;
				break;

			case 'e':
				err_str = optarg;
				break;

			case 'o':
				ok_str = optarg;
				break;

			case 'x':
				proxy_to_host_and_port(optarg, &proxy_host, &proxy_port);
				break;

			case 'g':
				url = optarg;
				break;

			case 'r':
				resolve_once = 1;
				break;

			case 'h':
				{
					char dummy_buffer[4096] = { 0 };
					snprintf(dummy_buffer, sizeof dummy_buffer, "http://%s/", optarg);
					url = strdup(dummy_buffer);
				}
				break;

			case 'p':
				portnr = atoi(optarg);
				break;

			case 'c':
				count = atoi(optarg);
				break;

			case 'i':
				wait = atof(optarg);
				if (wait < 0.0)
					error_exit("-i cannot have a value smaller than zero");
				wait_set = 1;
				break;

			case 't':
				timeout = atoi(optarg);
				break;

			case 'I':
				useragent = optarg;
				break;

			case 'R':
				referer = optarg;
				break;

			case 'a':
				audible = 1;
				break;

			case 'f':
				wait = 0;
				wait_set = 1;
				adaptive_interval = 0;
				break;

			case 'G':
				get_instead_of_head = 1;
				break;

#ifndef NO_SSL
			case 'l':
				use_ssl = 1;
				break;
#endif

			case 'm':
				machine_readable = 1;
				break;

			case 'q':
				quiet = 1;
				break;

			case 's':
				show_statuscodes = 1;
				break;

			case 'V':
				version();
				return 0;

			case 'n':
				if (nagios_mode)
					error_exit("-n and -N are mutual exclusive\n");
				else
					nagios_mode = 1;

				parse_nagios_settings(optarg, &nagios_warn, &nagios_crit);
				break;

			case 'N':
				if (nagios_mode) error_exit("-n and -N are mutual exclusive\n");
				nagios_mode = 2;
				nagios_exit_code = atoi(optarg);
				break;

			case 'P':
				auth_password = optarg;
				break;

			case 'U':
				auth_usr = optarg;
				break;

			case 'C':
				cookie = optarg;
				break;

			case 'F':
#ifdef TCP_TFO
				use_tfo = 1;
#else
				fprintf(stderr, "Warning: TCP TFO is not supported. Disabling.\n");
#endif
				break;
 
			case 'H':
				version();

				usage(argv[0]);

				return 0;

			case '?':
			default:
				fprintf(stderr, "\n");
				version();

				fprintf(stderr, "\n\nPlease run:\n\t%s --help\nto see a list of options.\n\n", argv[0]);

				return 1;
		}
	}

	if (max_mtu >= 0 && (proxy_host || use_ssl))
		error_exit("Cannot combine maximum MTU size setting with proxy connections or SSL");

	if (do_fetch_proxy_settings)
		fetch_proxy_settings(&proxy_user, &proxy_password, &proxy_host, &proxy_port, use_ssl, use_ipv6);

	if (optind < argc)
		url = argv[optind];

	if (!url)
	{
		fprintf(stderr, "No URL/host to ping given\n\n");
		return 1;
	}

	if (machine_readable + json_output + ncurses_mode > 1)
		error_exit("Cannot combine -m, -M and -K");

	if ((machine_readable || json_output) && n_aggregates > 0)
		error_exit("Aggregates can only be used in non-machine/json-output mode");

	clear_error();

	if (!(get_instead_of_head || use_ssl) && show_Bps)
		error_exit("-b/-B can only be used when also using -G (GET instead of HEAD) or -l (use SSL)\n");

	if (use_tfo && use_ssl)
		error_exit("TCP Fast open and SSL not supported together\n");

	if (colors)
		set_colors();

	if (!machine_readable && !json_output)
		printf("%s%s", c_normal, c_white);

	interpret_url(url, &get, &hostname, &portnr, use_ipv6, use_ssl, &complete_url, &au_dummy, &ap_dummy);
	if (!auth_usr)
		auth_usr = au_dummy;
	if (!auth_password)
		auth_password = ap_dummy;

#ifdef NC
	if (ncurses_mode)
	{
		if (wait == 0.0)
			wait = 0.001;

		init_ncurses_ui(graph_limit, 1.0 / wait);
	}
#endif

	if (strncmp(complete_url, "https://", 8) == 0 && !use_ssl)
	{
		use_ssl = 1;
#ifdef NC
		if (ncurses_mode)
		{
			slow_log("\nAuto enabling SSL due to https-URL");
			update_terminal();
		}
		else
#endif
		{
			fprintf(stderr, "Auto enabling SSL due to https-URL");
		}
	}

	if (verbose)
	{
#ifdef NC
		if (ncurses_mode)
		{
			slow_log("\nConnecting to host %s, port %d and requesting file %s", hostname, portnr, get);

			if (proxy_host)
				slow_log("\nUsing proxyserver: %s:%d", proxy_host, proxy_port);
		}
		else
#endif
		{
			printf("Connecting to host %s, port %d and requesting file %s\n\n", hostname, portnr, get);

			if (proxy_host)
				fprintf(stderr, "Using proxyserver: %s:%d\n", proxy_host, proxy_port);
		}
	}

#ifndef NO_SSL
	SSL_CTX *client_ctx = NULL;
	if (use_ssl)
	{
		client_ctx = initialize_ctx(ask_compression);
		if (!client_ctx)
		{
			set_error("problem creating SSL context");
			goto error_exit;
		}
	}
#endif

	if (!quiet && !machine_readable && !nagios_mode && !json_output)
	{
#ifdef NC
		if (ncurses_mode)
			slow_log("\nPING %s:%d (%s):", hostname, portnr, get);
		else
#endif
		printf("PING %s%s:%s%d%s (%s):\n", c_green, hostname, c_bright, portnr, c_normal, get);
	}

	if (json_output)
		printf("[\n");

	if (adaptive_interval && wait <= 0.0)
		error_exit("Interval must be > 0 when using adaptive interval");

	signal(SIGINT, handler);
	signal(SIGTERM, handler);

	timeout *= 1000;	/* change to ms */

	struct sockaddr_in6 addr;
	struct addrinfo *ai = NULL, *ai_use = NULL;
	struct addrinfo *ai_proxy = NULL, *ai_use_proxy = NULL;

	/*
		if (follow_30x)
		{
			get headers

			const char *get_location(const char *host, int port, char use_ssl, char *reply)

			set new host/port/path/etc
		}
	*/

	double started_at = get_ts();
	if (proxy_host)
	{
#ifdef NC
		if (ncurses_mode)
		{
			slow_log("\nResolving hostname %s", proxy_host);
			update_terminal();
		}
#endif

		if (resolve_host(proxy_host, &ai_proxy, use_ipv6, proxy_port) == -1)
			error_exit(get_error());

		ai_use_proxy = select_resolved_host(ai_proxy, use_ipv6);
		if (!ai_use_proxy)
			error_exit("No valid IPv4 or IPv6 address found for %s", proxy_host);
	}
	else if (resolve_once)
	{
#ifdef NC
		if (ncurses_mode)
		{
			slow_log("\nResolving hostname %s", hostname);
			update_terminal();
		}
#endif

		if (resolve_host(hostname, &ai, use_ipv6, portnr) == -1)
		{
			err++;
			emit_error(verbose, -1, started_at);
			have_resolved = 0;
			if (abort_on_resolve_failure)
				error_exit(get_error());
		}

		ai_use = select_resolved_host(ai, use_ipv6);
		if (!ai_use)
		{
			set_error("No valid IPv4 or IPv6 address found for %s", hostname);

			if (abort_on_resolve_failure)
				error_exit(get_error());

			/* do not emit the resolve-error here: as 'have_resolved' is set to 0
			   next, the program will try to resolve again anyway
			   this prevents a double error-message while err is increased only
			   once
			*/
			have_resolved = 0;
		}

		if (have_resolved)
			get_addr(ai_use, &addr);
	}

	if (persistent_connections)
		fd = -1;

	while((curncount < count || count == -1) && stop == 0)
	{
		double dstart = -1.0, dend = -1.0, dafter_connect = 0.0, dafter_resolve = 0.0;
		char *reply = NULL;
		double Bps = 0;
		char is_compressed = 0;
		long long int bytes_transferred = 0;
		time_t their_ts = 0;
		int age = -1;
		char *sc = NULL, *scdummy = NULL;
		char *fp = NULL;
#if defined(linux) || defined(__FreeBSD__)
		int re_tx = 0, pmtu = 0, tos = 0;
		socklen_t tos_len = sizeof tos;
#endif

		dstart = get_ts();

		for(;;)
		{
			int rc = -1;
			int persistent_tries = 0;
			int len = 0, overflow = 0, headers_len = 0;
			char req_sent = 0;
			double dummy_ms = 0.0;
			double their_est_ts = -1.0, toff_diff_ts = -1.0;
			char tfo_success = 0;
			double ssl_handshake = 0.0;
#if defined(linux) || defined(__FreeBSD__)
			struct tcp_info info;
			socklen_t info_len = sizeof(struct tcp_info);
#endif

			curncount++;

persistent_loop:
			if ((!resolve_once || (resolve_once == 1 && have_resolved == 0)) && fd == -1 && proxy_host == NULL)
			{
				memset(&addr, 0x00, sizeof addr);

#ifdef NC
				if (ncurses_mode && first_resolve)
				{
					slow_log("\nResolving hostname %s", hostname);
					update_terminal();
					first_resolve = 0;
				}
#endif

				if (ai)
				{
					freeaddrinfo(ai);

					ai_use = ai = NULL;
				}

				if (resolve_host(hostname, &ai, use_ipv6, portnr) == -1)
				{
					err++;
					emit_error(verbose, curncount, dstart);

					if (abort_on_resolve_failure)
						error_exit(get_error());
					break;
				}

				ai_use = select_resolved_host(ai, use_ipv6);
				if (!ai_use)
				{
					set_error("No valid IPv4 or IPv6 address found for %s", hostname);
					emit_error(verbose, curncount, dstart);
					err++;

					if (abort_on_resolve_failure)
						error_exit(get_error());

					break;
				}

				get_addr(ai_use, &addr);

				have_resolved = 1;
			}

			dafter_resolve = get_ts();
			dummy_ms = (dafter_resolve - dstart) * 1000.0;
			update_statst(&t_resolve, dummy_ms);

			free(request);
			request = create_request_header(proxy_host ? complete_url : get, proxy_host ? 1 : 0, get_instead_of_head, persistent_connections, add_host_header ? hostname : NULL, useragent, referer, ask_compression, no_cache, auth_usr, auth_password, cookie, proxy_buster, proxy_user, proxy_password);
			req_len = strlen(request);

			if ((persistent_connections && fd < 0) || !persistent_connections)
			{
				if (proxy_host && proxy_is_socks5)
					fd = socks5connect(ai_use_proxy, timeout, proxy_user, proxy_password, hostname, portnr, abort_on_resolve_failure);
#ifndef NO_SSL
				else if (proxy_host && use_ssl)
					fd = connect_ssl_proxy((struct sockaddr *)bind_to, ai_use_proxy, timeout, proxy_user, proxy_password, hostname, portnr, &use_tfo);
#endif
				else if (proxy_host)
					fd = connect_to((struct sockaddr *)bind_to, ai_use_proxy, timeout, &use_tfo, request, req_len, &req_sent, -1);
				else
					fd = connect_to((struct sockaddr *)bind_to, ai_use, timeout, &use_tfo, request, req_len, &req_sent, max_mtu);
			}

			if (fd == RC_CTRLC)	/* ^C pressed */
				break;

			if (fd < 0)
			{
				emit_error(verbose, curncount, dstart);
				fd = -1;
			}

			if (fd >= 0)
			{
				/* set fd blocking */
				if (set_fd_blocking(fd) == -1)
				{
					stats_close(&fd, &t_close, 1);
					break;
				}

				/* set socket to low latency */
				if (use_tcp_nodelay && set_tcp_low_latency(fd) == -1)
				{
					stats_close(&fd, &t_close, 1);
					break;
				}

#ifndef NO_SSL
				if (use_ssl && ssl_h == NULL)
				{
					int rc = connect_ssl(fd, client_ctx, &ssl_h, &s_bio, timeout, &ssl_handshake);
					if (rc == 0)
						update_statst(&t_ssl, ssl_handshake);
					else
					{
						stats_close(&fd, &t_close, 1);
						fd = rc;

						if (persistent_connections && ++persistent_tries < 2)
						{
							persistent_did_reconnect = 1;

							goto persistent_loop;
						}
					}
				}
#endif
			}

			dafter_connect = get_ts();

			dummy_ms = (dafter_connect - dafter_resolve) * 1000.0;
			update_statst(&t_connect, dummy_ms - ssl_handshake);

			if (fd < 0)
			{
				if (fd == RC_TIMEOUT)
					set_error("timeout connecting to host");

				emit_error(verbose, curncount, dstart);
				err++;

				fd = -1;

				break;
			}

#ifndef NO_SSL
			if (use_ssl)
				rc = WRITE_SSL(ssl_h, request, req_len);
			else
#endif
			{
				if (!req_sent)
					rc = mywrite(fd, request, req_len, timeout);
				else
					rc = req_len;
			}

			if (rc != req_len)
			{
				if (persistent_connections)
				{
					if (++persistent_tries < 2)
					{
						stats_close(&fd, &t_close, 0);
						persistent_did_reconnect = 1;
						goto persistent_loop;
					}
				}

				if (rc == -1)
					set_error("error sending request to host");
				else if (rc == RC_TIMEOUT)
					set_error("timeout sending to host");
				else if (rc == RC_INVAL)
					set_error("retrieved invalid data from host");
				else if (rc == RC_CTRLC)
				{/* ^C */}
				else if (rc == 0)
					set_error("connection prematurely closed by peer");

				emit_error(verbose, curncount, dstart);

				stats_close(&fd, &t_close, 1);
				err++;

				break;
			}

#if defined(linux) || defined(__FreeBSD__)
#ifdef NC
			if (!use_ssl && ncurses_mode)
			{
				int t_rc = -1;
				fd_set rfds;
				FD_ZERO(&rfds);
				FD_SET(fd, &rfds);

				struct timeval tv;
				tv.tv_sec = timeout;
				tv.tv_usec = 0;

				t_rc = select(fd + 1, &rfds, NULL, NULL, &tv);

				if (t_rc == 1 && \
					FD_ISSET(fd, &rfds) && \
					getsockopt(fd, IPPROTO_TCP, TCP_INFO, &info, &info_len) == 0 && \
					info.tcpi_unacked > 0)
				{
					static int in_transit_cnt = 0;

					in_transit_cnt++;
					if (in_transit_cnt == 4)
						slow_log("\nNo longer emitting message about \"still data in transit\"");
					else if (in_transit_cnt < 4)
						slow_log("\nHTTP server started sending data with %d bytes still in transit", info.tcpi_unacked);
				}

				if (t_rc == 0)
				{
					stats_close(&fd, &t_close, 1);

					rc = RC_TIMEOUT;
					set_error("timeout sending to host");

					emit_error(verbose, curncount, dstart);

					err++;

					break;
				}
			}
#endif

			if (getsockopt(fd, IPPROTO_IP, IP_TOS, &tos, &tos_len) == -1)
			{
				set_error("failed to obtain TOS info");
				tos = -1;
			}
#endif

			rc = get_HTTP_headers(fd, ssl_h, &reply, &overflow, timeout);

#ifdef NC
			if (ncurses_mode && !get_instead_of_head && overflow > 0)
			{
				static int more_data_cnt = 0;

				more_data_cnt++;
				if (more_data_cnt == 4)
					slow_log("\nNo longer emitting message about \"more data than response headers\"");
				else if (more_data_cnt < 4)
					slow_log("\nHTTP server sent more data than just the response headers");
			}
#endif

			emit_headers(reply);

			if ((show_statuscodes || machine_readable || json_output || ncurses_mode) && reply != NULL)
			{
				/* statuscode is in first line behind
				 * 'HTTP/1.x'
				 */
				char *dummy = strchr(reply, ' ');

				if (dummy)
				{
					sc = strdup(dummy + 1);

					/* lines are normally terminated with a
					 * CR/LF
					 */
					dummy = strchr(sc, '\r');
					if (dummy)
						*dummy = 0x00;
					dummy = strchr(sc, '\n');
					if (dummy)
						*dummy = 0x00;
				}
			}

			their_ts = parse_date_from_response_headers(reply);

			age = calc_page_age(reply, their_ts);

			is_compressed = check_compressed(reply);

			if (persistent_connections && show_bytes_xfer && reply != NULL)
			{
				char *length = strstr(reply, "\nContent-Length:");
				if (!length)
				{
					set_error("'Content-Length'-header missing!");
					emit_error(verbose, curncount, dstart);
					stats_close(&fd, &t_close, 1);
					break;
				}

				len = atoi(&length[17]);
			}

			headers_len = 0;
			if (reply)
			{
				headers_len = strlen(reply) + 4;
				free(reply);
				reply = NULL;
			}

			if (rc < 0)
			{
				if (persistent_connections)
				{
					if (++persistent_tries < 2)
					{
						stats_close(&fd, &t_close, 0);
						persistent_did_reconnect = 1;
						goto persistent_loop;
					}
				}

				if (rc == RC_SHORTREAD)
					set_error("short read during receiving reply-headers from host");
				else if (rc == RC_TIMEOUT)
					set_error("timeout while receiving reply-headers from host");

				emit_error(verbose, curncount, dstart);

				stats_close(&fd, &t_close, 1);
				err++;

				break;
			}

			ok++;

			if (get_instead_of_head && show_Bps)
			{
				double dl_start = get_ts(), dl_end;
				double cur_limit = Bps_limit;

				if (persistent_connections)
				{
					if (cur_limit == -1 || len < cur_limit)
						cur_limit = len - overflow;
				}

				for(;;)
				{
					int n = cur_limit != -1 ? min(cur_limit - bytes_transferred, buffer_size) : buffer_size;
					int rc = read(fd, buffer, n);

					if (rc == -1)
					{
						if (errno != EINTR && errno != EAGAIN)
							error_exit("read failed");
					}
					else if (rc == 0)
					{
						break;
					}

					bytes_transferred += rc;

					if (cur_limit != -1 && bytes_transferred >= cur_limit)
						break;
				}

				dl_end = get_ts();

				Bps = (double)bytes_transferred / max(dl_end - dl_start, 0.000001);
				Bps_min = min(Bps_min, Bps);
				Bps_max = max(Bps_max, Bps);
				Bps_avg += Bps;
			}

			dend = get_ts();

#ifndef NO_SSL
			if (use_ssl)
			{
				if ((show_fp || json_output || ncurses_mode) && ssl_h != NULL)
					fp = get_fingerprint(ssl_h);

				if (!persistent_connections)
				{
					if (close_ssl_connection(ssl_h) == -1)
					{
						set_error("error shutting down ssl");
						emit_error(verbose, curncount, dstart);
					}

					SSL_free(ssl_h);
					ssl_h = NULL;
					s_bio = NULL;
				}
			}
#endif

#if defined(linux) || defined(__FreeBSD__)
			if (getsockopt(fd, IPPROTO_TCP, TCP_INFO, &info, &info_len) == 0)
			{
#ifdef TCP_TFO
				if (info.tcpi_options & TCPI_OPT_SYN_DATA)
					tfo_success = 1;
#endif

				update_statst(&tcp_rtt_stats, (double)info.tcpi_rtt / 1000.0);

				re_tx = info.tcpi_retransmits;
				pmtu = info.tcpi_pmtu;
			}
#endif

			if (!persistent_connections)
				stats_close(&fd, &t_close, 0);

			dummy_ms = (dend - dafter_connect) * 1000.0;
			update_statst(&t_request, dummy_ms);

			dummy_ms = (dend - dstart) * 1000.0;
			update_statst(&t_total, dummy_ms);

			their_est_ts = (dend + dafter_connect) / 2.0; /* estimate of when other end started replying */
			toff_diff_ts = ((double)their_ts - their_est_ts) * 1000.0;
			update_statst(&stats_to, toff_diff_ts);

			if (json_output)
			{
				char current_host[1024] = "?";

				if (proxy_host)
					snprintf(current_host, sizeof current_host, "%s", hostname);
				else if (getnameinfo((const struct sockaddr *)&addr, sizeof addr, current_host, sizeof current_host, NULL, 0, NI_NUMERICHOST) == -1)
					snprintf(current_host, sizeof current_host, "getnameinfo() failed: %d (%s)", errno, strerror(errno));

				emit_json(1, curncount, dstart, &t_resolve, &t_connect, &t_request, atoi(sc ? sc : "-1"), sc ? sc : "?", headers_len, len, Bps, current_host, fp, toff_diff_ts, tfo_success);
			}
			else if (machine_readable)
			{
				if (sc)
				{
					char *dummy = strchr(sc, ' ');
					if (dummy)
						*dummy = 0x00;

					if (strstr(ok_str, sc))
						printf("%f", t_total.cur);
					else
						printf("%s", err_str);

					if (show_statuscodes)
						printf(" %s", sc);
				}
				else
				{
					printf("%s", err_str);
				}

				if(audible)
					putchar('\a');

				printf("\n");
			}
			else if (!quiet && !nagios_mode && t_total.cur >= offset_show)
			{
				char line[4096] = { 0 };
				int pos = 0;
				const char *ms_color = c_green;
				char current_host[1024] = "?";
				char *operation = !persistent_connections ? "connected to" : "pinged host";
				const char *sep = c_bright, *unsep = c_normal;

				if (show_ts || ncurses_mode)
				{
					char *ts = get_ts_str(verbose);

					if (ncurses_mode)
						pos += snprintf(&line[pos], sizeof line - pos, "[%s] ", ts);
					else
						pos += snprintf(&line[pos], sizeof line - pos, "%s ", ts);

					free(ts);
				}

				if (curncount & 1)
				{
					printf("%s", c_bright);
					sep = c_normal;
					unsep = c_bright;
				}

				if (proxy_host)
					snprintf(current_host, sizeof current_host, "%s", hostname);
				else if (getnameinfo((const struct sockaddr *)&addr, sizeof addr, current_host, sizeof current_host, NULL, 0, NI_NUMERICHOST) == -1)
					snprintf(current_host, sizeof current_host, "getnameinfo() failed: %d (%s)", errno, strerror(errno));

				const char *i6_bs = "", *i6_be = "";
				if (addr.sin6_family == AF_INET6)
				{
					i6_bs = "[";
					i6_be = "]";
				}

				if (offset_red > 0.0 && t_total.cur >= offset_red)
					ms_color = c_red;
				else if (offset_yellow > 0.0 && t_total.cur >= offset_yellow)
					ms_color = c_yellow;

				if (!ncurses_mode)
					pos += snprintf(&line[pos], sizeof line - pos, "%s%s ", c_white, operation);

				if (persistent_connections && show_bytes_xfer)
					pos += snprintf(&line[pos], sizeof line - pos, "%s%s%s%s%s:%s%d%s (%d/%d bytes), seq=%s%d%s ", c_red, i6_bs, current_host, i6_be, c_white, c_yellow, portnr, c_white, headers_len, len, c_blue, curncount-1, c_white);
				else
					pos += snprintf(&line[pos], sizeof line - pos, "%s%s%s%s%s:%s%d%s (%d bytes), seq=%s%d%s ", c_red, i6_bs, current_host, i6_be, c_white, c_yellow, portnr, c_white, headers_len, c_blue, curncount-1, c_white);

				if (split)
					pos += snprintf(&line[pos], sizeof line - pos, "time=%.2f+%.2f%s+%s%.2f%s=%s%s%.2f%s ms %s%s%s", t_resolve.cur, t_connect.cur, sep, unsep, t_request.cur, sep, unsep, ms_color, t_total.cur, c_white, c_cyan, sc?sc:"", c_white);
				else
					pos += snprintf(&line[pos], sizeof line - pos, "time=%s%.2f%s ms %s%s%s", ms_color, t_total.cur, c_white, c_cyan, sc?sc:"", c_white);

				if (persistent_did_reconnect)
				{
					pos += snprintf(&line[pos], sizeof line - pos, " %sC%s", c_magenta, c_white);
					persistent_did_reconnect = 0;
				}

				if (show_Bps)
				{
					pos += snprintf(&line[pos], sizeof line - pos, " %dKB/s", (int)Bps / 1024);
					if (show_bytes_xfer)
						pos += snprintf(&line[pos], sizeof line - pos, " %dKB", (int)(bytes_transferred / 1024));
					if (ask_compression)
					{
						pos += snprintf(&line[pos], sizeof line - pos, " (");
						if (!is_compressed)
							pos += snprintf(&line[pos], sizeof line - pos, "not ");
						pos += snprintf(&line[pos], sizeof line - pos, "compressed)");
					}
				}

				if (use_ssl && show_fp && fp != NULL)
				{
					pos += snprintf(&line[pos], sizeof line - pos, " %s", fp);
				}

				if (verbose > 0 && their_ts > 0)
				{
					/*  if diff_ts > 0, then their clock is running too fast */
					pos += snprintf(&line[pos], sizeof line - pos, " toff=%d", (int)toff_diff_ts);
				}

				if (verbose > 0 && age > 0)
					pos += snprintf(&line[pos], sizeof line - pos, " age=%d", age);

				pos += snprintf(&line[pos], sizeof line - pos, "%s", c_normal);

				if (audible)
				{
#ifdef NC
					my_beep();
#else
					putchar('\a');
#endif
				}

#ifdef TCP_TFO
				if (tfo_success)
					pos += snprintf(&line[pos], sizeof line - pos, " F");
#endif

#ifdef NC
				if (ncurses_mode)
				{
					if (dummy_ms >= show_slow_log)
						slow_log("\n%s", line);
					else
						fast_log("\n%s", line);
				}
				else
#endif
					printf("%s\n", line);

				do_aggregates(t_total.cur, (int)(get_ts() - started_at), n_aggregates, aggregates, verbose, show_ts);
			}

			if (show_statuscodes && ok_str != NULL && sc != NULL)
			{
				scdummy = strchr(sc, ' ');
				if (scdummy) *scdummy = 0x00;

				if (strstr(ok_str, sc) == NULL)
				{
					ok--;
					err++;
				}
			}

			break;
		}

		emit_statuslines(get_ts() - started_at);
#ifdef NC
		if (ncurses_mode)
			update_stats(&t_resolve, &t_connect, &t_request, &t_total, &t_ssl, curncount, err, sc, fp, use_tfo, nc_graph, &stats_to, &tcp_rtt_stats, re_tx, pmtu, tos, &t_close);
#endif

		free(sc);
		free(fp);

#ifdef NC
		if (ncurses_mode)
			update_terminal();
		else
#endif
		fflush(NULL);

		if (!stop && wait > 0)
		{
			double cur_sleep = wait;

			if (adaptive_interval)
			{
				double now = get_ts();
				double interval_left = fmod(now - started_at, wait);

				if (interval_left <= 0.0)
					cur_sleep = wait;
				else
					cur_sleep = wait - interval_left;
			}

			usleep((useconds_t)(cur_sleep * 1000000.0));
		}
	}

#ifdef NC
	if (ncurses_mode)
		end_ncurses();
#endif

	if (ok)
		avg_httping_time = t_total.avg / (double)t_total.n;
	else
		avg_httping_time = -1.0;

	total_took = get_ts() - started_at;
	if (!quiet && !machine_readable && !nagios_mode && !json_output)
	{
		int dummy = count;

		printf("--- %s ping statistics ---\n", complete_url);

		if (curncount == 0 && err > 0)
			fprintf(stderr, "internal error! (curncount)\n");

		if (count == -1)
			dummy = curncount;

		printf("%s%d%s connects, %s%d%s ok, %s%3.2f%%%s failed, time %s%s%.0fms%s\n", c_yellow, curncount, c_normal, c_green, ok, c_normal, c_red, (((double)err) / ((double)dummy)) * 100.0, c_normal, c_blue, c_bright, total_took * 1000.0, c_normal);

		if (ok > 0)
		{
			printf("round-trip min/avg/max%s = %s%.1f%s/%s%.1f%s/%s%.1f%s", verbose ? "/sd" : "", c_bright, t_total.min, c_normal, c_bright, avg_httping_time, c_normal, c_bright, t_total.max, c_normal);

			if (verbose)
			{
				double sd_final = t_total.n ? sqrt((t_total.sd / (double)t_total.n) - pow(avg_httping_time, 2.0)) : -1.0;
				printf("/%.1f", sd_final);
			}

			printf(" ms\n");

			if (show_Bps)
				printf("Transfer speed: min/avg/max = %s%f%s/%s%f%s/%s%f%s KB\n", c_bright, Bps_min / 1024, c_normal, c_bright, (Bps_avg / (double)ok) / 1024.0, c_normal, c_bright, Bps_max / 1024.0, c_normal);
		}
	}

error_exit:
	if (nagios_mode)
		return nagios_result(ok, nagios_mode, nagios_exit_code, avg_httping_time, nagios_warn, nagios_crit);

	if (!json_output && !machine_readable)
		printf("%s", c_very_normal);

	if (json_output)
		printf("\n]\n");

	freeaddrinfo(ai);
	free(request);
	free(buffer);
	free(get);
	free(hostname);
	free(complete_url);

	free(aggregates);

#ifndef NO_SSL
	if (use_ssl)
	{
		SSL_CTX_free(client_ctx);

		shutdown_ssl();
	}
#endif

	fflush(NULL);

	if (ok)
		return 0;
	else
		return 127;
}
