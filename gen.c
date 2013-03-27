#include <math.h>
#include <string.h>

#include "gen.h"

double calc_sd(stats_t *in)
{
	double avg = 0.0;

	if (in -> n == 0)
		return 0;

	avg = in -> avg / (double)in -> n;

	return sqrt((in -> sd / (double)in -> n) - pow(avg, 2.0));
}

/* Base64 encoding start */  
const char *alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

void encode_tryptique(char source[3], char result[4])
/* Encode 3 char in B64, result give 4 Char */
 {
    int tryptique, i;
    tryptique = source[0];
    tryptique *= 256;
    tryptique += source[1];
    tryptique *= 256;
    tryptique += source[2];
    for (i=0; i<4; i++)
    {
 	result[3-i] = alphabet[tryptique%64];
	tryptique /= 64;
    }
} 

int enc_b64(char *source, int source_lenght, char *target)
{
	/* Divide string /3 and encode trio */
	while (source_lenght >= 3) {
		encode_tryptique(source, target);
		source_lenght -= 3;
		source += 3;
		target += 4;
	}
	/* Add padding to the rest */
	if (source_lenght > 0) {
		char pad[3];
	 	memset(pad, 0, sizeof pad);
		memcpy(pad, source, source_lenght);
		encode_tryptique(pad, target);
		target[3] = '=';
		if (source_lenght == 1) target[2] = '=';
		target += 4;
	}
	target[0] = 0;
	return 1;
} 
/* Base64 encoding END */
