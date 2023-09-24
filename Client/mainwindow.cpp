#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QUdpSocket>
#include <QMessageBox>
#include <QDateTime>
#include <QCryptographicHash>
QUdpSocket udpClient;
QHostAddress serverAddress("127.0.0.1"); // 服务器的IP地址
quint16 serverPort = 12345; // 服务器的端口
QStringList onlineUsers;

QString md5Hash(const QString& input) {
    QByteArray data = input.toUtf8(); // 将输入字符串转换为字节数组
    QByteArray hash = QCryptographicHash::hash(data, QCryptographicHash::Md5);
    return QString(hash.toHex());
}


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->receiverComboBox->clear();
    ui->receiverComboBox->addItem("所有人"); // 添加"所有人"选项
    ui->receiverComboBox->addItems(onlineUsers);
    udpClient.bind(QHostAddress::Any, 0); // 让系统自动分配一个可用端口
    connect(&udpClient, &QUdpSocket::readyRead, this, &MainWindow::receiveMessage);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::updateOnlineUserList() {
    ui->onlineUsersList->clear();
    for (const QString& user : onlineUsers) {
        ui->onlineUsersList->addItem(user);
    }
    ui->receiverComboBox->clear();
    ui->receiverComboBox->addItem("所有人"); // 添加"所有人"选项
    ui->receiverComboBox->addItems(onlineUsers);
}

void MainWindow::receiveMessage() {
    QByteArray response;
    while (udpClient.hasPendingDatagrams()) {
        response.resize(udpClient.pendingDatagramSize());
        udpClient.readDatagram(response.data(), response.size());
        QString responseMessage = QString(response);

        // 解析服务器广播消息
        QStringList responseParts = responseMessage.split("|");
        if (responseParts[0] == "MESSAGE") {
            QString username = responseParts[1];
            QString message = responseParts[2];
            QString displayMessage = username + ": " + message;
            QDateTime currentDateTime = QDateTime::currentDateTime();
            QString currentTimeString = "["+currentDateTime.toString("hh:mm:ss")+"]";
            ui->chatHistory->append(currentTimeString + " " + displayMessage);
        }
        else if (responseParts[0] == "PING") {
            udpClient.writeDatagram("PONG", serverAddress, serverPort);
        }
        else if (responseParts[0] == "ONLINEUSERS") {
            // 更新在线用户列表
            onlineUsers = responseParts.mid(1); // 从第二个元素开始是在线用户列表
            updateOnlineUserList();
        }
        else if (responseParts.size() == 2) {
            QString message = responseParts[1];
            QMessageBox::information(this,"通知",message);
        }

    }
}

void MainWindow::on_registerButton_clicked()
{
    serverAddress=QHostAddress(ui->serverAddress->text());
    QString username = ui->userName->text();
    QString password = ui->passWord->text();
    QString encryptedPassword = md5Hash(password);
    QString message = "REGISTER|" + username + "|" + encryptedPassword;
    QByteArray datagram = message.toUtf8();
    udpClient.writeDatagram(datagram, serverAddress, serverPort);
}

void MainWindow::on_loginButton_clicked()
{
    serverAddress=QHostAddress(ui->serverAddress->text());
    QString username = ui->userName->text();
    QString password = ui->passWord->text();
    QString encryptedPassword = md5Hash(password);
    QString message = "LOGIN|" + username + "|" + encryptedPassword;
    QByteArray datagram = message.toUtf8();
    udpClient.writeDatagram(datagram, serverAddress, serverPort);
}


void MainWindow::on_send_clicked()
{
    QString message = ui->textInput->toPlainText();
    QString receiver = ui->receiverComboBox->currentText(); // 获取选择的接收者
    if (receiver == "所有人") {
        receiver="ALL";
    }
    QByteArray datagram = "MESSAGETO|" + receiver.toUtf8() + "|" + message.toUtf8();
    udpClient.writeDatagram(datagram, serverAddress, serverPort);
    ui->textInput->clear();
}

