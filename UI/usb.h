#ifndef USB_H
#define USB_H

#include <QThread>

class USBThread : public QThread {
    Q_OBJECT
    void run() override;

signals:
    void resultReady(const QString &s);
};

#endif // USB_H
