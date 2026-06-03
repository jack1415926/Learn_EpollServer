# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## 项目概述

Learn_EpollServer 是一个借鉴 Nginx 架构的 C++17 高性能网络服务器框架，基于 Epoll (ET 模式) + 多线程 + Master-Worker 多进程模型。采用自定义"包头+包体"二进制协议，集成 MySQL 连接池和 Redis Cache-Aside 缓存。

## 构建与运行

```bash
# 编译
cd server && make

# 清理
cd server && make clean

# 运行（先确保 MySQL 和 Redis 已启动）
cd server && ./nginx
```

编译依赖：g++ (C++17)、libpthread、libmysqlclient、libredis++、libhiredis。编译输出为 `server/nginx`。

## 架构

### 进程模型

Master 进程 (`NGX_PROCESS_MASTER`) fork 出 N 个 Worker 进程 (`NGX_PROCESS_WORKER`，数量由 `server/nginx.conf` 中 `WorkerProcesses` 配置)。Master 进程只做信号管理，所有网络 I/O 和业务处理在 Worker 中进行。每个 Worker 独立拥有自己的 epoll 实例、线程池、MySQL 连接池。

关键入口文件：
- `server/app/nginx.cxx` — `main()` 入口，加载配置 → 信号初始化 → socket 初始化 → fork 守护进程 → 进入 `ngx_master_process_cycle()`
- `server/proc/ngx_process_cycle.cxx` — Master/Worker 进程生命周期管理
- `server/proc/ngx_event.cxx` — Worker 事件循环 `ngx_process_events_and_timers()`

### 网络层 (`server/net/`)

核心类 `CSocekt`（`server/include/ngx_c_socket.h`，实现在 `server/net/` 下的多个 `.cxx` 文件）：

- **连接池**：预分配 `ngx_connection_t` 对象链表（`m_connectionList`），空闲连接通过 `m_freeconnectionList` 快速分配。每个连接持有 fd、读写回调函数指针、收发包状态机、epoll 事件标记等。
- **事件驱动**：`epoll_wait` 阻塞等待事件，读事件由 `rhandler` 函数指针分发（新连接 → `ngx_event_accept`，已有连接 → `ngx_read_request_handler`），写事件由 `whandler` 分发（→ `ngx_write_request_handler`）。
- **后台线程**：每个 Worker 额外创建 2-3 个辅助线程：
  - `ServerSendQueueThread`：从发送队列取数据执行实际 send，发送缓冲区满时注册 EPOLLOUT 事件驱动续传
  - `ServerRecyConnectionThread`：延迟回收关闭的连接
  - `ServerTimerQueueMonitorThread`（可选）：心跳超时踢出

Socket 初始化分为两个阶段：
1. `Initialize()` — 在 Master 进程 fork 之前调用，创建监听 socket（启用 `SO_REUSEPORT` 防惊群）
2. `Initialize_subproc()` — 在 Worker 子进程中调用，创建 epoll 实例和辅助线程

### 业务逻辑层 (`server/logic/`)

`CLogicSocket` 继承自 `CSocekt`，通过成员函数指针数组 `statusHandler[]` 按消息码 (`msgCode`) 分发业务。消息码定义在 `server/include/ngx_logiccomm.h`。

业务流程：
1. 网络层收完整包 → 封装为 `STRUC_MSG_HEADER` + `COMM_PKG_HEADER` + 包体
2. 投递到线程池消息队列 (`CThreadPool::inMsgRecvQueueAndSignal`)
3. 线程池工作线程调用 `CLogicSocket::threadRecvProcFunc()` → CRC 校验 → Redis Lua 限流 → 按 msgCode 分发到具体 `_Handle*` 方法

当前支持的业务：注册、登录、按 userId 查用户信息（Cache-Aside）、心跳 Ping。

### 线程池 (`server/misc/ngx_c_threadpool.cxx`)

`CThreadPool` 使用 POSIX 信号量 + 条件变量实现生产者-消费者模型。线程数量由 `ProcMsgRecvWorkThreadCount` 配置。工作线程在 `ThreadFunc()` 中循环从 `m_MsgRecvQueue` 取消息，调用 `g_socket.threadRecvProcFunc()`。

### 通信协议 (`server/include/ngx_comm.h`)

