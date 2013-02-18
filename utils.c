/* Released under GPLv2 with exception for the OpenSSL library. See license.txt */

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include "error.h"

double get_ts(void)
{
	struct timeval ts;
       	struct timezone tz;

	if (gettimeofday(&ts, &tz) == -1)
		error_exit("gettimeofday failed");

	return (((double)ts.tv_sec) + ((double)ts.tv_usec)/1000000.0);
}
