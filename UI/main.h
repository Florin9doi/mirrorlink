#ifndef UPNP_H
#define UPNP_H

#include <QThread>
#include <arpa/inet.h>
#include <arpa/inet.h>
#include <remote_server.h>

typedef struct _RemoteClient {
    int sockFd;
    struct sockaddr_in group_addr;

    struct _RemoteClient *next;
} RemoteClient;

typedef struct _Context {
    int serverState = 0;

    RemoteClient *remoteClients;
    char tmApplicationServer[1024];
    char tmClientProfile[1024];
    char tmServerDevice[1024];

    remote_server *m_server;
} Context;

class UPnPThread : public QThread {
    Q_OBJECT
    void run() override;

signals:
    void resultReady(const QString &s);
    void addApp(int appID, char *name, char *description);
};


RemoteClient *add_client(Context *context, int sockFd, struct sockaddr_in group_addr);
int remove_client(Context *context, int sockFd);

#endif // UPNP_H
