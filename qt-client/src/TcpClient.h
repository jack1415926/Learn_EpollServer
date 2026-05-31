#ifndef TCPCLIENT_H
#define TCPCLIENT_H

#include <QByteArray>
#include <QObject>
#include <QTcpSocket>

class TcpClient : public QObject
{
    Q_OBJECT

public:
    explicit TcpClient(QObject *parent = nullptr);

    bool isConnected() const;
    void connectToServer(const QString &host, quint16 port);
    void disconnectFromServer();
    void sendPing();

signals:
    void connected();
    void disconnected();
    void errorOccurred(const QString &message);
    void pingReplyReceived();
    void logMessage(const QString &message);

private slots:
    void onSocketConnected();
    void onSocketDisconnected();
    void onSocketReadyRead();
    void onSocketError(QAbstractSocket::SocketError socketError);

private:
    void appendLog(const QString &message);
    void tryConsumePackets();

    QTcpSocket m_socket;
    QByteArray m_recvBuffer;
};

#endif
