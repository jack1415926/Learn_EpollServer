#include "MainWindow.h"

#include "TcpClient.h"

#include <QCheckBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QStatusBar>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QWidget>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_client(new TcpClient(this))
{
    setWindowTitle(QStringLiteral("Epoll Server Qt Client (MVP)"));
    resize(720, 520);

    auto *central = new QWidget(this);
    auto *rootLayout = new QVBoxLayout(central);

    auto *formLayout = new QFormLayout();
    m_hostEdit = new QLineEdit(QStringLiteral("127.0.0.1"), this);
    m_portEdit = new QLineEdit(QStringLiteral("8080"), this);
    m_portEdit->setMaximumWidth(120);
    formLayout->addRow(QStringLiteral("服务器 IP:"), m_hostEdit);
    formLayout->addRow(QStringLiteral("端口:"), m_portEdit);
    rootLayout->addLayout(formLayout);

    auto *btnRow = new QHBoxLayout();
    m_connectBtn = new QPushButton(QStringLiteral("连接"), this);
    m_disconnectBtn = new QPushButton(QStringLiteral("断开"), this);
    m_pingBtn = new QPushButton(QStringLiteral("发送 Ping"), this);
    m_autoHeartbeatCheck = new QCheckBox(QStringLiteral("自动心跳 (5s)"), this);
    btnRow->addWidget(m_connectBtn);
    btnRow->addWidget(m_disconnectBtn);
    btnRow->addWidget(m_pingBtn);
    btnRow->addWidget(m_autoHeartbeatCheck);
    btnRow->addStretch();
    rootLayout->addLayout(btnRow);

    m_logEdit = new QTextEdit(this);
    m_logEdit->setReadOnly(true);
    rootLayout->addWidget(m_logEdit, 1);

    setCentralWidget(central);

    m_statusLabel = new QLabel(QStringLiteral("未连接"), this);
    statusBar()->addPermanentWidget(m_statusLabel);

    m_heartbeatTimer.setInterval(5000);
    connect(&m_heartbeatTimer, &QTimer::timeout, this, &MainWindow::onHeartbeatTimeout);

    connect(m_connectBtn, &QPushButton::clicked, this, &MainWindow::onConnectClicked);
    connect(m_disconnectBtn, &QPushButton::clicked, this, &MainWindow::onDisconnectClicked);
    connect(m_pingBtn, &QPushButton::clicked, this, &MainWindow::onPingClicked);
    connect(m_autoHeartbeatCheck, &QCheckBox::toggled, this, &MainWindow::onAutoHeartbeatToggled);

    connect(m_client, &TcpClient::connected, this, &MainWindow::onClientConnected);
    connect(m_client, &TcpClient::disconnected, this, &MainWindow::onClientDisconnected);
    connect(m_client, &TcpClient::errorOccurred, this, &MainWindow::onClientError);
    connect(m_client, &TcpClient::logMessage, this, &MainWindow::onClientLog);
    connect(m_client, &TcpClient::pingReplyReceived, this, &MainWindow::onPingReply);

    updateConnectionUi(false);
    appendLog(QStringLiteral("就绪。请先启动 VM 内 nginx（监听 8080），再连接。"));
}

MainWindow::~MainWindow() = default;

void MainWindow::onConnectClicked()
{
    const QString host = m_hostEdit->text().trimmed();
    bool ok = false;
    const quint16 port = static_cast<quint16>(m_portEdit->text().toUShort(&ok));
    if (host.isEmpty() || !ok) {
        appendLog(QStringLiteral("请输入有效的 IP 和端口"));
        return;
    }
    m_client->connectToServer(host, port);
}

void MainWindow::onDisconnectClicked()
{
    m_heartbeatTimer.stop();
    m_autoHeartbeatCheck->setChecked(false);
    m_client->disconnectFromServer();
}

void MainWindow::onPingClicked()
{
    m_client->sendPing();
}

void MainWindow::onAutoHeartbeatToggled(bool enabled)
{
    if (!m_client->isConnected()) {
        m_autoHeartbeatCheck->setChecked(false);
        appendLog(QStringLiteral("请先连接再开启自动心跳"));
        return;
    }
    if (enabled) {
        m_heartbeatTimer.start();
        appendLog(QStringLiteral("已开启自动心跳（5 秒）"));
        onPingClicked();
    } else {
        m_heartbeatTimer.stop();
        appendLog(QStringLiteral("已关闭自动心跳"));
    }
}

void MainWindow::onClientConnected()
{
    updateConnectionUi(true);
}

void MainWindow::onClientDisconnected()
{
    m_heartbeatTimer.stop();
    m_autoHeartbeatCheck->setChecked(false);
    updateConnectionUi(false);
}

void MainWindow::onClientError(const QString &message)
{
    appendLog(QStringLiteral("[错误] %1").arg(message));
}

void MainWindow::onClientLog(const QString &message)
{
    appendLog(message);
}

void MainWindow::onPingReply()
{
    appendLog(QStringLiteral("Ping 成功"));
}

void MainWindow::onHeartbeatTimeout()
{
    if (m_client->isConnected()) {
        m_client->sendPing();
    }
}

void MainWindow::appendLog(const QString &line)
{
    m_logEdit->append(line);
}

void MainWindow::updateConnectionUi(bool connected)
{
    m_connectBtn->setEnabled(!connected);
    m_disconnectBtn->setEnabled(connected);
    m_pingBtn->setEnabled(connected);
    m_autoHeartbeatCheck->setEnabled(connected);
    m_hostEdit->setEnabled(!connected);
    m_portEdit->setEnabled(!connected);
    m_statusLabel->setText(connected ? QStringLiteral("已连接") : QStringLiteral("未连接"));
}
