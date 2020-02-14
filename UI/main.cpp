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
#include "main.h"

#define TAG "[UPnP]"


////////////////////////////////
/// CLIENTS
////////////////////////////////

RemoteClient *add_client(Context *context, int sockFd, struct sockaddr_in group_addr) {
    printf("add_client = %d\n", sockFd);
    RemoteClient *rc = (RemoteClient*) calloc(1, sizeof(RemoteClient));
    rc->sockFd = sockFd;
    rc->group_addr = group_addr;
    if (context->remoteClients == NULL) {
        context->remoteClients = rc;
    } else {
        RemoteClient *i = context->remoteClients;
        while (i->next != NULL) {
            i = i->next;
        }
        i->next = rc;
    }
    return rc;
}

int remove_client(Context *context, int sockFd) {
    printf("remove_client = %d\n", sockFd);
    int addrlen = sizeof(struct sockaddr_in);
    struct sockaddr_in client_address;
    getpeername(sockFd, (struct sockaddr*)&client_address, (socklen_t*)&addrlen);
    printf("Remove client (fd:%d): %s:%d\n", sockFd, inet_ntoa(client_address.sin_addr) , ntohs(client_address.sin_port));

    if (context->remoteClients == NULL) return -1;
    RemoteClient *last, *i;
    for (last = NULL, i = context->remoteClients; i != NULL; last = i, i = i->next) {
        if (i->sockFd == sockFd) {
            if (last == NULL) {
                context->remoteClients = i->next;
            } else {
                last->next = i->next;
            }
            close(i->sockFd);
            free(i);
            i = NULL;
            return 0;
        }
    }
    return -1;
}


////////////////////////////////
/// SSDP
////////////////////////////////

#define TM_TAS "urn:schemas-upnp-org:service:TmApplicationServer:1"
#define TM_TCP "urn:schemas-upnp-org:service:TmClientProfile:1"
#define TM_TSD "urn:schemas-upnp-org:device:TmServerDevice:1"

int create_ssdp_message(char *out, char *searchTarget) {
     int len = 0;
     len += sprintf(out + len, "M-SEARCH * HTTP/1.1\r\n");
     len += sprintf(out + len, "HOST: 239.255.255.250:1900\r\n");
     len += sprintf(out + len, "MAN: \"ssdp:discover\"\r\n");
     len += sprintf(out + len, "MX: 1\r\n");
     len += sprintf(out + len, "ST: %s\r\n", searchTarget);
     len += sprintf(out + len, "\r\n");
     return len;
}

int send_ssdp(int fd, struct sockaddr *addr, char *searchTarget) {
     char buffer[1024] = { 0 };
     int buf_len = create_ssdp_message(buffer, searchTarget);

//     qDebug() << TAG << "sending: " << buffer;
     int ret = sendto(fd, buffer, buf_len, 0, addr, sizeof(struct sockaddr_in));
     if (ret < 0) {
          perror("sendto");
          return 1;
     }
     return 0;
}

int handle_ssdp_resp(int fd, struct sockaddr *addr, char *buffer) {
     char *ptr = NULL;
     int found = 0;
     char location[1024] = {0};
     char st[1024] = {0};

     ptr = strtok(buffer, "\n");
     while (ptr) {
          if (strstr(ptr, "ST: ")) {
               sscanf(ptr, "ST: %s", st);
               if (!strcmp(st, TM_TAS)) {
                    found = 1;
               }
          } else if (strstr(ptr, "Location: ")) {
               sscanf(ptr, "Location: %s", location);
          }
          ptr = strtok(NULL, "\n");
     }

     if (found) {
          qDebug() << TAG << " ### " << location;
          qDebug() << TAG << " ### " << st;
     }
     return 0;
}

int open_ssdp(Context* context, char* ifname) {
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

     if (setsockopt(fd, SOL_SOCKET, SO_BINDTODEVICE, ifname, strlen(ifname)) < 0) {
          perror("SO_BINDTODEVICE");
          exit(1);
     };

     struct sockaddr_in group_addr;
     memset(&group_addr, 0, sizeof(struct sockaddr_in));
     group_addr.sin_family      = AF_INET;
     group_addr.sin_addr.s_addr = inet_addr("239.255.255.250");
     group_addr.sin_port        = htons(1900);

     send_ssdp(fd, (struct sockaddr *) &group_addr, TM_TAS);
     usleep(300);
     send_ssdp(fd, (struct sockaddr *) &group_addr, TM_TCP);
     usleep(300);
     send_ssdp(fd, (struct sockaddr *) &group_addr, TM_TSD);

     add_client(context, fd, group_addr);
}


////////////////////////////////
/// NETLINK
////////////////////////////////

