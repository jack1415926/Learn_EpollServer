# 当前状态与下一会话交接

> 快照时间：2026-09-15，工作目录 `/home/yzy/epoll`。新会话应先阅读本文，再查看 [性能验证说明](PERFORMANCE_VALIDATION.md) 和 [优化路线图](OPTIMIZATION_AND_AGENT_ROADMAP.md)。

## 1. 仓库与环境状态

- 当前性能代码提交：`9072832`；Redis-off文档提交：`10065ed`。
- 本轮最终文档提交并推送后工作树应为干净状态；新会话仍应先用 `git status --short` 核对，不执行会覆盖未知改动的pull、reset或checkout。
- 环境：Ubuntu 24.04.5 LTS、G++ 13.3、Python 3.12.3；VM 已从 6 vCPU 调整为 1 插槽 × 12 vCPU，约 3.8 GiB 内存。
- MySQL 8.0.46 和 Redis 7.0.15 当前均为 active；真实服务使用开发账号和 `epoll_db.users`。
- nginx 已停止，8080、18080、18081、18082 均无监听；`server/nginx` 是当前 Debug 构建产物并被 Git 忽略。
- 项目 `.venv` 已创建并安装 `mcp==1.29.0`，目录被 Git 忽略。

## 2. 已完成的 Linux 实机验收

以下结果均在当前 Ubuntu VM 实际运行，不是 Windows 或离线推断：

- 系统开发依赖安装完成，`make -C server clean && make -C server` 的 Debug 干净编译通过。
- 4项MCP、3项协议及扩展后的压测客户端/资源采样器测试通过，当前完整离线套件为16/16。
- 线程池排空测试在 10 秒内返回 0。
- `test_shutdown.py` 的 SIGTERM、SIGQUIT、SIGINT、Worker SIGKILL 四个场景全部通过；Worker 被回收且端口释放。
- 真实 MySQL/Redis 联调完成：注册、登录、存在用户查询、不存在用户查询均通过。
- Redis 已直接观察到正常用户 JSON 缓存与 `NULL_USER` 短 TTL，不是只用重复响应推断命中。
- 通过不同 loopback 四元组发送请求后，4 个 Worker 均观察到各自独立的 Redis TCP 连接。
- stdio MCP Client 已完成三个工具发现、文档检索、固定日志读取和真实 C++ Ping；服务停止后返回结构化 `connection_failed`。

最新停机日志：

```text
/tmp/epoll-shutdown-zpppc16m
```

真实业务测试在数据库留下测试用户 ID 1；这是开发数据，不是生产凭据。

## 3. 本轮未提交实现

### 可配置 Redis L2 限流

`server/nginx.conf` 新增：

```ini
RedisRateLimitEnable = 1
RedisRateLimitWindowSec = 60
RedisRateLimitMaxRequests = 20
```

默认安全行为不变。Lua 脚本现在使用窗口配置，限流日志改用项目支持的 `%L` 输出 64 位计数；已实测第 21 次请求触发时日志正确显示 `21`。

### 可复现 Ping 压测客户端

`testscript/tcp_stress_test.py` 已替换旧硬编码脚本，现支持：

- CLI 指定目标、超时、并发、单连接请求数和可选源 IP。
- 支持 `--processes` 拆分客户端进程，保持总连接数和总请求数不变。
- 支持全量、每N次抽样或关闭延迟记录；正式模式都完整收包并校验包长、CRC、命令码和Ping空包体。
- 固定Ping请求包只构造一次，默认使用预分配缓冲区和 `recv_into()` 完整收包。
- 默认使用Linux `SO_RCVTIMEO/SO_SNDTIMEO` 内核超时，避免Python超时socket对每次I/O增加用户态就绪等待；仍保留响应超时与错误分类。
- `single`、`length-only` 和无响应超时的 `blocking` 只用于历史口径诊断，不能作为正式结果。
- 输出成功、失败、未尝试、错误分类、QPS 和 p50/p95/p99/max。
- JSON 记录 Git HEAD、dirty 状态、系统、VM CPU/内存，以及声明的构建、Worker、线程和限流模式。
- 全部请求成功才返回 0；限流、断连或协议错误会返回非零。

`testscript/test_tcp_stress.py` 覆盖分段响应、错误命令、延迟模式和客户端进程连接分配。

