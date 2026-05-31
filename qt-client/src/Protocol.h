#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <QByteArray>
#include <QString>

class Protocol
{
public:
    static QByteArray buildPingPacket();
    static bool parsePingResponse(const QByteArray &buf, QString *errorOut = nullptr);
    static QString toHexPreview(const QByteArray &data, int maxBytes = 32);
};

#endif
