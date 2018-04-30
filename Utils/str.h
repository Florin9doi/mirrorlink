#ifndef _STR_H
#define _STR_H

#ifdef __cplusplus
extern "C" {
#endif

typedef char* str_t;

void str_append(str_t *str, const char *tail);

#ifdef __cplusplus
}
#endif

#endif
