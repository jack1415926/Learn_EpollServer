#include "Protocol.h"

#include "../shared/protocol_types.h"

#include <cstring>

#include <QtEndian>

QByteArray Protocol::buildPingPacket()
{
    QByteArray packet(PKG_HEADER_SIZE, '\0');
    auto *hdr = reinterpret_cast<CommPkgHeader *>(packet.data());
    hdr->pkgLen  = qToBigEndian(static_cast<quint16>(PKG_HEADER_SIZE));
    hdr->msgCode = qToBigEndian(static_cast<quint16>(CMD_PING));
    hdr->crc32   = qToBigEndian(static_cast<qint32>(0));
    return packet;
}

bool Protocol::parsePingResponse(const QByteArray &buf, QString *errorOut)
{
    if (buf.size() < PKG_HEADER_SIZE) {
        if (errorOut) {
            *errorOut = QStringLiteral("响应长度不足 %1 字节（当前 %2）")
                            .arg(PKG_HEADER_SIZE)
                            .arg(buf.size());
        }
        return false;
    }

    CommPkgHeader hdr;
    memcpy(&hdr, buf.constData(), PKG_HEADER_SIZE);
    const quint16 pkgLen  = qFromBigEndian(hdr.pkgLen);
    const quint16 msgCode = qFromBigEndian(hdr.msgCode);

    if (pkgLen != PKG_HEADER_SIZE) {
        if (errorOut) {
            *errorOut = QStringLiteral("pkgLen 异常: %1").arg(pkgLen);
        }
        return false;
    }
    if (msgCode != CMD_PING) {
        if (errorOut) {
            *errorOut = QStringLiteral("msgCode 异常: %1（期望 Ping=0）").arg(msgCode);
        }
        return false;
    }
    return true;
}

QString Protocol::toHexPreview(const QByteArray &data, int maxBytes)
{
    const int n = qMin(data.size(), maxBytes);
    QString hex;
    hex.reserve(n * 3);
    for (int i = 0; i < n; ++i) {
        if (i > 0) {
            hex.append(QLatin1Char(' '));
        }
        hex.append(QStringLiteral("%1").arg(static_cast<uchar>(data.at(i)), 2, 16, QLatin1Char('0')));
    }
    if (data.size() > maxBytes) {
        hex.append(QStringLiteral(" ..."));
    }
    return hex.toUpper();
}
