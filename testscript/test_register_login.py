#!/usr/bin/env python3
"""简易注册/登录测试（需与服务端 STRUCT_REGISTER / STRUCT_LOGIN 布局一致）"""
import socket
import struct

HOST = "127.0.0.1"
PORT = 8080
CMD_REGISTER = 5
CMD_LOGIN = 6

def send_pkg(sock, msg_code, body: bytes):
    pkg_len = 8 + len(body)
    header = struct.pack("!HHi", pkg_len, msg_code, 0)
    sock.sendall(header + body)

def recv_pkg(sock):
    hdr = sock.recv(8)
    if len(hdr) < 8:
        return None, None
    pkg_len, msg_code, _ = struct.unpack("!HHi", hdr)
    body_len = pkg_len - 8
    body = sock.recv(body_len) if body_len > 0 else b""
    return msg_code, body

def main():
    user, pwd = "testuser01", "pass1234"
    s = socket.socket()
    s.connect((HOST, PORT))

    # 注册：iType(4) + username(56) + password(40) = 100
    reg_body = struct.pack("!i", 0) + user.encode().ljust(56, b"\0")[:56] + pwd.encode().ljust(40, b"\0")[:40]
    send_pkg(s, CMD_REGISTER, reg_body)
    code, body = recv_pkg(s)
    iType = struct.unpack("!i", body[:4])[0] if body else -1
    print("注册响应 msgCode=", code, "iType=", iType)

    # 登录：iResult(4) + username(56) + password(40)
    login_body = struct.pack("!i", 0) + user.encode().ljust(56, b"\0")[:56] + pwd.encode().ljust(40, b"\0")[:40]
    send_pkg(s, CMD_LOGIN, login_body)
    code, body = recv_pkg(s)
    iResult = struct.unpack("!i", body[:4])[0] if body else -1
    print("登录响应 msgCode=", code, "iResult=", iResult)

    s.close()

if __name__ == "__main__":
    main()
