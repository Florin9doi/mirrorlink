#include <QDebug>

#include "../CoreStack/remote_server.h"
#include "mirrorlinkclient.h"
#include "threadmanager.h"

MirrorLinkClient::MirrorLinkClient(QObject *parent) :
	QObject(parent)
{
	m_server = 0;
	m_vnc_session = 0;
}

MirrorLinkClient::~MirrorLinkClient() {
	remote_server_destory(m_server);
}

void MirrorLinkClient::onStart(QString ip, quint16 port, QString path) {
	if (!m_server) {
		m_server = remote_server_create(ip.toStdString().c_str(), port, path.toStdString().c_str());
		/* remote_server_set_client_profile(m_server, 0); */
	}
	if (m_server) {
		char appFilter[] = "*";
		remote_server_get_application_list(m_server, 0, appFilter);
		App *nextApp = m_server->appList;
		while (nextApp) {
			App *app = nextApp;
			qDebug() << "App: " << app->appID << " " << app->name << " " << app->description
					 << " " << app->iconPath;
			emit addApp(app->appID, app->name, app->description);
			nextApp = app->next;
			free(app);
		}
	}
}

void MirrorLinkClient::onStop() {
	remote_server_destory(m_server);
	m_server = 0;
}

void MirrorLinkClient::onLaunch(quint32 appid) {
	if (m_server) {
		if (!remote_server_launch_application(m_server, appid, 0)) {
			process_app_launched(m_server, appid);
		}
	}
}

void MirrorLinkClient::process_app_launched(remote_server *server, quint32 appid) {
	(void) appid;
	qDebug() << "process_app_launched: " << server->vnc_ip << " " << server->vnc_port;
	int fd = vnc_session_start(server->vnc_ip, server->vnc_port);
	if (fd >= 0) {
		vnc_session_task(fd, NULL);
	}
}
