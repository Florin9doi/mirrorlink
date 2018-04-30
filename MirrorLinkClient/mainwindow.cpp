#include <QDebug>
#include <QUrl>

#include "mainwindow.h"
#include "ui_mainwindow.h"


MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow)
{
	ui->setupUi(this);
	appListModel = new QStandardItemModel();
	ui->AppListView->setModel(appListModel);

	// signals
	connect(&m_manager, &ThreadManager::addApp, this, &MainWindow::on_addApp);
	connect(ui->AppListView, &QListView::doubleClicked, this, &MainWindow::on_viewDoubleClick);

	// test : to be removed
	QStandardItem *item = new QStandardItem(QString("TEST"));
	item->setIcon(QIcon("../icon.png"));
	item->setData(3);
	appListModel->appendRow(item);
}

MainWindow::~MainWindow() {
	delete ui;
}

void MainWindow::on_startButton_clicked() {
	appListModel->clear();
	QString t = ui->locLineEdit->text();
	QUrl url(t);
	m_manager.startMirrorLinkClient(url.host(), url.port(), url.path());
}

void MainWindow::on_stopButton_clicked() {
	m_manager.stopMirrorLinkClient();
}

void MainWindow::on_launchButton_clicked() {
}


void MainWindow::on_addApp(int appID, char *name, char *description) {
	QStandardItem *item = new QStandardItem(QString(name));
	item->setToolTip(description);
	item->setIcon(QIcon("../icon.png"));
	item->setData(appID);
	appListModel->appendRow(item);
}

void MainWindow::on_viewDoubleClick(const QModelIndex& idx) {
	int appID = appListModel->item(idx.row(), idx.column())->data().toInt();
	m_manager.launchApp(appID);
}
