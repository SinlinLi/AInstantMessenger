
#include <iostream>
#include <main.h>

QMap<QString, QString> users; // 用于存储用户名和密码的映射
QMap<QPair<QString,quint16>, QString> onlineUsers;
QMap<QPair<QString, quint16>, QDateTime> lastHeartbeatTime;

// 账户信息持久化
void saveAccountInfo() {
    QFile file("accounts.txt");
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        for (const QString& username : users.keys()) {
            out << username << "|" << users[username] << Qt::endl;
        }
        file.close();
    }
}

QString timeStr()
{
    QTime currentTime = QTime::currentTime();
    QString currentTimeString = currentTime.toString("hh:mm:ss");
    currentTimeString="["+currentTimeString+"]";
    return currentTimeString;
}

// 用于读取账户信息的函数
void readAccountInfo() {
    QFile file("accounts.txt");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        while (!in.atEnd()) {
            QString line = in.readLine();
            QStringList parts = line.split("|");
            if (parts.size() == 2) {
                users[parts[0]] = parts[1];
            }
        }
        file.close();
    }
}

void printHelp() {
    qDebug().noquote() <<timeStr() << "可用命令:";
    qDebug().noquote() <<timeStr() << "help - 查看可用命令";
    qDebug().noquote() <<timeStr() << "list - 查看在线用户";
    qDebug().noquote() <<timeStr() << "disconnect <用户名> - 断开指定用户";
}

void listOnlineUsers() {
    qDebug().noquote() <<timeStr()<< "在线用户列表:";
    for (const QString& username : onlineUsers.values()) {
        qDebug().noquote() <<timeStr() << username;
    }
}

void disconnectUser(const QString& username) {
    if(onlineUsers.values().contains(username))
    {
        qDebug().noquote() <<timeStr()<< "已断开用户:" << username;
            for(const auto&ip:onlineUsers.keys())
        {
            if(onlineUsers[ip]==username)
            {
                onlineUsers.remove(ip);
            }
        }
    }
    else
    {
        qDebug().noquote() <<timeStr() << "未找到用户:" << username;;
    }
}

void MyServer::receiveMessage()
{
    if (udpServer.hasPendingDatagrams()) {
        datagram.resize(udpServer.pendingDatagramSize());
        udpServer.readDatagram(datagram.data(), datagram.size(), &clientAddress, &clientPort);

        qDebug().noquote() <<timeStr()<< "From:" << clientAddress.toString() << ":" << clientPort<< "Data: " << datagram;;

        // 解析客户端请求
        QString request = QString(datagram);
        QStringList parts = request.split("|");

        if (parts.size() >= 1) {
            QString action = parts[0];

            if (action == "REGISTER") {
                // 处理注册请求
                QString username = parts[1];
                QString password = parts[2];
                if (!users.contains(username)) {
                    users[username] = password;
                    saveAccountInfo();
                    udpServer.writeDatagram("RSUCCESS|Registered successfully.", clientAddress, clientPort);
                    qDebug().noquote() <<timeStr()<< "To:" << clientAddress.toString() << ":" << clientPort<<"Data: RSUCCESS|Registered successfully";
                } else {
                    udpServer.writeDatagram("RFAILURE|Username already exists.", clientAddress, clientPort);
                    qDebug().noquote() <<timeStr()<< "To:" << clientAddress.toString() << ":" << clientPort<<"Data: RFAILURE|Username already exists";
                }
            } else if (action == "LOGIN") {
                // 处理登录请求
                QString username = parts[1];
                QString password = parts[2];
                if (users.contains(username) && users[username] == password) {
                    udpServer.writeDatagram("LSUCCESS|Login successful.", clientAddress, clientPort);
                    qDebug().noquote() <<timeStr()<< "To:" << clientAddress.toString() << ":" << clientPort<< "Data: LSUCCESS|Login successful";
                    // 初始化心跳时间戳
                    lastHeartbeatTime[QPair<QString, quint16>(clientAddress.toString(), clientPort)] = QDateTime::currentDateTime();
                    onlineUsers[QPair<QString,quint16>(clientAddress.toString(),clientPort)] = username;
                } else {
                    udpServer.writeDatagram("LFAILURE|Invalid username or password.", clientAddress, clientPort);
                    qDebug().noquote() <<timeStr()<< "To:" << clientAddress.toString() << ":" << clientPort<<"Data: LFAILURE|Invalid username or password";
                }
            } else if (action == "MESSAGETO") {
                // 处理消息请求，发送给在线用户
                QString username = parts[1];
                QString message = parts[2];
                if(!onlineUsers.contains(QPair<QString,quint16>(clientAddress.toString(), clientPort)))
                {
                    udpServer.writeDatagram("ERROR|User not logged in",clientAddress, clientPort);
                    qDebug().noquote() <<timeStr()<< "To:" << clientAddress.toString() << ":" << clientPort <<"Data: "<<"ERROR|Unkown user";

                }
                else
                {
                    if(username=="ALL")
                    {
                        for (const auto& ip : onlineUsers.keys()) {
                            QHostAddress address(ip.first);
                            udpServer.writeDatagram(QString("MESSAGE|%1|%2").arg(onlineUsers[QPair<QString,quint16>(clientAddress.toString(), clientPort)]).arg(message).toUtf8(),
                                                    address, ip.second);
                            qDebug().noquote() <<timeStr()<< "To:" << ip.first << ":" << ip.second <<"Data: "<<QString("MESSAGE|%1|%2").arg(onlineUsers.value(ip)).arg(message).toUtf8();
                        }
                    }
                    else
                    {
                        for (const auto& ip : onlineUsers.keys()) {
                            if(onlineUsers[ip]==username||onlineUsers[ip]==onlineUsers[QPair<QString,quint16>(clientAddress.toString(), clientPort)])
                            {
                                QHostAddress address(ip.first);
                                udpServer.writeDatagram(QString("MESSAGE|%1|%2").arg(onlineUsers[QPair<QString,quint16>(clientAddress.toString(), clientPort)]).arg(message).toUtf8(),
                                                        address, ip.second);
                                qDebug().noquote() <<timeStr()<< "To:" << ip.first << ":" << ip.second <<"Data: "<<QString("MESSAGE|%1|%2").arg(onlineUsers.value(ip)).arg(message).toUtf8();
                            }

                        }
                    }
                }

            }
            else if (action == "PONG") {
                // 更新心跳时间戳
                lastHeartbeatTime[QPair<QString, quint16>(clientAddress.toString(), clientPort)] = QDateTime::currentDateTime();
            }
        }
    }
}

