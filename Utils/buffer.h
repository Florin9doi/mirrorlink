#ifndef _BUFFER_H
#define _BUFFER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _buffer {
	uint8_t *buf;
	uint32_t size;
	uint32_t len;
} buffer;

void buffer_init(buffer *buf, uint32_t size);

void buffer_clear(buffer *buf);

void buffer_append(buffer *buf, int size);

#ifdef __cplusplus
}
#endif

#endif
