#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <qstandarditemmodel.h>

#include "threadmanager.h"

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
	void on_viewDoubleClick(const QModelIndex& idx);

private slots:
	void on_startButton_clicked();
	void on_stopButton_clicked();
	void on_launchButton_clicked();

private:
	Ui::MainWindow *ui;
	QStandardItemModel *appListModel;
	ThreadManager m_manager;
};

#endif // MAINWINDOW_H
