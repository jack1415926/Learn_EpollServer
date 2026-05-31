#include "MainWindow.h"

#include <QApplication>
#include <QNetworkProxy>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    // 避免继承系统 HTTP/SOCKS 代理，导致 TCP 直连 VM 失败
    QNetworkProxy::setApplicationProxy(QNetworkProxy::NoProxy);
    QApplication::setApplicationName(QStringLiteral("EpollQtClient"));
    QApplication::setOrganizationName(QStringLiteral("LearnEpollServer"));

    MainWindow window;
    window.show();
    return app.exec();
}
