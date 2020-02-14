#include "qt_mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    usbThread = new USBThread();
    usbThread->start();
    upnpThread = new UPnPThread();
    upnpThread->start();
}

MainWindow::~MainWindow()
{
    delete ui;
}
