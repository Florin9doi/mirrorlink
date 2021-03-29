#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStandardItemModel>
#include "usb.h"
#include "main.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

public slots:
    void on_addApp(int appID, char *name, char *description);

private:
    Ui::MainWindow *ui;
    USBThread *usbThread;
    UPnPThread *upnpThread;

    QStandardItemModel *appListModel;
};

#endif // MAINWINDOW_H
