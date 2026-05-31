#ifndef PROTOCOL_TYPES_H
#define PROTOCOL_TYPES_H

#include <cstdint>

// 与服务端 ngx_comm.h / ngx_logiccomm.h 保持一致

#define PKG_HEADER_SIZE 8

#define CMD_START     0
#define CMD_PING      (CMD_START + 0)
#define CMD_REGISTER  (CMD_START + 5)
#define CMD_LOGIN     (CMD_START + 6)

#pragma pack(push, 1)

struct CommPkgHeader {
    std::uint16_t pkgLen;
    std::uint16_t msgCode;
    std::int32_t  crc32;
};

struct StructRegister {
    std::int32_t iType;
    char         username[56];
    char         password[40];
};

struct StructLogin {
    char username[56];
    char password[40];
};

#pragma pack(pop)

static_assert(sizeof(CommPkgHeader) == PKG_HEADER_SIZE, "CommPkgHeader must be 8 bytes");

#endif
