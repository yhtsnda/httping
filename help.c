#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <libintl.h>

#include "gen.h"
#include "main.h"
#include "help.h"
#include "utils.h"

void new_version_alert(void)
{
	char new_version = 0;
	FILE *fh = fopen(SPAM_FILE, "r");
	if (!fh)
		new_version = 1;
	else
	{
		char buffer[4096], *dummy = 0x00;

		fgets(buffer, sizeof buffer, fh);

		fclose(fh);

		dummy = strchr(buffer, '\n');
		if (dummy)
			*dummy = 0x00;

		if (strcmp(buffer, VERSION) != 0)
			new_version = 1;
	}

	if (new_version)
	{
		struct utsname buf;
		FILE *fh = fopen(SPAM_FILE, "w");
		if (fh)
		{
			fprintf(fh, "%s\n", VERSION);

			fclose(fh);
		}

		printf("Welcome to the new HTTPing version " VERSION "!\n\n");
#ifdef NC
		printf("Did you know that with -K you can start a fullscreen GUI version with nice graphs and lots more information? And that you can disable the moving graphs with -D?\n");
#ifndef FW
		printf("And if you compile this program with libfftw3, that it can also show a fourier transform of the measured values?\n");
#endif
#else
		printf("Did you know that if you compile this program with NCURSES, that it then includes a nice GUI with lots more information and graphs?\n");
#endif

#if !defined(TCP_TFO) && defined(linux)
		if (uname(&buf) == 0)
		{
			char **rparts = NULL;
			int n_rparts = 0;

			split_string(buf.release, ".", &rparts, &n_rparts);

			if (n_rparts >= 2 && ((atoi(rparts[0]) >= 3 && atoi(rparts[1]) >= 6) || atoi(rparts[0]) >= 4))
				printf("This program supports TCP Fast Open! (if compiled in and only on Linux kernels 3.6 or more recent) See the readme.txt how to enable this.\n");

			free_splitted_string(rparts, n_rparts);
		}
#endif

		printf("\n\n");
	}
}

void version(void)
{
	fprintf(stderr, gettext("HTTPing v" VERSION ", (C) 2003-2013 folkert@vanheusden.com\n"));
#ifndef NO_SSL
	fprintf(stderr, gettext(" * SSL support included (-l)\n"));
#endif

#ifdef NC
#ifdef FW
	fprintf(stderr, gettext(" * ncurses interface with FFT included (-K)\n"));
#else
	fprintf(stderr, gettext(" * ncurses interface included (-K)\n"));
#endif
#endif

#ifdef TCP_TFO
	fprintf(stderr, gettext(" * TFO (TCP fast open) support included (-F)\n"));
#endif
	fprintf(stderr, gettext("\n"));
}

void format_help(const char *short_str, const char *long_str, const char *descr)
{
	int par_width = SWITCHES_COLUMN_WIDTH, max_wrap_width = MAX_WORD_SIZE_WRAP;
	int descr_width = max_x - (par_width + 1);
	char *line = NULL, *p = (char *)descr;
	char first = 1;

	if (long_str)
		str_add(&line, "%s / %s", short_str, long_str);
	else
		str_add(&line, "%s", short_str);

	fprintf(stderr, "%-*s ", par_width, line);

	free(line);

	if (par_width + 1 >= max_x)
	{
		fprintf(stderr, "%s\n", descr);
		return;
	}

	for(;strlen(p);)
	{
		char *n = &p[min((int)strlen(p), descr_width - 1)], *kn = n, *copy = NULL;
		int n_len = 0, cur_len = 0;

		while(*p == ' ')
			p++;

		n = &p[min((int)strlen(p), descr_width - 1)];
		kn = n;

		while (*n != ' ' && n_len < max_wrap_width)
		{
			n--;
			n_len++;
		}

		if (n_len >= max_wrap_width)
			n = kn;

		cur_len = (int)(n - p);

		copy = (char *)malloc(cur_len + 1);
		memcpy(copy, p, cur_len);
		copy[cur_len] = 0x00;

		if (first)
			first = 0;
		else
			fprintf(stderr, "%*s ", par_width, "");

		fprintf(stderr, "%s\n", copy);

		free(copy);

		p = n;
	}
}

