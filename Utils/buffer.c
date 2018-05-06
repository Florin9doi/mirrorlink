#include "buffer.h"

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

void buffer_init(buffer *buf, uint32_t size)
{
	memset(buf, 0, sizeof(*buf));
	if (size) {
		buf->buf = (uint8_t *)calloc(1, size);
		buf->size = size;
	}
}

void buffer_clear(buffer *buf)
{
	free(buf->buf);
	buf->buf = 0;
	buf->size = 0;
	buf->len = 0;
}

void buffer_append(buffer *buf, int size)
{
	if (size + buf->len > buf->size) {
		uint8_t *b = (uint8_t *)calloc(1, (size + buf->len) * sizeof(*b));
		memcpy(b, buf->buf, buf->len);
		free(buf->buf);
		buf->buf = b;
		buf->size = size + buf->len;
	}
}

