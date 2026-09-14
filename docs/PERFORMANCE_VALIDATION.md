# 可复现性能验证

> 状态日期：2026-09-14。压测客户端与 Redis L2 限流配置已完成第一轮改造和小规模冒烟；尚未进行正式性能复测，下述冒烟数字不能作为服务器性能上限。

## 当前改造

`testscript/tcp_stress_test.py` 现在支持命令行指定目标、超时、并发数、单连接请求数、客户端进程数和可选源地址。固定 Ping 请求包只构造一次；默认使用每连接预分配的8字节缓冲区和 `recv_into()` 完整收满响应，并逐包校验包长、命令码和空包CRC。延迟可选择全量、每N次抽样或关闭；关闭延迟不关闭协议校验。结果包含成功率、错误分类、QPS和可选的p50/p95/p99/max延迟，可写入JSON。

默认 `kernel-timeout` 模式先用Python超时完成连接，再切回阻塞socket并通过Linux `SO_RCVTIMEO/SO_SNDTIMEO` 保留收发超时。`timeout` 保留Python用户态超时用于诊断；`blocking` 没有响应超时，`single` 收包和 `length-only` 校验也仅用于历史口径诊断，不能用于权威结果。

JSON 同时记录 Git HEAD、工作树是否 dirty、Linux/Python 版本、VM CPU/内存，以及调用者声明的构建模式、Worker 数、线程数和限流模式。

`testscript/performance_sampler.py` 可以包裹压测命令，在同一时间窗口采样 Master、全部直接子 Worker、压测控制进程及其子进程和 Redis 的 CPU、RSS、线程数，并记录整机 CPU busy/user/system/iowait/steal、上下文切换、进程创建及 load average；同时增量提取 `error.log` 中的收发队列长度和丢包计数。采样器输出独立 JSON，并透传压测命令退出码；它只读取 `/proc` 和日志，不修改服务配置或热路径。

服务端新增以下配置，默认行为仍为开启 Redis 限流、60 秒 20 次：

```ini
[NetSecurity]
RedisRateLimitEnable = 1
RedisRateLimitWindowSec = 60
RedisRateLimitMaxRequests = 20
```

基线测试必须使用独立临时配置设置 `RedisRateLimitEnable = 0`。Redis 完整链路测试保持启用，并将阈值明确提高到测试总请求以上；不能直接用默认 20 次阈值解释正常链路吞吐。

## 已执行验证

