#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>

class TcpClient;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void onConnectClicked();
    void onDisconnectClicked();
    void onPingClicked();
    void onAutoHeartbeatToggled(bool enabled);
    void onClientConnected();
    void onClientDisconnected();
    void onClientError(const QString &message);
    void onClientLog(const QString &message);
    void onPingReply();
    void onHeartbeatTimeout();

private:
    void appendLog(const QString &line);
    void updateConnectionUi(bool connected);

    TcpClient *m_client = nullptr;
    QTimer m_heartbeatTimer;

    class QLineEdit *m_hostEdit = nullptr;
    class QLineEdit *m_portEdit = nullptr;
    class QPushButton *m_connectBtn = nullptr;
    class QPushButton *m_disconnectBtn = nullptr;
    class QPushButton *m_pingBtn = nullptr;
    class QCheckBox *m_autoHeartbeatCheck = nullptr;
    class QTextEdit *m_logEdit = nullptr;
    class QLabel *m_statusLabel = nullptr;
};

#endif
