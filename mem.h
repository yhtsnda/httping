/* Released under GPLv2 with exception for the OpenSSL library. See license.txt */

void * mymalloc(int size, char *what);
void * myrealloc(void *oldp, int newsize, char *what);
void myfree(void *p);
char * mystrdup(char *in, char *what);
