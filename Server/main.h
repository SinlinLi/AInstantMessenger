#ifndef MAIN_H
#define MAIN_H
#include <QCoreApplication>
#include <QUdpSocket>
#include <QDebug>
#include <QFile>
#include <QDateTime>
#include <QTextStream>
#include <QMap>
#include <QTimer>
#include <QThread>
#include <Windows.h>
#endif // MAIN_H
class MyServer : public QObject {
    Q_OBJECT

public:
    MyServer();


public slots:
    void receiveMessage();
    void checkHeartbeat();
private:
    QUdpSocket udpServer;
    QByteArray datagram;
    QHostAddress clientAddress;
    quint16 clientPort;
    QTimer heartbeatTimer;
};



