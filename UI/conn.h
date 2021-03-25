#ifndef _CONN_H
#define _CONN_H

#include <stdint.h>
#include "buffer.h"

int conn_open(const char *ip, uint16_t port);

void conn_close(int fd);

int conn_read(int fd, char *buf, uint32_t len);

int conn_read_all(int fd, buffer *buf);

int conn_write(int fd, char *buf, uint32_t len);

#endif

