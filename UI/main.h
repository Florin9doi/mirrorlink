#ifndef UPNP_H
#define UPNP_H

#include <arpa/inet.h>
#include <QThread>

typedef struct _RemoteClient {
    int sockFd;
    struct sockaddr_in group_addr;

    struct _RemoteClient *next;
} RemoteClient;

typedef struct _Context {
    RemoteClient *remoteClients;
} Context;

class UPnPThread : public QThread {
    Q_OBJECT
    void run() override;

signals:
    void resultReady(const QString &s);
};

#endif // UPNP_H
