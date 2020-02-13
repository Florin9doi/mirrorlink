#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    usbThread = new USBThread();
    usbThread->start();
}

MainWindow::~MainWindow()
{
    delete ui;
}
