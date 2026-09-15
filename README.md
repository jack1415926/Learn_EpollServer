# **🚀 Learn\_EpollServer**

**基于 Epoll 与线程池的 C++ 并发服务器学习项目**

## **📖 项目简介**

Learn\_EpollServer 是一个借鉴 Nginx 架构思想实现的 C++ 网络服务器学习项目。本项目采用 **Reactor 模型**，底层依赖 Linux epoll 机制进行多路复用，并配合自定义线程池处理并发连接。

无论是作为 C++ Linux 服务端开发的学习基石，还是作为轻量级业务服务器的底层框架，本项目都提供了极具参考价值的源码实现。

## **✨ 核心特性**

* ⚡ **事件驱动网络 I/O**: 基于 epoll 水平触发 (LT) 模式和非阻塞套接字处理连接。
* 🛡️ **多进程架构练习**: 借鉴 Nginx 的 Master-Worker 模型，支持守护进程 (Daemon) 模式后台运行。
* 🧵 **并发线程池**: 自定义实现的高效 C++ 线程池，将网络数据收发与核心业务逻辑完美解耦。  
* 📦 **自定义协议处理**: 采用“包头 \+ 包体”的二进制通信协议，通过收包状态机处理 TCP 粘包、半包。
* 🛠️ **配套工具**:
  * 🖥️ **Qt 可视化客户端**: qt-client/ 目录下包含基于 Qt 编写的图形化测试客户端。  
  * 🐍 **Python 压测脚本**: testscript/ 目录下包含 TCP 和 Redis 的高并发压力测试脚本。
  * 🔌 **只读 MCP 接口**: epoll_mcp/ 通过本机 stdio 向 Agent 提供 Ping、文档检索和固定日志读取。

## **📂 核心目录结构**

Learn\_EpollServer/  
├── server/       \# 服务端源码  
│   ├── app/      \# 主程序入口、配置文件加载、核心初始化  
│   ├── net/      \# 网络层：Socket 封装、连接池、Epoll 事件分发  
│   ├── logic/    \# 业务逻辑层：业务请求注册与处理  
│   ├── misc/     \# 杂项模块：内存池、线程池、CRC32 校验  
│   ├── proc/     \# 进程管理：守护进程、Master-Worker 进程循环  
│   ├── signal/   \# 信号处理模块  
│   └── include/  \# 所有头文件  
├── sql/          \# 数据库初始化脚本  
├── testscript/   \# 压力测试与功能测试脚本  
├── qt-client/    \# Qt 图形化 TCP 测试客户端  
├── epoll_mcp/    \# 面向本机 Agent 的只读 MCP sidecar
└── docs/         \# 学习笔记与项目文档

## **🚀 编译与运行**

### **环境要求**

* 操作系统：Linux (推荐 Ubuntu / CentOS)  
* 编译器：GCC / G++（C++17）
* 构建工具：Make

### **快速启动**

1. **编译服务端程序**：  
   cd server && make

   编译成功后，server 目录下会生成可执行文件 nginx。  
2. **运行服务端**：  
   cd server && ./nginx

   *注：默认可能以守护进程模式运行，可通过修改 server/nginx.conf 配置文件调整行为。*

### MCP sidecar

```bash
python -m pip install -r requirements-mcp.txt
python -m epoll_mcp.server
```

MCP 客户端应以 stdio 方式启动上述模块；从仓库外启动时，可将参数改为 `F:\codex_project\Learn_EpollServer-main\epoll_mcp\server.py` 这样的绝对脚本路径。当前只暴露 `epoll_ping_server`、`epoll_search_docs` 和 `epoll_tail_log` 三个只读工具，不支持远程 HTTP、配置修改、服务启停或数据库操作。

## **🔮 路线图 (Roadmap)**

本项目正处于持续迭代中，未来的版本演进计划如下：

* \[x\] **v0.1**: 核心 epoll 框架、线程池、TCP 包处理和 Qt 测试客户端。
* \[x\] **v0.2**: 引入 **MySQL** 持久化与 Worker 独立连接池。
* \[x\] **v0.3**: 集成 **Redis Cache-Aside** 与限流实验。
* \[ \] **v0.4**: 修复资源生命周期和协议测试，补齐优雅退出与可复现压测。
* \[ \] **v0.5**: 已增加只读 stdio MCP MVP；完整运维诊断 Agent 与 KBrag V6 工具级演示仍待实现。

**当前进度（2026-09-15）**：v0.4 进行中。提交 `9072832` 已在Ubuntu 24.04.5 VM（G++ 13.3）完成Debug构建；当前16项Python离线回归、线程池排空及4个停机场景通过。注册、登录、用户查询、Redis缓存和多Worker独立Redis连接已完成实机联调。压测端支持多进程、延迟抽样、预分配Ping完整校验和整机资源采样；单进程约减半已定位为Python用户态超时socket开销，改用Linux内核收发超时后严格单进程恢复到约1.61W。最终干净提交基线在12-vCPU VMware本地环回、Debug、Redis限流关闭条件下三轮中位数约5.19W QPS，600万请求全部成功。该结果用于个人项目验证，不代表生产吞吐或服务器上限；密码安全、慢读及sanitizer专项仍未完成。

新会话先阅读 [当前状态与下一步](docs/CURRENT_STATUS_AND_NEXT_STEPS.md)。停机测试步骤见 [Ubuntu 验收说明](docs/SHUTDOWN_VALIDATION.md)，压测方法与当前边界见 [性能验证说明](docs/PERFORMANCE_VALIDATION.md)。本轮不将代码修改视为生产可用或性能已验证。

详细优先级、验收标准与 Agent 集成设计见 [`docs/OPTIMIZATION_AND_AGENT_ROADMAP.md`](docs/OPTIMIZATION_AND_AGENT_ROADMAP.md)。

## **🤝 参与贡献**

欢迎任何对 C++ 后端开发感兴趣的开发者提交 Issue 或 Pull Request。如果您觉得这个项目对您的学习有帮助，欢迎点亮 ⭐️ **Star**！
