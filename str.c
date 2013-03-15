/* Released under GPLv2 with exception for the OpenSSL library. See license.txt */

#include <string.h>
#include "error.h"
#include "utils.h"

/*
Most unixes have this function already.

#ifndef _GNU_SOURCE
char *strndup(char *in, int size)
{
	char *out = malloc(size + 1);

	memcpy(out, in, size);
	out[size] = 0x00;

	return out;
}
#endif
*/