void help_long(void)
{
	fprintf(stderr, gettext("--aggregate x[,y[,z]]  show an aggregate each x[/y[/z[/etc]]] seconds\n"));
	fprintf(stderr, gettext("--ai / --adaptive-interval  execute pings at multiples of interval relative to start, default on in ncurses output mode\n"));
	fprintf(stderr, gettext("--audible-ping         -a\n"));
	fprintf(stderr, gettext("--basic-auth           -A\n"));
	fprintf(stderr, gettext("--bind-to              -y\n"));
	fprintf(stderr, gettext("--colors               -Y\n"));
	fprintf(stderr, gettext("--cookie               -C\n"));
	fprintf(stderr, gettext("--count                -c\n"));
	fprintf(stderr, gettext("--data-limit           -L\n"));
	fprintf(stderr, gettext("--divert-connect       connect to a different host than in the URL given\n"));
#if defined(NC) && defined(FW)
	fprintf(stderr, gettext("--draw-phase           draw phase (fourier transform) in gui\n"));
#endif
	fprintf(stderr, gettext("--nagios-mode-2        -n\n"));
	fprintf(stderr, gettext("--flood                -f\n"));
	fprintf(stderr, gettext("--get-request          -G\n"));
	fprintf(stderr, gettext("--graph-limit x        do not scale to values above x\n"));
	fprintf(stderr, gettext("--hostname             -h\n"));
	fprintf(stderr, gettext("--host-port            -x\n"));
	fprintf(stderr, gettext("--interval             -i\n"));
	fprintf(stderr, gettext("--ipv6                 -6\n"));
	fprintf(stderr, gettext("--keep-cookies         return the cookies given by the HTTP server in the following request(s)\n"));
	fprintf(stderr, gettext("--max-mtu              limit the MTU size\n"));
	fprintf(stderr, gettext("--nagios-mode-1        -n\n"));
	fprintf(stderr, gettext("--nagios-mode-2        -n\n"));
#ifdef NC
	fprintf(stderr, gettext("--ncurses              -K\n"));
#endif
	fprintf(stderr, gettext("--no-cache             -Z\n"));
	fprintf(stderr, gettext("--no-graph             -D\n"));
	fprintf(stderr, gettext("--no-host-header       do not add \"Host:\"-line to the request headers\n"));
	fprintf(stderr, gettext("--no-tcp-nodelay       do not disable Naggle\n"));
	fprintf(stderr, gettext("--ok-result-codes      -o (only for -m)\n"));
	fprintf(stderr, gettext("--parseable-output     -m\n"));
	fprintf(stderr, gettext("--password             -P\n"));
	fprintf(stderr, gettext("--persistent-connections  -Q\n"));
	fprintf(stderr, gettext("--port                 -p\n"));
	fprintf(stderr, gettext("--proxy-buster x       adds \"&x=[random value]\" to the request URL\n"));
	fprintf(stderr, gettext("--proxy-user           \n"));
	fprintf(stderr, gettext("--proxy-password       \n"));
	fprintf(stderr, gettext("--proxy-password-file  \n"));
	fprintf(stderr, gettext("--quiet                -q\n"));
	fprintf(stderr, gettext("--recv-buffer          receive buffer size\n"));
	fprintf(stderr, gettext("--referer              -R\n"));
	fprintf(stderr, gettext("--resolve-once         -r\n"));
	fprintf(stderr, gettext("--result-string        -e\n"));
	fprintf(stderr, gettext("--show-kb              -X\n"));
	fprintf(stderr, gettext("--show-statusodes      -s\n"));
	fprintf(stderr, gettext("--show-transfer-speed  -b\n"));
	fprintf(stderr, gettext("--show-xfer-speed-compressed  -B\n"));
#ifdef NC
	fprintf(stderr, gettext("--slow-log             when the duration is x or more, show ping line in the slow log window (the middle window)\n"));
#endif
	fprintf(stderr, gettext("--split-time           -S\n"));
#ifdef TCP_TFO
	fprintf(stderr, gettext("--tcp-fast-open        -F\n"));
#endif
	fprintf(stderr, gettext("--threshold-red        from what ping value to show the value in red (must be bigger than yellow)\n"));
	fprintf(stderr, gettext("--threshold-show       from what ping value to show the results\n"));
	fprintf(stderr, gettext("--threshold-yellow     from what ping value to show the value in yellow\n"));
	fprintf(stderr, gettext("--timeout              -t\n"));
	fprintf(stderr, gettext("--timestamp / --ts     put a timestamp before the measured values, use -v to include the date and -vv to show in microseconds\n"));
	fprintf(stderr, gettext("--tx-buffer            transmit buffer size\n"));
	fprintf(stderr, gettext("--url                  -g\n"));
	fprintf(stderr, gettext("--user-agent           -I\n"));
	fprintf(stderr, gettext("--username             -U\n"));
#ifndef NO_SSL
	fprintf(stderr, gettext("--use-ssl              -l\n"));
	fprintf(stderr, gettext("--show-fingerprint     -z\n"));
#endif
	fprintf(stderr, gettext("--version              -V\n"));
	fprintf(stderr, gettext("--help                 -H\n"));
}

