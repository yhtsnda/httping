/* Released under GPLv2 with exception for the OpenSSL library. See license.txt */

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>

void error_exit(char *format, ...)
{
	va_list ap;

	va_start(ap, format);
	(void)vfprintf(stderr, format, ap);
	va_end(ap);

	fprintf(stderr, "\n\nerrno=%d which means %s (if applicable)\n", errno, strerror(errno));

	exit(1);
}