int open_netlink_socket() {
    int netlink_socket = -1;
    if ((netlink_socket = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE)) < 0) {
        qDebug() << TAG << "netlink socket error";
        return -1;
    }

    struct sockaddr_nl addr;
    memset(&addr, 0, sizeof(addr));
    addr.nl_family = AF_NETLINK;
    addr.nl_groups = /*RTMGRP_LINK |*/ RTMGRP_IPV4_IFADDR /*| RTMGRP_IPV6_IFADDR*/;
    if (bind(netlink_socket, (struct sockaddr *)&addr, sizeof(struct sockaddr_nl)) < 0) {
        qDebug() << TAG << "netlink bind error";
        return -1;
    }
    return netlink_socket;
}

int on_netlink_event(Context *context, int sockint) {
    int len;
    char buf[4096];
    struct sockaddr_nl snl;
    struct iovec iov = { buf, sizeof(buf) };
    struct msghdr msg = { &snl, sizeof(snl), &iov, 1, NULL, 0, 0};
    if((len = recvmsg(sockint, &msg, 0)) < 0) {
        if (errno == EWOULDBLOCK || errno == EAGAIN) {
            return -1;
        }
        qDebug() << TAG << "recvmsg error: " << len;
        return -1;
    }

    struct nlmsghdr *nlh;
    for (nlh = (struct nlmsghdr *) buf;
            NLMSG_OK (nlh, len) && nlh->nlmsg_type != NLMSG_DONE && nlh->nlmsg_type != NLMSG_ERROR;
            nlh = NLMSG_NEXT (nlh, len)) {

        struct ifinfomsg *ifi = (struct ifinfomsg*) NLMSG_DATA(nlh);
        struct ifaddrmsg *ifa = (struct ifaddrmsg*) NLMSG_DATA(nlh);
        char ifname[IFNAMSIZ];
        switch (nlh->nlmsg_type) {
            case RTM_NEWLINK:
                if_indextoname(ifa->ifa_index, ifname);
                qDebug() << TAG << "msg_handler: RTM_NEWLINK : " << ifname << " "
                         << ((ifi->ifi_flags & IFF_UP) ? "Up" : "Down");
                break;
            case RTM_NEWADDR:
                if_indextoname(ifi->ifi_index, ifname);
                qDebug() << TAG << "msg_handler: RTM_NEWADDR : " << ifname;
                open_ssdp(context, ifname);
                break;
            case RTM_DELADDR:
                if_indextoname(ifi->ifi_index, ifname);
                qDebug() << TAG << "msg_handler: RTM_DELADDR : " << ifname;
                break;
            case RTM_DELLINK:
                if_indextoname(ifa->ifa_index,ifname);
                qDebug() << TAG << "msg_handler: RTM_DELLINK : " << ifname;
                break;
            default:
                qDebug() << TAG << "msg_handler: Unknown netlink nlmsg_type : " << nlh->nlmsg_type;
                break;
        }
    }
    return 0;
}

void UPnPThread::run() {
    Context *context = (Context*) calloc(1, sizeof (Context));

    int netlink_socket = open_netlink_socket();

    // check if mirrorlink is already started
    struct ifaddrs *addrs, *tmp;
    getifaddrs(&addrs);
    tmp = addrs;
    while (tmp) {
        if (tmp->ifa_addr && tmp->ifa_addr->sa_family == AF_PACKET) {
            if (tmp->ifa_flags & IFF_MULTICAST && strcmp(tmp->ifa_name, "enp0s20f0u2") == 0) {
                open_ssdp(context, tmp->ifa_name);
            }
        }
        tmp = tmp->ifa_next;
    }
    freeifaddrs(addrs);


    fd_set readfds;
    RemoteClient *rc;

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(netlink_socket, &readfds);
        int max_fd = netlink_socket;

        for (rc = context->remoteClients; rc != NULL; rc = rc->next) {
            FD_SET(rc->sockFd, &readfds);
            max_fd = rc->sockFd > max_fd ? rc->sockFd : max_fd;
        }

        struct timeval timeout = {0, 10*1000};   // 10 ms
        if ((select(max_fd + 1, &readfds, NULL, NULL, &timeout) < 0) && (errno != EINTR)) {
            printf("select error");
        }

        if (FD_ISSET(netlink_socket, &readfds)) {
            on_netlink_event(context, netlink_socket);
        }

        for (rc = context->remoteClients; rc != NULL; rc = rc->next) {
            if (FD_ISSET(rc->sockFd, &readfds)) {
                char buffer[1025];
                int read_size = read(rc->sockFd, buffer, 1024);
                if (read_size == 0) {
                    remove_client(context, rc->sockFd);
                } else {
                    printf("message = \"%s\"\n", buffer);
                    handle_ssdp_resp(rc->sockFd, (struct sockaddr *) &rc->group_addr, buffer);
                }
            }
        }
    }

    return;
}
