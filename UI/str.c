#include "str.h"

#include <string.h>
#include <stdlib.h>
#include <stdint.h>

void str_append(str_t *str, const char *tail)
{
	uint32_t lo = *str ? strlen(*str):0;
	uint32_t ln = strlen(tail);
	if (ln) {
		char *buf = calloc(1, lo + ln + 1);
		if (lo) {
			strcpy(buf, *str);
		}
		strcpy(buf + lo, tail);
		if (*str) {
			free(*str);
		}
		*str = buf;
	}
}