- Python 语法、CLI 帮助和非法并发参数检查通过。
- 压测客户端离线回归现覆盖分段Ping、错误字段、延迟模式、多进程连接分配和内核超时分类；资源采样器覆盖 `/proc`、进程树、整机CPU增量、日志增量和汇总。当前完整离线套件共16/16通过。
- 资源采样器已通过短命令冒烟，并与 400000 请求 Debug 基线完成首轮共同运行，能生成逐时刻样本、进程汇总及队列日志事件。
- 配置改造后 Debug 编译通过，四场景停机回归再次通过，日志位于 `/tmp/epoll-shutdown-zpppc16m`。
- 基线冒烟：Debug、4 Worker × 120 线程、限流关闭、20 客户端 × 50 请求，1000/1000 成功，约 6497 QPS，p95 约 6.73 ms。
- Redis 完整链路冒烟：相同规模、限流开启且阈值 100000，1000/1000 成功，约 6392 QPS，p95 约 6.62 ms。
- 默认限流行为：独立源地址发送 21 次，前 20 次成功，第 21 次无响应并被客户端归类为 timeout；服务端日志正确记录窗口内请求数 21。
- 历史参数初始复现：Debug、4 Worker × 120 线程、限流关闭、200 客户端 × 2000 请求，400000/400000 成功，约 10111 QPS，p50 15.43 ms、p95 48.87 ms、p99 68.73 ms。资源采样覆盖 81 个时间点。
- 6 核重复基线：另做 25000 请求预热后，连续三轮相同的 400000 请求全部成功，QPS 为 9844、10132、9931，中位数 9931，轮间最大跨度约 2.9%；p50/p95/p99 中位数分别为 15.47/49.15/68.50 ms。三轮 Python 客户端平均 CPU 为 187.53%–191.88%，4 个 Worker 平均 CPU 合计为 66.27%–67.02%，各 Worker 最大 RSS 约 28 MiB；日志收/发队列峰值不超过 3/1，发送丢包为 0。
- 12 核严格口径基线：VM 改为 1 插槽 × 12 vCPU 后，保持其余条件不变，预热后连续三轮 400000 请求全部成功，QPS 为 8504、8553、8552，中位数 8552；p50/p95/p99 中位数分别为 20.25/56.72/78.02 ms。客户端平均 CPU 为 218.11%–224.20%，4 个 Worker 平均 CPU 合计为 68.47%–70.08%，队列峰值不超过 3/1，发送丢包为 0。12 核中位数反而比 6 核低约 13.9%，不能用 vCPU 数量线性解释吞吐。
- 旧版本交叉实验：`/home/yzy/Learn_EpollServer-main` 的快照没有 MySQL，但已经无条件执行 Redis Lua。将源码复制到 `/tmp`、仅旁路 Redis 分支并隔离到 18082 后，旧服务端配当前严格客户端得到 8571 QPS，几乎等于当前服务端严格口径中位数 8552。旧服务端配原始旧客户端报告 17665 QPS；当前服务端配旧客户端三轮报告 17969、18274、16460 QPS，其中第三轮受 VM 墙钟跳变影响无效。有效历史口径落在约 1.77W–1.83W，基本复现用户记忆中的约 1.9W。
- 客户端校准矩阵：单进程 `all/sampled/off` 分别约为 8584/8708/8598 QPS，证明逐请求计时和保存延迟不是主要瓶颈；关闭延迟后改为 2/4/6 个 Python 进程分别约为 20380/36998/51623 QPS，8 进程回落到 32969。多进程扩展直接证明此前约 0.86W 是单 Python 进程上限，不是服务器上限。
- Ping 专用快路径与 CPU 隔离：预构建固定请求包、完整收满并逐次校验 8 字节响应后，单进程抽样模式达到 12094 QPS。将服务端固定 CPU 0–5、6 进程客户端固定 CPU 6–11，400000 请求三轮中位数为 55123 QPS，抽样 p95 中位数 6.84 ms，但短轮次范围仍为 50583–57953。
- 长期校准：相同 6 进程、CPU 隔离和 1/100 延迟抽样下，每轮提升到 2000000 请求，三轮 6000000/6000000 全部成功，QPS 为 50261、45461、59174，中位数 50261；p50/p95/p99 中位数为 2.82/7.22/10.21 ms。各轮吞吐与客户端和 Worker 获得的 CPU 时间同步变化，队列峰值不超过 11/24、发送丢包为 0。约 30% 的轮间跨度说明 VMware 同机测试环境仍不稳定，当前中位数只能视为 Debug 校准结果。
- 单进程减半根因：同一当前服务端、同一未绑核时间窗口下，Python用户态3秒超时socket的 `recv_exact`、预分配 `recv_into`、单次 `recv` 严格模式和仅长度诊断分别约为8791、8889、9052、9200 QPS；收包分配、完整收包和字段校验都只造成小幅差异。保持旧式内联循环、仅把连接后的socket切回阻塞模式后达到17788 QPS，同窗口原始旧客户端为18157 QPS。由此确认约减半的主因是Python超时socket对每次I/O执行额外就绪等待。Linux内核收发超时加 `recv_into` 严格校验达到16099 QPS，与无响应超时的纯阻塞严格模式16171 QPS几乎一致，并已验证超时仍被分类为 `timeout`。
- 6/8进程公平对照：服务端固定CPU 0–3，客户端固定CPU 4–11，使用内核超时、`recv_into`、严格校验和1/100延迟抽样。8进程长期三轮QPS为47337、50726、49442，中位数49442，p95中位数6.75 ms；6进程为53059、47310、53218，中位数53059，p95中位数6.60 ms。两组各6000000/6000000请求成功且无发送丢包；6进程中位数高约7.3%，当前选择6进程作为同机候选，8进程并无收益。

单进程改用内核超时后，严格口径由约0.89W提升到约1.61W，提升约81%。6进程最新长期中位数约5.31W，相比更早的约5.03W高约5.6%，但两批测试跨VM重启且CPU分配不同，不能把这部分增幅全部归因于超时实现；后续Release测试必须使用当前固定的内核超时配置重新建立基线。

