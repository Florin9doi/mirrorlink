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
#include "ssdp.h"

#define TAG "[Main]"


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

    if (bind(netlink_socket, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
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


////////////////////////////////
/// Multicast Receiver
////////////////////////////////

int open_multicast_socket() {
    int multicast_socket;
    if ((multicast_socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        qDebug() << TAG << "multicast socket error";
        return -1;
    }

    int reuse = 1;
    if (setsockopt(multicast_socket, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        perror("multicast reuseaddr error");
        return -1;
    }

    struct sockaddr_in group_addr;
    memset(&group_addr, 0, sizeof(group_addr));
    group_addr.sin_family      = AF_INET;
    group_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    group_addr.sin_port        = htons(1900);

    if (bind(multicast_socket, (struct sockaddr *)&group_addr, sizeof(group_addr)) < 0) {
        qDebug() << TAG << "multicast bind error";
        return -1;
    }

    struct ip_mreq membership;
    memset(&membership, 0, sizeof(membership));
    membership.imr_multiaddr.s_addr = inet_addr("239.255.255.250");
    membership.imr_interface.s_addr = htonl(INADDR_ANY);

	if (setsockopt(multicast_socket, IPPROTO_IP, IP_ADD_MEMBERSHIP, &membership, sizeof(membership)) < 0) {
		qDebug() << TAG << "multicast add membership error";
		return -1;
	}

	return multicast_socket;
}


////////////////////////////////
/// main loop
////////////////////////////////

void UPnPThread::run() {
    Context *context = (Context*) calloc(1, sizeof (Context));

    int netlink_socket = open_netlink_socket();
    int multicast_socket = open_multicast_socket();

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

        FD_SET(multicast_socket, &readfds);
        max_fd = multicast_socket > max_fd ? multicast_socket : max_fd;

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

        if (FD_ISSET(multicast_socket, &readfds)) {
            char buffer[1025];
            struct sockaddr_in from_addr;
            unsigned int from_len = sizeof(from_addr);
            int read_size = recvfrom(multicast_socket, buffer, 1024, 0, (struct sockaddr*)&from_addr, &from_len);
            if (read_size > 0) {
                buffer[read_size] = 0;
                qDebug() << TAG << "multicast_socket: " << inet_ntoa(from_addr.sin_addr)
                         << " : " << read_size
                         << " : " << buffer;
            }
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
