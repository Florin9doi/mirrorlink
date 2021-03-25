#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <asm/types.h>
#include <ifaddrs.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <QDebug>
#include "ssdp.h"

#define TAG "[SSDP]"

////////////////////////////////
/// SSDP
////////////////////////////////

const char* TM_TAS = "urn:schemas-upnp-org:service:TmApplicationServer:1";
const char* TM_TCP = "urn:schemas-upnp-org:service:TmClientProfile:1";
const char* TM_TSD = "urn:schemas-upnp-org:device:TmServerDevice:1";

int create_ssdp_message(char *out, const char *searchTarget) {
     int len = 0;
     len += sprintf(out + len, "M-SEARCH * HTTP/1.1\r\n");
     len += sprintf(out + len, "HOST: 239.255.255.250:1900\r\n");
     len += sprintf(out + len, "MAN: \"ssdp:discover\"\r\n");
     len += sprintf(out + len, "MX: 1\r\n");
     len += sprintf(out + len, "ST: %s\r\n", searchTarget);
     len += sprintf(out + len, "\r\n");
     return len;
}

int send_ssdp(int fd, struct sockaddr *addr, const char *searchTarget) {
     char buffer[1024] = { 0 };
     int buf_len = create_ssdp_message(buffer, searchTarget);

     qDebug() << TAG << "sending: " << buffer;
     int ret = sendto(fd, buffer, buf_len, 0, addr, sizeof(struct sockaddr_in));
     if (ret < 0) {
          perror("sendto");
          return 1;
     }
     return 0;
}

int handle_ssdp_resp(Context* context, int fd, struct sockaddr *addr, char *buffer) {
     char *ptr = NULL;
     int found = 0;
     char st[1024] = {0};
     char location[1024] = {0};

     ptr = strtok(buffer, "\n");
     while (ptr) {
          if (strstr(ptr, "ST: ")) {
               sscanf(ptr, "ST: %s", st);
          } else if (strstr(ptr, "Location: ")) {
               sscanf(ptr, "Location: %s", location);
          }
          ptr = strtok(NULL, "\n");
     }

     if (strcmp(st, TM_TAS) == 0) {
        found = 1;
        strcpy(context->tmApplicationServer, location);
     }
     if (strcmp(st, TM_TCP) == 0) {
        found = 1;
        strcpy(context->tmClientProfile, location);
     }
     if (strcmp(st, TM_TSD) == 0) {
        found = 1;
        strcpy(context->tmServerDevice, location);
    }
    if (found) {
        qDebug() << TAG << " ### " << st;
        qDebug() << TAG << " # " << location;
    }
    return 0;
}

int open_ssdp(Context* context, struct in_addr *addr) {
     int fd = socket(AF_INET, SOCK_DGRAM, 0);
     if (fd < 0) {
          perror("socket");
          exit(1);
     }

     int reuse = 1;
     if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
          perror("SO_REUSEADDR");
          exit(1);
     }

     struct timeval tv;
     tv.tv_sec = 1;
     tv.tv_usec = 0;
     if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0 ) {
          perror("SO_RCVTIMEO");
          exit(1);
     }

//     if (setsockopt(fd, SOL_SOCKET, SO_BINDTODEVICE, ifname, strlen(ifname)) < 0) {
//          perror("SO_BINDTODEVICE");
//          exit(1);
//     };

     struct sockaddr_in group_addr_bind;
     memset(&group_addr_bind, 0, sizeof(group_addr_bind));
     group_addr_bind.sin_family      = AF_INET;
     group_addr_bind.sin_addr.s_addr = INADDR_ANY;
     group_addr_bind.sin_port        = htons(1911);

     if (bind(fd, (struct sockaddr *)&group_addr_bind, sizeof(group_addr_bind)) < 0) {
         qDebug() << TAG << "multicast bind error";
         return -1;
     }

     char loopch = 0;
     if (setsockopt(fd, IPPROTO_IP, IP_MULTICAST_LOOP, &loopch, sizeof(loopch)) < 0) {
         perror("IP_MULTICAST_LOOP");
         exit(1);
     }

//     struct ip_mreq imr;
//     struct sockaddr_in *sin = (struct sockaddr_in *)addr;
//     imr.imr_interface        = sin->sin_addr;
//     imr.imr_multiaddr.s_addr = inet_addr("239.255.255.250");
//     if (setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &imr, sizeof(imr))) {
//         perror("IP_ADD_MEMBERSHIP");
//         exit(1);
//     }

     struct in_addr if_addr = *addr;
     if(setsockopt(fd, IPPROTO_IP, IP_MULTICAST_IF, &if_addr, sizeof(if_addr)) < 0) {
         perror("IP_MULTICAST_IF");
         exit(1);
     }

     struct sockaddr_in group_addr;
     memset(&group_addr, 0, sizeof(group_addr));
     group_addr.sin_family      = AF_INET;
     group_addr.sin_addr.s_addr = inet_addr("239.255.255.250");
     group_addr.sin_port        = htons(1900);

     send_ssdp(fd, (struct sockaddr *) &group_addr, TM_TAS);
     usleep(300);
     send_ssdp(fd, (struct sockaddr *) &group_addr, TM_TCP);
     usleep(300);
     send_ssdp(fd, (struct sockaddr *) &group_addr, TM_TSD);

     add_client(context, fd, group_addr);
     return 0;
}