二进制协议，1 字节对齐 (`#pragma pack(1)`)：

```
COMM_PKG_HEADER (8 bytes):
  - pkgLen:  unsigned short (2B) — 包头+包体总长
  - msgCode: unsigned short (2B) — 消息类型
  - crc32:   int (4B)            — CRC32 校验
```

收包状态机：`_PKG_HD_INIT` → `_PKG_HD_RECVING` → `_PKG_BD_INIT` → `_PKG_BD_RECVING` → 回到 `_PKG_HD_INIT`，解决 TCP 粘包/半包。

### MySQL 连接池 (`server/misc/ngx_c_mysql_connpool.cxx`)

`CMysqlConnPool` 单例，每个 Worker 进程在 fork 后独立初始化。使用 `std::shared_ptr<MYSQL>` + 自定义 deleter 实现 RAII 归还，`std::condition_variable` 实现等待超时（5 秒）。归还时自动 ping 检测坏连接并重建。

`CMysqlDao` 使用 MySQL Prepared Statement 执行 SQL，防止注入。

### Redis Cache-Aside (`server/misc/ngx_c_user_cache.cxx`)

`CUserCacheService` 实现 Cache-Aside 模式：
- 读：先查 Redis → 命中空值哨兵（防缓存穿透）→ 未命中则查 MySQL 并回写
- 写：写入 MySQL 后更新 Redis
- 缓存 key 格式：`user:info:{userId}`，值用简单 JSON 序列化

### 防御机制

- **CRC32 校验**：每个包体验签，防篡改
- **Flood 攻击检测**：统计单连接收包频率，超阈值踢出
- **Redis Lua 限流 (L2)**：滑动窗口计数器，同一 IP 60 秒超 20 次请求则加入本地黑名单
- **本地黑名单 (L1)**：新连接到达时先检查 IP 是否在黑名单
- **心跳超时**：`std::multimap<time_t, ...>` 定时器队列，到期不发心跳则踢出
- **C 字符串防护**：收包后强制 `buf[size-1]='\0'`
- **防超大包**：包长超过 `_PKG_MAX_LENGTH`(30000) 直接丢弃
- **防发送队列堆积**：单连接积压超 400 条或全局队列超 50000 条，丢弃并切断

### 配置文件 (`server/nginx.conf`)

INI 风格，section 用 `[SectionName]` 标记。`CConfig::Load()` 解析为键值对列表，通过 `GetString()`/`GetIntDefault()` 读取。关键 section：`[Log]`、`[Proc]`、`[Mysql]`、`[Cache]`、`[Net]`、`[NetSecurity]`。

### 单例模式

项目大量使用单例 + 嵌套 `CGarhuishou` 类实现自动析构：`CConfig`、`CMemory`、`CCRC32`、`CMysqlConnPool`。

### 内存管理

`CMemory` 封装了内存池，业务代码中通过 `CMemory::AllocMemory()` 分配、`CMemory::FreeMemory()` 释放，而非直接 new/delete。

## 测试工具

```bash
# TCP 并发压测（修改脚本中的 SERVER_IP/PORT 后运行）
python3 testscript/tcp_stress_test.py

# Redis 压测
python3 testscript/redis_stress_test.py

# 注册/登录功能测试
python3 testscript/test_register_login.py

# 按 userId 查询用户信息测试
python3 testscript/test_get_user_info.py

# MySQL 初始化（需要先启动 MySQL）
mysql -u root -p < sql/init_users.sql
```

Qt 客户端在 `qt-client/` 目录，使用 CMake 构建。

## 编码注意事项

- 源文件扩展名 `.cxx`，头文件 `.h`，都放在 `server/include/`
- 头文件搜索路径：`server/include/`（编译时 `-I` 指定）
- 类名前缀 `C`（如 `CSocekt`），结构体前缀 `ngx_`，成员变量前缀 `m_`，全局变量前缀 `g_`
- 网络序/主机序转换：发送用 `htons`/`htonl`/`ngx_hton64`，接收用 `ntohs`/`ntohl`/`ngx_ntoh64`
- `ngx_log_stderr()` 和 `ngx_log_error_core()` 是主要日志 API，日志级别定义在 `ngx_macro.h`
- Windows 上无法编译运行（依赖 Linux epoll、fork、pthread 等 POSIX API）