void usage(const char *me)
{
	char *dummy = NULL, has_color = 0;
	char host[256] = { 0 };

	fprintf(stderr, gettext("\n-g url         url (e.g. -g http://localhost/)\n"));
	fprintf(stderr, gettext("-h hostname    hostname (e.g. localhost)\n"));
	fprintf(stderr, gettext("-p portnr      portnumber (e.g. 80)\n"));
	fprintf(stderr, gettext("-x host:port   hostname+portnumber of proxyserver\n"));
	fprintf(stderr, gettext("-5             proxy is a socks5 server\n"));
	fprintf(stderr, gettext("-c count       how many times to connect\n"));
	fprintf(stderr, gettext("-i interval    delay between each connect\n"));
	fprintf(stderr, gettext("-t timeout     timeout (default: 30s)\n"));
	fprintf(stderr, gettext("-Z             ask any proxies on the way not to cache the requests\n"));
	fprintf(stderr, gettext("-Q             use a persistent connection. adds a 'C' to the output if httping had to reconnect\n"));
	fprintf(stderr, gettext("-6             use IPv6\n"));
	fprintf(stderr, gettext("-s             show statuscodes\n"));
	fprintf(stderr, gettext("-S             split time in connect-time and processing time\n"));
	fprintf(stderr, gettext("-G             do a GET request instead of HEAD (read the contents of the page as well)\n"));
	fprintf(stderr, gettext("-b             show transfer speed in KB/s (use with -G)\n"));
	fprintf(stderr, gettext("-B             like -b but use compression if available\n"));
	fprintf(stderr, gettext("-L x           limit the amount of data transferred (for -b) to 'x' (in bytes)\n"));
	fprintf(stderr, gettext("-X             show the number of KB transferred (for -b)\n"));
#ifndef NO_SSL
	fprintf(stderr, gettext("-l             connect using SSL\n"));
	fprintf(stderr, gettext("-z             show fingerprint (SSL)\n"));
#endif
	fprintf(stderr, gettext("-f             flood connect (no delays)\n"));
	fprintf(stderr, gettext("-a             audible ping\n"));
	fprintf(stderr, gettext("-m             give machine parseable output (see also -o and -e)\n"));
	fprintf(stderr, gettext("-M             json output, cannot be combined with -m\n"));
	fprintf(stderr, gettext("-o rc,rc,...   what http results codes indicate 'ok' comma seperated WITHOUT spaces inbetween default is 200, use with -e\n"));
	fprintf(stderr, gettext("-e str         string to display when http result code doesn't match\n"));
	fprintf(stderr, gettext("-I str         use 'str' for the UserAgent header\n"));
	fprintf(stderr, gettext("-R str         use 'str' for the Referer header\n"));
	format_help("-r", NULL, "resolve hostname only once (usefull when pinging roundrobin DNS: also takes the first DNS lookup out of the loop so that the first measurement is also correct)");
	fprintf(stderr, gettext("-W             do not abort the program if resolving failed: keep retrying\n"));
	fprintf(stderr, gettext("-n warn,crit   Nagios-mode: return 1 when avg. response time >= warn, 2 if >= crit, otherwhise return 0\n"));
	fprintf(stderr, gettext("-N x           Nagios mode 2: return 0 when all fine, 'x'\n when anything failes\n"));
	fprintf(stderr, gettext("-y ip[:port]   bind to ip-address (and thus interface) [/port]\n"));
	fprintf(stderr, gettext("-q             quiet, only returncode\n"));
	fprintf(stderr, gettext("-A             Activate Basic authentication\n"));
	fprintf(stderr, gettext("-U username    needed for authentication\n"));
	fprintf(stderr, gettext("-P password    needed for authentication\n"));
	fprintf(stderr, gettext("-T x           read the password fom the file 'x' (replacement for -P)\n"));
	fprintf(stderr, gettext("-C cookie=value Add a cookie to the request\n"));
	fprintf(stderr, gettext("-Y             add colors\n"));
	fprintf(stderr, gettext("-E             fetch proxy settings from environment variables\n"));
	fprintf(stderr, gettext("-K             ncurses mode\n"));
#ifdef TCP_TFO
	fprintf(stderr, gettext("-F             \"TCP fast open\" (TFO), reduces the latency of TCP connects\n"));
#endif
	fprintf(stderr, gettext("-v             verbose mode\n"));
	fprintf(stderr, gettext("-V             show the version\n\n"));
	fprintf(stderr, gettext("\n"));
	fprintf(stderr, gettext("-J             list long options\n"));
	fprintf(stderr, gettext("NOTE: not all functionality has a \"short\" switch, so not all are listed here! Please check -J too.\n"));
	fprintf(stderr, gettext("\n"));

	dummy = getenv("TERM");
	if (dummy)
	{
		if (strstr(dummy, "ANSI") || strstr(dummy, "xterm") || strstr(dummy, "screen"))
			has_color = 1;
	}

	if (gethostname(host, sizeof host))
		strcpy(host, "localhost");

	fprintf(stderr, gettext("Example:\n"));
	fprintf(stderr, "\t%s %s%s -s -Z\n\n", me, host, has_color ? " -Y" : "");

	new_version_alert();
}
