# EpollServer 优化与 Agent 集成路线图

> 更新日期：2026-09-08。本文同时记录代码进度与后续计划；“代码已修改”不等于“运行验收通过”。Agent 部分仍为计划。

## 1. 当前定位

当前项目已经具备 Linux C++17 服务端的完整学习链路：Master/Worker、epoll LT、非阻塞 Socket、线程池、自定义二进制协议、MySQL 连接池、Redis Cache-Aside、Qt 客户端和 Python 联调脚本。它适合作为 C++ 后端学习与面试项目，但尚不应描述为生产可用或经过充分性能验证的服务。

### 本轮五项优化进度

以下按本轮约定的执行顺序排列：第三项合并原 P0 的“优雅退出”和“并发关闭”；第五项对应下文 P1 压测与 P2 性能优化，不改变原优先级编号。

| 顺序 | 工作项 | 代码进度 | 验证状态 |
|---|---|---|---|
| 1 | Redis 在 Worker fork 后初始化 | 已修改 | 调用顺序已核对；Linux 构建、多 Worker Redis 联调待验收 |
| 2 | CRC、完整收包、超时与失败断言 | 已修改 | 2026-09-07 离线协议回归 3 项通过；真实业务联调待验收 |
| 3 | 优雅退出与发送/关闭并发保护 | 已修改，已提供回归脚本 | 脚本语法、帮助入口和差异检查通过；C++、停机、慢读及 sanitizer 检查待验收 |
| 4 | 密码存储与响应认证信息 | 未开始 | 当前仍有明文密码存储和注册响应回传密码，不能视为安全问题已解决 |
| 5 | 可复现压测，再根据数据优化 | 未开始 | 现有压测脚本尚未完成本轮改造，无新增性能实测结论 |

用户已有 VMware + Ubuntu，已约定由用户在 VM 中进行编译和运行测试；截至本次整理尚未收到 VM 验收结果。停机验收入口见 [SHUTDOWN_VALIDATION.md](SHUTDOWN_VALIDATION.md)，协议联调命令见下文 P0 第 2 项。

下一项代码工作是认证边界；前三项的 VM 验收独立跟踪，不提前标记为完成。

## 2. 优化优先级

### P0：先保证正确性和可验证性

1. **Redis 改为 fork 后初始化（代码已调整，运行验收待完成）**：`CLogicSocket::Initialize()` 只调用父类初始化监听资源；`ngx_worker_process_init()` 在启动线程前调用 `InitializeRedis()`，每个 Worker 独立创建 Redis 连接池。2026-09-07 已检查调用顺序和差异格式；本机未安装 WSL，尚未完成 Linux 构建及多 Worker 并发读写验收。连接池创建日志不代表 Redis 已连接成功，仍需实际请求验证。
2. **修复协议测试脚本（已修改，离线回归通过，真实联调待完成）**：两个脚本共用 `testscript/protocol.py`，发送包体 CRC32，完整接收并校验响应 CRC、包长、命令码、业务结果及返回身份。连接和读写默认超时 10 秒，失败退出码为 1，断言在 `python -O` 下仍有效。注册测试每次创建一个唯一测试账号；查询脚本通过参数指定已存在/不存在的 ID。2026-09-07 离线回归通过，包含分段/连续包、CRC 标准向量、错误包长、断连，以及两个脚本的成功和七类失败退出检查；尚未连接真实 Linux/MySQL/Redis 服务。重复查询成功不代表已证实缓存命中。

   ```bash
   # 离线协议及模拟服务回归，无需 MySQL/Redis
   python -m unittest discover -s testscript -p test_protocol.py -v
   # 真实联调：替换地址和已知的用户 ID，每次注册会新增一条测试数据
   python testscript/test_register_login.py --host 127.0.0.1 --port 8080 --timeout 10
   python testscript/test_get_user_info.py --host 127.0.0.1 --exist-id 1 --missing-id 99999
   ```
3. **实现优雅退出（代码已修改，Ubuntu 运行验收待完成）**：SIGTERM/SIGQUIT/SIGINT 只设置信号标记，Master 在主循环转发停止信号并回收 Worker；Worker 停止接收，线程池处理完已入队任务，最多再用 5 秒发送剩余响应，然后停止辅助线程、释放连接及 MySQL/Redis。异常 Worker 退出采用整组停止策略，Master 返回 1，不自动重启。后端连接/读写及 Redis 池等待增加 5 秒超时；业务排队耗时不包含在发送阶段的 5 秒内，不承诺整个停机过程固定 5 秒结束。测试步骤见 [SHUTDOWN_VALIDATION.md](SHUTDOWN_VALIDATION.md)。
4. **收紧认证边界（未开始）**：密码改为带盐哈希，响应不再携带密码；公开部署前补充传输加密、会话身份和管理接口鉴权。
5. **处理连接并发关闭（代码已修改，运行验收待完成）**：每个 Worker 使用网络状态锁串行化收发、关闭和回收；锁不跨越 MySQL/Redis 调用。业务任务执行期间固定连接对象，关闭后旧响应通过序列号检查丢弃；重复关闭不重复回收，发送使用 `MSG_NOSIGNAL`。此方案优先保证正确性，网络锁对吞吐的影响待压测，尚未经过 TSan 验证。

