# Epoll Server Qt 客户端（MVP）

Windows 本机 Qt6 桌面客户端，通过 TCP 对接 VMware 内 Linux epoll 服务端（默认 `8080`）。

## 功能

- 连接 / 断开（`QTcpSocket`）
- 手动发送 Ping（8 字节包头，与大端 Python `tcp_stress_test.py` 一致）
- 可选自动心跳（5 秒 `QTimer`）
- 日志窗口显示收发十六进制摘要

## 协议说明

- **客户端发送**：仅 `COMM_PKG_HEADER`（8 字节），无服务端内部消息头。
- **服务端回复**：同样仅 8 字节包头（`msgCode = 0` 表示 Ping）。
- 定义见 `shared/protocol_types.h`，与 `nginx/_include/ngx_comm.h`、`ngx_logiccomm.h` 对齐。

## 环境要求

- Windows 10/11
- Qt 6.x（Widgets、Network）
- CMake 3.16+
- MSVC 或 MinGW（与 Qt  kit 一致）
- VMware 内已编译并运行 `nginx`（`ListenPort0 = 8080`）

## 构建（Qt Creator 推荐）

1. 打开 Qt Creator → **文件 → 打开文件或项目** → 选择 `qt-client/CMakeLists.txt`
2. 选择 Qt 6 kit（Desktop）
3. 构建并运行 `EpollQtClient`

## 构建（命令行）

```powershell
cd qt-client
cmake -B build -DCMAKE_PREFIX_PATH="C:\Qt\6.x.x\msvc2019_64"
cmake --build build --config Release
.\build\Release\EpollQtClient.exe
```

将 `CMAKE_PREFIX_PATH` 换成你的 Qt 安装路径。

## 与 VM 服务端联调

1. 在 VM 内启动 Redis（若服务端启用 L2）：`redis-server`
2. 在 `nginx` 目录：`./nginx`（或你的启动方式）
3. 确认监听：`ss -lntp | grep 8080`
4. 网络任选其一：
   - **NAT + 端口转发**：Windows 连 `127.0.0.1:8080`（需在 VMware 配置端口映射）
   - **桥接**：客户端填 VM 的局域网 IP，如 `192.168.x.x`
5. 先用 Python 验证：`python tcp_stress_test.py`（在能访问到服务端的环境）
6. 再打开本客户端 → 连接 → **发送 Ping**，日志应出现 `Ping 成功`

### 连接报错：`The proxy type is invalid for this operation`

Windows 上 Qt 可能自动使用系统/环境变量里的代理，`QTcpSocket` 无法通过 HTTP/SOCKS 代理做原始 TCP 直连。本工程已在代码中设置 `QNetworkProxy::NoProxy`。若仍报错，检查系统「代理」是否开启，或临时关闭 VPN/抓包工具的本地代理。

## 目录结构

```
qt-client/
  CMakeLists.txt
  shared/protocol_types.h
  src/
    main.cpp
    MainWindow.{h,cpp}
    TcpClient.{h,cpp}
    Protocol.{h,cpp}
```

## 后续扩展

- 注册 / 登录（`STRUCT_REGISTER`、`STRUCT_LOGIN` + CRC32）
- 接收缓冲区状态机（大包体粘包）
- 多连接压测页