### 同轮性能资源采样

新增 `testscript/performance_sampler.py`，它包裹压测客户端并输出独立 JSON，记录：

- Master、直接子 Worker、Python 控制进程及其客户端子进程、Redis 的逐时刻 CPU、RSS 和线程数。
- 整机 CPU busy/user/system/iowait/steal、上下文切换、进程创建和 load average。
- 每个进程的平均/最大 CPU、最大 RSS 和最大线程数汇总。
- 同一时间窗口内 `error.log` 新增的收发队列长度与发送丢包记录。
- 压测命令、标签、持续时间和客户端退出码。

采样器透传客户端退出码，已完成 3 项离线回归、短命令冒烟，并与 400000 请求 Debug 基线完成首轮共同运行。

## 4. 已完成的压测记录

历史原始JSON路径如下；多次VM重启已清理早期 `/tmp` 目录，旧数据仅保留本文汇总。本次最终干净提交数据位于最后列出的 `/tmp/epoll-final-debug.XbAu3a/`：

```text
/tmp/epoll-benchmark.qwMwDE/baseline.json
/tmp/epoll-benchmark.qwMwDE/redis.json
/tmp/epoll-benchmark.qwMwDE/rate-limit-final.json
/tmp/epoll-benchmark-next.6szSID/baseline-debug-200x2000-run1.json
/tmp/epoll-benchmark-next.6szSID/baseline-debug-200x2000-run1-resources.json
/tmp/epoll-benchmark-next.6szSID/baseline-debug-warmup.json
/tmp/epoll-benchmark-next.6szSID/baseline-debug-repeat{1,2,3}.json
/tmp/epoll-benchmark-next.6szSID/baseline-debug-repeat{1,2,3}-resources.json
/tmp/epoll-benchmark-12core.ofMk9E/baseline-debug-12core-run{1,2,3}.json
/tmp/epoll-benchmark-12core.ofMk9E/baseline-debug-12core-run{1,2,3}-resources.json
/tmp/epoll-old-12core.SUHIQa/old-server-strict-12core-run1.json
/tmp/epoll-old-12core.SUHIQa/old-server-strict-12core-run1-resources.json
/tmp/epoll-calibration-12core/*.json
/tmp/epoll-8proc-12core/*.json
/tmp/epoll-final-debug.XbAu3a/{warmup,run1,run2,run3}.json
/tmp/epoll-final-debug.XbAu3a/run{1,2,3}-resources.json
/tmp/epoll-final-redis-on.2qxi5Z/{warmup,run1,run2,run3}.json
/tmp/epoll-final-redis-on.2qxi5Z/run{1,2,3}-resources.json
```

结果：

| 模式 | 负载 | 成功 | 观测 QPS | p95 |
|---|---:|---:|---:|---:|
| Debug，Redis 限流关闭 | 20 × 50 = 1000 | 1000/1000 | 约 6497 | 约 6.73 ms |
| Debug，Redis Lua 开启，阈值 100000 | 20 × 50 = 1000 | 1000/1000 | 约 6392 | 约 6.62 ms |
| Debug，Redis 限流关闭，历史参数首轮 | 200 × 2000 = 400000 | 400000/400000 | 约 10111 | 约 48.87 ms |
| 6 核 Debug，严格客户端，三轮中位数 | 每轮 200 × 2000 | 1200000/1200000 | 约 9931 | 约 49.15 ms |
| 12 核 Debug，严格客户端，三轮中位数 | 每轮 200 × 2000 | 1200000/1200000 | 约 8552 | 约 56.72 ms |
| 12 核旧服务端，严格客户端 | 200 × 2000 = 400000 | 400000/400000 | 约 8571 | 约 56.56 ms |
| 12 核当前服务端，旧客户端有效轮次 | 每轮 200 × 2000 | 旧脚本报告全部成功 | 约 17969、18274 | 不采样 |
| 12 核 Debug，6进程严格快路径、CPU隔离，长期三轮中位数 | 每轮 200 × 10000 | 6000000/6000000 | 约 50261 | 抽样约 7.22 ms |
| 12核 Debug，6进程内核超时，服务4核/客户端8核 | 每轮200 × 10000 | 6000000/6000000 | 约53059 | 抽样约6.60 ms |
| 12核 Debug，8进程内核超时，服务4核/客户端8核 | 每轮200 × 10000 | 6000000/6000000 | 约49442 | 抽样约6.75 ms |
| `9072832`干净提交最终Debug基线，6进程 | 每轮200 × 10000 | 6000000/6000000 | 约51873 | 抽样约6.69 ms |
| Redis-on高阈值对照，6进程 | 每轮200 × 10000 | 6000000/6000000 | 约27893 | 抽样约10.59 ms |

