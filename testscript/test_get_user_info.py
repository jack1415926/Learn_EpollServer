#!/usr/bin/env python3
"""GetUserInfo (msgCode=7) Cache-Aside 联调：需先注册/登录或手动 INSERT users"""
import socket
import struct

HOST = "127.0.0.1"
PORT = 8080
CMD_GET_USER_INFO = 7

NGX_USER_OK = 0
NGX_USER_NOT_FOUND = 1


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


def pack_user_id(user_id: int) -> bytes:
    return struct.pack("!q", user_id)


def parse_get_user_resp(body: bytes):
    if len(body) < 68:
        return None
    i_result, user_id = struct.unpack("!iq", body[:12])
    username = body[12:68].split(b"\0", 1)[0].decode(errors="replace")
    return i_result, user_id, username


def query(sock, user_id: int, label: str):
    send_pkg(sock, CMD_GET_USER_INFO, pack_user_id(user_id))
    code, body = recv_pkg(sock)
    parsed = parse_get_user_resp(body) if body else None
    print(f"[{label}] msgCode={code} userId={user_id} ->", parsed)
    return parsed


def main():
    # 默认测 id=1；不存在用户可改 99999 验证 NULL_USER 缓存
    exist_id = int(input("存在的 userId [默认 1]: ").strip() or "1")
    missing_id = int(input("不存在的 userId [默认 99999]: ").strip() or "99999")

    s = socket.socket()
    s.connect((HOST, PORT))

    print("--- 存在用户：第 1 次（应查库并 SET Redis）---")
    query(s, exist_id, "hit-1")
    print("--- 存在用户：第 2 次（应只 GET Redis）---")
    query(s, exist_id, "hit-2")

    print("--- 不存在用户：第 1 次（查库 + SET NULL_USER）---")
    query(s, missing_id, "miss-1")
    print("--- 不存在用户：第 2 次（GET NULL_USER）---")
    query(s, missing_id, "miss-2")

    s.close()
    print("验证: redis-cli GET user:info:%d" % exist_id)
    print("验证: redis-cli GET user:info:%d  应为 NULL_USER" % missing_id)


if __name__ == "__main__":
    main()
