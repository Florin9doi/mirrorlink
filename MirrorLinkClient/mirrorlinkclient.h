#ifndef MIRRORLINKCLIENT_H
#define MIRRORLINKCLIENT_H

#include <QObject>
#include "../CoreStack/remote_server.h"
#include "../CoreStack/vnc_session.h"

class MirrorLinkClient : public QObject
{
	Q_OBJECT
public:
	explicit MirrorLinkClient(QObject *parent = 0);
	~MirrorLinkClient();

signals:
	void addApp(int appID, char *name, char *description);

public slots:
	void onStart(QString ip, quint16 port, QString path);
	void onStop();
	void onLaunch(quint32 appid);

private:
	void process_app_launched(remote_server *server, quint32 appid);
	remote_server *m_server;
	vnc_session *m_vnc_session;
};

#endif // MIRRORLINKCLIENT_H