前两行只是并发 20、总请求 1000 的脚本冒烟。第三行虽然恢复了历史负载规模，但客户端和服务端仍共用 VM，工作树为 dirty，且只完成一轮 Debug；三者都不能直接与历史 1.9W/2.7W 比较或更新为项目性能数据。

默认限流另做功能测试：独立源 IP 发送 21 次，前 20 次成功，第 21 次无响应，客户端分类为 timeout 并返回 1，服务端日志记录计数 21。

完成 25000 请求预热后，历史参数三轮重复的 QPS 为 9844、10132、9931，最大跨度约 2.9%；p50/p95/p99 中位数为 15.47/49.15/68.50 ms。三轮 Python 客户端平均 CPU 为 187.53%–191.88%，4 个 Worker 平均 CPU 合计为 66.27%–67.02%，单 Worker 最大 RSS 约 28 MiB；服务日志中的收/发队列峰值不超过 3/1，发送丢包为 0。该基线使用 dirty 工作树、Debug 和同机 Python 客户端，客户端资源占用明显高于服务端且 Worker 队列接近空，因此不能把约 9931 QPS 中位数解释为服务器上限。测试后 Master/Worker 已经 SIGTERM 正常回收，18080 端口释放。

12 核严格客户端三轮 QPS 为 8504、8553、8552，中位数 8552，比 6 核低约 13.9%。旧服务端配同一严格客户端也只有 8571 QPS，而当前/旧服务端换回旧客户端都达到约 1.77W–1.83W。由此可判断，当前与历史数字的主要差异是客户端测量开销和统计口径，不是 MySQL 或当前服务端改造；增加 vCPU 也没有在当前 VM 调度条件下带来线性收益。旧客户端第三轮因 `time.time()` 遭遇 VM 墙钟跳变而无效。

客户端校准已经进一步证实这一点：单进程全量/抽样/关闭延迟都约 0.86W，而2/4/6进程完整校验分别提升到约2.04W/3.70W/5.16W。固定 Ping 快路径和 CPU 0–5/6–11 隔离后，6进程长期三轮共600万请求全部成功，中位数约5.03W QPS，抽样 p95约7.22 ms。三轮仍在4.55W–5.92W间波动，且吞吐与客户端/Worker获得的CPU时间同步变化、队列和丢包没有饱和迹象；这表明约1W的压测端瓶颈已解决，但同机VMware调度仍不适合声明服务器上限。

后续A/B已精确定位单进程减半原因：Python用户态超时socket的严格/简化收包都只有约0.88W–0.92W；同一新客户端切回阻塞I/O立即达到1.78W，同窗口旧客户端为1.82W。Linux内核收发超时配严格 `recv_into` 达到约1.61W，并保留超时失败边界。相同服务4核/客户端8核条件下，6进程长期中位数约5.31W，高于8进程约4.94W，因此当前同机候选为6进程而不是8进程。

内核超时让严格单进程相对约0.89W提升约81%。6进程最新中位数比早期约5.03W高约5.6%，但两批结果跨VM重启且CPU分配不同，不能据此宣称多进程也由同一修复确定提升5.6%。

2026-09-15在已推送提交 `9072832`、干净工作树上完成最终个人项目Debug基线：三轮QPS为44312、51873、54906，中位数51873；p50/p95/p99中位数3.00/6.69/9.33 ms，600万请求全部成功、失败0、发送丢包0。三轮范围仍约24%，简历必须注明12-vCPU VMware本地环回、Debug和Redis限流关闭，不能表述为生产吞吐或服务器上限。

相同条件下开启Redis Lua限流、临时将阈值提高到1亿后，三轮QPS为23846、27893、34728，中位数27893；p50/p95/p99中位数5.17/10.59/14.02 ms，600万请求全部成功。Redis记录610万次成功 `EVAL`，与预热及正式请求总数一致。相比Redis-off，QPS中位数下降约46.2%，p95上升约58.3%，说明当前每请求同步Redis往返是明确成本。

