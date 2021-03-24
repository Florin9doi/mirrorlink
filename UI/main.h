#ifndef UPNP_H
#define UPNP_H

#include <QThread>
#include <arpa/inet.h>

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


RemoteClient *add_client(Context *context, int sockFd, struct sockaddr_in group_addr);
int remove_client(Context *context, int sockFd);

#endif // UPNP_H
