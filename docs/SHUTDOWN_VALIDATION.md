# 第三项：停机与并发关闭验收

状态复核（2026-09-08）：代码已修改；2026-09-07 Windows 上协议回归 3 项通过，停机脚本语法、帮助入口及代码差异格式检查通过。以下 C++ 构建、线程池测试和完整停机联调尚未运行，已约定由用户在 VMware + Ubuntu 中验收，目前尚未收到运行结果。统一进度见 [优化路线图](OPTIMIZATION_AND_AGENT_ROADMAP.md)。

## 行为与边界

- 向 Master 发送 SIGTERM、SIGQUIT 或 SIGINT：关闭监听，通知 Worker 停机并等待回收。
- Worker 停止接收新请求，完成已经进入业务队列的任务，再给待发送响应最多 5 秒。超时有日志，剩余响应丢弃。
- 业务线程退出后才销毁后端连接池。MySQL/Redis 网络超时为 5 秒；队列长度和后端调用会影响总退出时间，5 秒不是总停机上限。
- Worker 非预期退出：Master 停止其他 Worker 并返回 1。正常停机返回 0。
- 网络锁覆盖发送、读事件、关闭、回收和复用；数据库查询不持有网络锁。业务任务计数防止查询期间对象复用；关闭后产生的旧响应不会发送给新连接。
- 本轮也将原有访问 protected 成员的用户信息回包自由函数改为私有成员函数，避免该访问方式阻碍编译；线协议保持不变。

## Ubuntu 中执行

在项目根目录执行；MySQL/Redis 依赖及业务数据库需按现有说明准备好。

```bash
make -C server

# 仅测试真实线程池的排空逻辑，不连接数据库；应在 10 秒内返回 0
g++ -std=c++17 -pthread -Iserver/include \
  testscript/test_threadpool_shutdown.cxx server/misc/ngx_c_memory.cxx \
  -o /tmp/test_threadpool_shutdown
/tmp/test_threadpool_shutdown

# 完整服务器停机测试，需要 MySQL/Redis 可用
python3 testscript/test_shutdown.py --binary ./server/nginx
```

停机脚本启动独立的双 Worker 实例，使用临时端口、临时配置和独立进程组，不连接或停止已运行的服务器。测试结束移除复制的配置，保留日志目录（路径会打印）。它会发送 Ping，因此会在 Redis 留下短期限流计数，不新增业务用户。

覆盖 SIGTERM/SIGQUIT/SIGINT、并发连接和 RST 断连、半包停机、Worker 被 SIGKILL 后整组退出、子进程回收和监听端口释放。任一检查失败返回非零，不以超时当作通过。

这套联调不证明所有竞态已消除，也不确定性触发发送缓冲区满。后续仍需针对慢读客户端做发送排空测试，并在 Linux 上进行 ASan/TSan 检查和性能测量。

## 验收结果记录（待用户运行后填写）

| 检查 | 当前记录 |
|---|---|
| Ubuntu / 编译器 / 依赖版本 | 待记录 |
| `make -C server` | 未运行 |
| 线程池排空测试 | 未运行 |
| 完整停机脚本及日志目录 | 未运行 |
| 多 Worker Redis 与注册/查询联调 | 未运行，另见路线图命令 |
| 慢读客户端、ASan/TSan、性能测量 | 未运行 |

填写时保留命令、退出码、日志路径和错误摘要；不要记录真实凭据。通过协议离线回归不能替代上述验收。
