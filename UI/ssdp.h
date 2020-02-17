#ifndef SSDP_H
#define SSDP_H

#include <sys/socket.h>
#include "main.h"

int handle_ssdp_resp(int fd, struct sockaddr *addr, char *buffer);
int open_ssdp(Context* context, char* ifname);

#endif // SSDP_H
