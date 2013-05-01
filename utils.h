/* Released under GPLv2 with exception for the OpenSSL library. See license.txt */
/* $Revision$ */

double get_ts(void);

void split_string(const char *in, const char *split, char ***list, int *list_n);
void free_splitted_string(char **list, int n);

void str_add(char **to, const char *what, ...);

#define min(x, y)	((x) < (y) ? (x) : (y))
#define max(x, y)	((x) > (y) ? (x) : (y))