## 5. 历史目录核对结论

可读历史目录：`/home/yzy/Learn_EpollServer-main`，它不是 Git 仓库。

- 旧 `python_test/tcp_stress_test.py` 明确使用 200 并发、每连接 2000 请求，共 400000 请求。
- 旧构建配置为 Debug，服务为 4 Worker × 120 线程。
- 旧客户端只执行一次 `recv(8)`，不保证收满，不校验命令码或 CRC；全局统计也缺少可靠同步和错误分类。
- 旧源码没有 Redis 限流配置开关，现存版本会对每个合法包执行 Redis Lua。
- 学习笔记只记录“2.7W QPS”，用户记忆中另有约 1.9W 且关闭 Redis，但目录中没有原始报告可验证测试日期和代码状态。
- 2026-05-15 历史日志明确包含大量 Redis 限流和本地黑名单记录，说明至少有一轮旧测试开启了 Redis；不能用它证明“关闭 Redis”的那轮条件。

隔离复测已经基本复现约 1.9W 的历史口径，但旧脚本的完整收包、校验、计数同步和非单调计时问题仍然存在，因此历史数字只能作为旧口径参考，不能作为严格基准。

## 6. 当前收尾与可选后续

个人项目范围内的功能联调、停机验收、压测器校准和最终Debug基线已经完成。当前不需要继续追求Release或服务器理论上限。

### 第一步：审查并保留当前改动

```bash
cd /home/yzy/epoll
git status --short
git diff --check
git diff
```

不要覆盖当前工作树。先复核限流配置和压测脚本；如需提交，由用户决定提交时机和提交信息。

### 第二步：同轮资源记录已完成

`testscript/performance_sampler.py` 的实现、离线验证和最终三轮同步采样均已完成，记录包括：

- Master 和全部 Worker PID。
- 每个 Worker CPU、RSS、线程数。
- Python 压测进程 CPU，判断客户端是否先饱和。
- Redis CPU。
- 服务日志中的收/发队列长度和丢包计数。

最终采样结果和客户端JSON使用同一标签和目录；示例命令见 [PERFORMANCE_VALIDATION.md](PERFORMANCE_VALIDATION.md)。

### 第三步：性能主线已收尾

Debug历史参数、旧版本交叉实验、多进程客户端校准和干净提交最终基线均已完成。简历使用约5.2万QPS中位数及明确的VM/Debug/Redis-off限定即可：

```text
200 并发 × 每连接 10000 请求 = 每轮 2000000 请求
```

不需要为当前个人项目专门构建Release。不要修改并提交默认的 `RedisRateLimitEnable = 1`。

### 第四步：Redis对照已完成

相同负载下的限流关闭/开启对照已经完成，可在面试中说明同步Redis Lua往返的性能成本。

可选对照做两组：

1. Redis 限流关闭：网络、线程池、发送链路基线。
2. Redis Lua 开启，阈值显式提高到本轮总请求以上：测量每请求 Redis 往返成本。

两组必须保持构建模式、Worker、线程、请求数和客户端位置一致。

### 第五步：更高精度测试仅作可选项

客户端和服务端目前同处12 vCPU VM。单进程减半已定位并修复，6进程优于8进程；最终干净提交中位数约5.19W。只有以后要研究服务器理论上限时，才需要外部压测机、Release和更完整的并发阶梯。

### 第六步：处理其他老旧脚本

`testscript/redis_stress_test.py` 仍是旧式硬编码脚本，并把无响应、RST 和黑名单混为一类。正式使用前应重写为限流功能测试，或明确废弃并由新压测客户端的限流场景取代。

## 7. 仍未完成的其他工作

- 密码带盐哈希和响应不回传密码。
- 生产级性能上限验证（不在当前个人项目范围）。
- 慢读客户端填满发送缓冲区、EPOLLOUT 续传与停机排空。
- ASan/UBSan 和 TSan 专项。
- 长时间连接抖动、批量断连和资源泄漏测试。
- Qt 客户端跨 VM 联调。

性能测试仍不证明生产可用。认证安全属于未完成 P0，当前优先执行性能复测是用户指定的顺序调整，不代表安全项优先级下降。