前两组 1000 请求吞吐只用于证明脚本和配置模式可运行。6 核和 12 核三轮严格口径可以作为各自当前条件下的可重复基线，但不能作为服务器上限。两种 vCPU 配置下客户端 CPU 都明显高于 Worker，Worker 队列接近空；旧/新服务端在同一严格客户端下结果几乎相同，而旧客户端让两者都接近 1.8W，证明主要差异来自客户端测量开销与统计口径，不是 MySQL 或当前服务端改造。

旧客户端只调用一次 `recv(8)`、不保证收满、不校验响应内容或CRC、使用共享全局计数，并用可能受VM校时影响的 `time.time()` 计时。早期严格客户端还会全量记录延迟，但A/B已证明计时、完整收包和字段校验都不是减半主因，主因是Python用户态超时socket。当前默认的内核超时严格模式保留正确性与失败边界；约1.8W仍只能称为历史旧客户端口径，不能替代严格口径或作为可靠服务器上限。

多次VM重启已清理早期 `/tmp` 原始文件，6核/12核初始、旧版本交叉、客户端矩阵和单进程根因数据仅保留本文汇总。当前仍可读的6/8进程公平对照及资源JSON位于 `/tmp/epoll-8proc-12core/`，同样属于临时文件，后续正式结果不应只保存在 `/tmp`。

## 正式复测前置条件

1. 提交或明确记录当前 diff，使结果不再只有 `dbdf8a6 + dirty`。
2. 使用资源采样器同步记录服务端、客户端、Redis 和队列指标，并确认输出覆盖完整压测时段。
3. 分别构建 Debug 与 Release，正式数字以 Release 为主，Debug 只用于诊断对照。
4. 每个模式先预热，再运行至少三轮相同参数，保留每轮 JSON，不只报告最好结果。
5. 基线和 Redis 组使用相同 Worker、线程、连接数和请求数；测试之间确认无残留 nginx，并记录 Redis/MySQL 状态。
6. 当前多进程客户端已消除约1W的单进程上限，但相同4/8核分配下长期轮间波动仍约7%–13%；正式服务器上限应从另一台机器发压，或至少解释宿主机调度条件，不能把共享VM上限归因于服务器。

示例客户端命令：

```bash
python3 testscript/tcp_stress_test.py \
  --host 127.0.0.1 --port 18080 --timeout 3 \
  --concurrency 100 --requests-per-client 1000 \
  --processes 6 --latency-mode sampled --latency-sample-every 100 \
  --receive-mode into --validation-mode strict --socket-mode kernel-timeout \
  --label baseline-release \
  --build-mode release --server-workers 4 --worker-threads 120 \
  --rate-limit-mode disabled \
  --json-out /tmp/epoll-benchmark/baseline-release.json
```

正式运行时由采样器包裹客户端，使两份 JSON 使用相同目录和标签。`MASTER_PID` 应在启动前通过 `pgrep -o -x nginx` 等只读命令核对为 Master，而不是盲目使用任一 Worker PID：

```bash
MASTER_PID=$(pgrep -o -x nginx)
python3 testscript/performance_sampler.py \
  --master-pid "$MASTER_PID" \
  --label baseline-release-run1 \
  --output /tmp/epoll-benchmark/baseline-release-run1-resources.json \
  --log error.log --interval 0.5 -- \
  python3 testscript/tcp_stress_test.py \
    --host 127.0.0.1 --port 18080 --timeout 3 \
    --concurrency 200 --requests-per-client 2000 \
    --processes 6 --latency-mode sampled --latency-sample-every 100 \
    --receive-mode into --validation-mode strict --socket-mode kernel-timeout \
    --label baseline-release-run1 \
    --build-mode release --server-workers 4 --worker-threads 120 \
    --rate-limit-mode disabled \
    --json-out /tmp/epoll-benchmark/baseline-release-run1.json
```

采样器的单进程 CPU 百分比按一个逻辑 CPU 为 100% 计算，因此多线程 Python 客户端或 Redis 数值可能超过 100%。若客户端先持续占满可用 CPU，应按前置条件 6 调整测试部署。