完成标准：Linux 环境可重复构建；功能脚本能自动判定通过/失败；多 Worker、断连和退出场景留下可复现实验记录。

### P1：建立可信的工程验证

- 为包头编解码、CRC 和关键配置解析增加少量确定性测试，不为追求数量引入庞大测试框架。
- 增加 ASan/UBSan 构建；并发路径再使用 TSan 做专项检查。
- 压测参数改为命令行输入，输出成功率、错误分类、QPS、p50/p95/p99 延迟，并同时记录 CPU、内存、Worker/线程数和 Redis/MySQL 配置。
- 同步 README 与实现状态，性能数据没有实测前不使用“海量并发”“工业级”等结论。

### P2：基于数据优化性能

- 评估每个业务包同步执行 Redis `EVAL` 的延迟和吞吐影响，再决定是否采用本地一级计数、`EVALSHA` 或批量上报。
- 回收队列已在第三项修改中使用迭代器连续删除，但未测量性能收益；定时器删除路径仍有从头遍历，后续根据数据优化，只有测得瓶颈后再增加反向索引等额外结构。
- 根据压测调整当前 `4 Worker × 120` 业务线程及数据库连接池大小，不把更大的线程数默认视为更高性能。

## 3. 推荐的 Agent 方向

最适合本项目的第一步不是让 Agent 控制服务器，而是增加一个**只读运维与故障诊断 Agent**。它作为独立 Python 侧车运行，不进入 epoll I/O 热路径。

```text
用户 / Qt 管理页
        |
        v
agent_assistant（Python/FastAPI）
  |- search_epoll_docs：检索 README、docs、配置说明和关键源码证据
  |- analyze_server_log：解释错误日志并给出排查步骤
  `- get_server_status：后续通过受限只读接口获取状态
        |
        v
EpollServer（C++ 数据面，不调用大模型）
```

建议新增位置（均为计划）：

```text
agent_assistant/
  app.py                  # HTTP/CLI 入口
  agent.py                # 有界决策循环
  tools/
    search_epoll_docs.py  # 文档/源码检索
    analyze_server_log.py # 日志解析
    epoll_client.py       # 后续访问只读服务命令
  tests/test_tools.py
```

MVP 只实现 `search_epoll_docs` 与 `analyze_server_log`。只有当只读流程稳定后，再考虑新增本机绑定、带鉴权的 `_CMD_SERVER_STATS`。不建议一开始允许 Agent 修改 `nginx.conf`、重启服务、操作数据库或自动封禁 IP。

## 4. 与 KBrag V6 的关联方式

关联项目：`F:\BaiduNetdiskDownload\V6_submit_code`。

当前已验证，V6 使用 FastAPI 薄入口、手写有界 ReAct 循环和单一 `search_manual` 工具；技术问题经过产品路由、混合检索、RRF、rerank 后返回 parent section 证据。其 `agent.py`、提示词、图片格式和 `ProductRouter` 与产品手册客服领域紧密耦合，因此不宜直接整体复制到 EpollServer。

推荐分三步建立联系：

1. **复用方法，不复制业务代码**：沿用“路由 → 检索工具 → 观察 → 有界收束”和“答案必须带证据”的设计，把 `search_manual` 换成 `search_epoll_docs`。
2. **增加协议适配器**：P0 安全问题解决后，由 `agent_assistant/tools/epoll_client.py` 统一实现包头、CRC、超时和响应解析，调用只读状态命令。
3. **形成跨项目演示**：V6 负责 Agent/RAG 与工具编排，EpollServer 负责高并发连接、账号数据和运行状态。演示问题可以是“登录请求为什么无响应”“当前 Worker/连接数是否异常”“这条 Redis 日志如何排查”。

若以后两个项目确实出现两种以上稳定的共用工具，再抽取通用 Agent 核心；在此之前保持 API/协议边界，避免为了“关联”制造共享代码包。

## 5. 建议实施顺序

```text
Redis fork 修复
  -> 可信协议测试
  -> 优雅退出与并发关闭
  -> 密码存储与响应认证信息
  -> 可复现压测
  -> 只读文档/日志 Agent
  -> 受限状态查询工具
  -> V6 跨项目演示
```
