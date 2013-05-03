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
	int par_width = SWITCHES_COLUMN_WIDTH, max_wrap_width = par_width / 2, cur_par_width = 0;
	int descr_width = max_x - (par_width + 1);
	char *line = NULL, *p = (char *)descr;
	char first = 1;

	if (long_str && short_str)
		str_add(&line, "%s / %s", short_str, long_str);
	else if (long_str)
		str_add(&line, "%s", long_str);
	else
		str_add(&line, "%s", short_str);

	cur_par_width = fprintf(stderr, "%-*s ", par_width, line);

	free(line);

	if (par_width + 1 >= max_x || cur_par_width >= max_x)
	{
		fprintf(stderr, "%s\n", descr);
		return;
	}

	for(;strlen(p);)
	{
		char *n =  NULL, *kn = NULL, *copy = NULL;
		int n_len = 0, len_after_ww = 0, len_before_ww = 0;
		int str_len = 0, cur_descr_width = first ? max_x - cur_par_width : descr_width;

		while(*p == ' ')
			p++;

		str_len = strlen(p);
		if (!str_len)
			break;

		len_before_ww = min(str_len, cur_descr_width);

		n = &p[len_before_ww];
		kn = n;

		if (str_len > cur_descr_width)
		{ 
			while (*n != ' ' && n_len < max_wrap_width)
			{
				n--;
				n_len++;
			}

			if (n_len >= max_wrap_width)
				n = kn;
		}

		len_after_ww = (int)(n - p);
		if (len_after_ww <= 0)
			break;

		copy = (char *)malloc(len_after_ww + 1);
		memcpy(copy, p, len_after_ww);
		copy[len_after_ww] = 0x00;

		if (first)
			first = 0;
		else
			fprintf(stderr, "%*s ", par_width, "");

		fprintf(stderr, "%s\n", copy);

		free(copy);

		p = n;
	}
}
void usage(const char *me)
{
	char *dummy = NULL, has_color = 0;
	char host[256] = { 0 };

#if 0
	fprintf(stderr, gettext("--aggregate x[,y[,z]]  show an aggregate each x[/y[/z[/etc]]] seconds\n"));
	fprintf(stderr, gettext("--ai / --adaptive-interval  execute pings at multiples of interval relative to start, default on in ncurses output mode\n"));
	fprintf(stderr, gettext("--divert-connect       connect to a different host than in the URL given\n"));
#if defined(NC) && defined(FW)
	fprintf(stderr, gettext("--draw-phase           draw phase (fourier transform) in gui\n"));
#endif
	fprintf(stderr, gettext("--graph-limit x        do not scale to values above x\n"));
	fprintf(stderr, gettext("--keep-cookies         return the cookies given by the HTTP server in the following request(s)\n"));
	fprintf(stderr, gettext("--max-mtu              limit the MTU size\n"));
#ifdef NC
#endif
	fprintf(stderr, gettext("--no-host-header       do not add \"Host:\"-line to the request headers\n"));
	fprintf(stderr, gettext("--no-tcp-nodelay       do not disable Naggle\n"));
	fprintf(stderr, gettext("--ok-result-codes      -o (only for -m)\n"));
	fprintf(stderr, gettext("--parseable-output     -m\n"));
	fprintf(stderr, gettext("--password             -P\n"));
	fprintf(stderr, gettext("--persistent-connections  -Q\n"));
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
#endif
	format_help("-g url", NULL, gettext("URL to ping (e.g. -g http://localhost/)"));
	format_help("-h hostname", "--hostname", gettext("hostname to ping (e.g. localhost) - use either -g or -h"));
	format_help("-p portnr", "--port", gettext("portnumber (e.g. 80) - use with -h"));
	format_help("-x host:port", "--proxy", gettext("-x host:port   hostname+portnumber of proxyserver"));
	format_help("-5", NULL, gettext("proxy is a socks5 server"));
	format_help("-c count", "--count", gettext("how many times to connect"));
	format_help("-i interval", "--interval", gettext("delay between each connect"));
	format_help("-t timeout", NULL, gettext("timeout (default: 30s)"));
	format_help("-Z", "--no-cache", gettext("ask any proxies on the way not to cache the requests"));
	format_help("-Q", NULL, gettext("use a persistent connection. adds a 'C' to the output if httping had to reconnect"));
	format_help("-6", "--ipv6", gettext("use IPv6"));
	format_help("-s", NULL, gettext("show statuscodes"));
	format_help("-S", NULL, gettext("split time in connect-time and processing time"));
	format_help("-G", "--get-request", gettext("do a GET request instead of HEAD (read the contents of the page as well)"));
	format_help("-b", NULL, gettext("show transfer speed in KB/s (use with -G)"));
	format_help("-B", NULL, gettext("like -b but use compression if available"));
	format_help("-L x", "--data-limit", gettext("limit the amount of data transferred (for -b) to 'x' (in bytes)"));
	format_help("-X", NULL, gettext("show the number of KB transferred (for -b)"));
#ifndef NO_SSL
	format_help("-l", NULL, gettext("connect using SSL"));
	format_help("-z", NULL, gettext("show fingerprint (SSL)"));
#endif
	format_help("-f", "--flood", gettext("flood connect (no delays)"));
	format_help("-a", "--audible-ping", gettext("audible ping"));
	format_help("-m", NULL, gettext("give machine parseable output (see also -o and -e)"));
	format_help("-M", NULL, gettext("json output, cannot be combined with -m"));
	format_help("-o rc,rc,...", NULL, gettext("what http results codes indicate 'ok' comma seperated WITHOUT spaces inbetween default is 200, use with -e"));
	format_help("-e str", NULL, gettext("string to display when http result code doesn't match"));
	format_help("-I str", NULL, gettext("use 'str' for the UserAgent header"));
	format_help("-R str", NULL, gettext("use 'str' for the Referer header"));
	format_help("-r", NULL, gettext("resolve hostname only once (usefull when pinging roundrobin DNS: also takes the first DNS lookup out of the loop so that the first measurement is also correct)"));
	format_help("-W", NULL, gettext("do not abort the program if resolving failed: keep retrying"));
	format_help("-n warn,crit", "--nagios-mode-1 / --nagios-mode-2", gettext("Nagios-mode: return 1 when avg. response time >= warn, 2 if >= crit, otherwhise return 0"));
	format_help("-N x", NULL, gettext("Nagios mode 2: return 0 when all fine, 'x' when anything failes"));
	format_help("-y ip[:port]", "--bind-to", gettext("bind to ip-address (and thus interface) [/port]"));
	format_help("-q", NULL, gettext("quiet, only returncode"));
	format_help("-A", "--basic-auth", gettext("activate basic authentication"));
	format_help("-U username", NULL, gettext("needed for authentication"));
	format_help("-P password", NULL, gettext("needed for authentication"));
	format_help("-T x", NULL, gettext("read the password fom the file 'x' (replacement for -P)"));
	format_help("-C cookie=value", "--cookie", gettext("add a cookie to the request"));
	format_help("-Y", "--colors", gettext("add colors"));
	format_help("-E", NULL, gettext("fetch proxy settings from environment variables"));
	format_help("-K", "--ncurses / --gui", gettext("ncurses/GUI mode"));
	format_help("-D", "--no-graph", gettext("do not show graphs (in ncurses/GUI mode)"));
#ifdef TCP_TFO
	format_help("-F", NULL, gettext("\"TCP fast open\" (TFO), reduces the latency of TCP connects"));
#endif
	format_help("-v", NULL, gettext("verbose mode"));
	format_help("-V", NULL, gettext("show the version"));
	fprintf(stderr, gettext("\n"));
	format_help("-J", NULL, gettext("list long options"));
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
