#include "TcpClient.h"

#include "Protocol.h"

#include "../shared/protocol_types.h"

#include <QNetworkProxy>

TcpClient::TcpClient(QObject *parent)
    : QObject(parent)
{
    // Windows 上若走系统/环境变量代理，QTcpSocket 直连会报：
    // "The proxy type is invalid for this operation"
    m_socket.setProxy(QNetworkProxy::NoProxy);

    connect(&m_socket, &QTcpSocket::connected, this, &TcpClient::onSocketConnected);
    connect(&m_socket, &QTcpSocket::disconnected, this, &TcpClient::onSocketDisconnected);
    connect(&m_socket, &QTcpSocket::readyRead, this, &TcpClient::onSocketReadyRead);
    connect(&m_socket, &QTcpSocket::errorOccurred, this, &TcpClient::onSocketError);
}

bool TcpClient::isConnected() const
{
    return m_socket.state() == QAbstractSocket::ConnectedState;
}

void TcpClient::connectToServer(const QString &host, quint16 port)
{
    if (isConnected()) {
        appendLog(QStringLiteral("已连接，请先断开"));
        return;
    }
    m_recvBuffer.clear();
    appendLog(QStringLiteral("正在连接 %1:%2 ...").arg(host).arg(port));
    m_socket.connectToHost(host, port);
}

void TcpClient::disconnectFromServer()
{
    if (m_socket.state() == QAbstractSocket::UnconnectedState) {
        return;
    }
    m_socket.disconnectFromHost();
    if (m_socket.state() != QAbstractSocket::UnconnectedState) {
        m_socket.waitForDisconnected(3000);
    }
    m_recvBuffer.clear();
}

void TcpClient::sendPing()
{
    if (!isConnected()) {
        emit errorOccurred(QStringLiteral("未连接，无法发送 Ping"));
        return;
    }

    const QByteArray packet = Protocol::buildPingPacket();
    const qint64 sent = m_socket.write(packet);
    if (sent != packet.size()) {
        emit errorOccurred(QStringLiteral("Ping 发送失败"));
        return;
    }
    m_socket.flush();
    appendLog(QStringLiteral("发送 Ping: %1").arg(Protocol::toHexPreview(packet)));
}

void TcpClient::onSocketConnected()
{
    appendLog(QStringLiteral("TCP 连接成功"));
    emit connected();
}

void TcpClient::onSocketDisconnected()
{
    m_recvBuffer.clear();
    appendLog(QStringLiteral("TCP 连接已断开"));
    emit disconnected();
}

void TcpClient::onSocketReadyRead()
{
    m_recvBuffer.append(m_socket.readAll());
    tryConsumePackets();
}

void TcpClient::onSocketError(QAbstractSocket::SocketError /*socketError*/)
{
    if (m_socket.state() == QAbstractSocket::ConnectedState) {
        return;
    }
    emit errorOccurred(m_socket.errorString());
}

void TcpClient::appendLog(const QString &message)
{
    emit logMessage(message);
}

void TcpClient::tryConsumePackets()
{
    while (m_recvBuffer.size() >= PKG_HEADER_SIZE) {
        const QByteArray chunk = m_recvBuffer.left(PKG_HEADER_SIZE);
        QString error;
        if (!Protocol::parsePingResponse(chunk, &error)) {
            appendLog(QStringLiteral("收到数据: %1（解析: %2）")
                          .arg(Protocol::toHexPreview(m_recvBuffer))
                          .arg(error));
            m_recvBuffer.clear();
            emit errorOccurred(error);
            return;
        }

        m_recvBuffer.remove(0, PKG_HEADER_SIZE);
        appendLog(QStringLiteral("收到 Ping 响应: %1").arg(Protocol::toHexPreview(chunk)));
        emit pingReplyReceived();
    }
}
