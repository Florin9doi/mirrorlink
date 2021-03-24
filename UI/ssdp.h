#ifndef SSDP_H
#define SSDP_H

#include <sys/socket.h>
#include "main.h"

int handle_ssdp_resp(int fd, struct sockaddr *addr, char *buffer);
int open_ssdp(Context* context, struct in_addr *addr);

#endif // SSDP_H