class InputThread : public QThread
{
public:
    void run() override
    {
        QTextStream input(stdin);
        QString command;
        QStringList args;
        while (!isInterruptionRequested())
        {
            QString line = input.readLine();
            QStringList parts = line.split(' ', Qt::SkipEmptyParts);
            if (!parts.isEmpty()) {
                command = parts[0].toLower(); // 将命令转换为小写

                if (command == "help") {
                    printHelp();
                } else if (command == "list") {
                    listOnlineUsers();
                } else if (command == "disconnect" && !parts[1].isEmpty()) {
                    disconnectUser(parts[1]);
                } else {
                    qDebug().noquote() <<timeStr() << "未知命令：" << line;
                }
            }
        }
    }
};


void MyServer::checkHeartbeat() {
    QDateTime currentTime = QDateTime::currentDateTime();

    for (auto it = lastHeartbeatTime.begin(); it != lastHeartbeatTime.end();) {
        const QDateTime& lastHeartbeat = it.value();

        // 计算时间差
        qint64 secondsSinceLastHeartbeat = lastHeartbeat.secsTo(currentTime);

        // 如果用户超过5秒未响应，则移除用户
        if (secondsSinceLastHeartbeat > 10) {
            const QPair<QString, quint16>& userKey = it.key();
            qDebug().noquote() <<timeStr() << "DISCONNECTED：" << onlineUsers.value(userKey);
                                                                        it = lastHeartbeatTime.erase(it);
            onlineUsers.remove(userKey);
        } else {
            ++it;
        }
    }
    QStringList onlineUsersList;
    for (const QString& username : onlineUsers.values()) {
        onlineUsersList << username;
    }

    QString userListMessage = "ONLINEUSERS|" + onlineUsersList.join("|");
    // 向所有在线客户端发送在线用户列表
    for (const auto& user : onlineUsers.keys()) {
        QHostAddress address(user.first);
        udpServer.writeDatagram(userListMessage.toUtf8(), address, user.second);
        qDebug().noquote() <<timeStr()<< "To:" << user.first << ":" << user.second <<"Data: "<<userListMessage.toUtf8();
    }
    // 定期发送PING消息
    for (const auto& user : onlineUsers.keys()) {
        QHostAddress address(user.first);
        udpServer.writeDatagram("PING", address, user.second);
        qDebug().noquote() <<timeStr()<< "To:" << user.first << ":" << user.second <<"Data: "<<"PING";
    }
}
MyServer::MyServer()
{
    SetConsoleTitle(L"Server");
    udpServer.bind(QHostAddress::Any, 12345); // 服务器监听的端口
    connect(&udpServer, SIGNAL(readyRead()), this, SLOT(receiveMessage()));
    QObject::connect(&heartbeatTimer, &QTimer::timeout, this, &MyServer::checkHeartbeat);
    heartbeatTimer.start(5000);  // 每5秒检查一次
}
int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    readAccountInfo();
    qDebug().noquote() <<timeStr()<< "UDP Server is running on port 12345...";
    MyServer server;

    InputThread thread;
    thread.start();
    return a.exec();
}
