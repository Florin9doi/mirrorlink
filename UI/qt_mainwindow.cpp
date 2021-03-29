#include "qt_mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

	appListModel = new QStandardItemModel();
	ui->AppListView->setModel(appListModel);

    usbThread = new USBThread();
    upnpThread = new UPnPThread();

	// signals
	connect(upnpThread, &UPnPThread::addApp, this, &MainWindow::on_addApp);
	//connect(ui->AppListView, &QListView::doubleClicked, this, &MainWindow::on_viewDoubleClick);

	// test : to be removed
	QStandardItem *item = new QStandardItem(QString("TEST"));
	item->setIcon(QIcon("../../icon.png"));
	item->setData(3);
	appListModel->appendRow(item);

    usbThread->start();
    upnpThread->start();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_addApp(int appID, char *name, char *description) {
	QStandardItem *item = new QStandardItem(QString(name));
	item->setToolTip(description);
	item->setIcon(QIcon("../../icon.png"));
	item->setData(appID);
	appListModel->appendRow(item);
}
