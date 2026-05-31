// --- 模板定义部分 ---

// 字体设置
#let font = (
  main: "IBM Plex Serif",
  mono: "IBM Plex Mono",
  cjk: "Noto Serif CJK SC",
)

// 图标函数
#let icon(path, fill: rgb("#000000")) = box(
  height: 0.7em,
  width: 1.25em,
  align(
    center + horizon,
    image(bytes(read(path).replace("path d", "path fill=\"" + fill.to-hex() + "\" d")), height: 1em),
  ),
)

// 主体框架
#let resume(
  size: 10pt,
  theme-color: rgb("#26267d"),
  margin: (
    top: 1.2cm,
    bottom: 1.2cm,
    left: 1.5cm,
    right: 1.5cm,
  ),
  photograph: "",
  photograph-width: 0em,
  photograph-dx: 0cm,
  photograph-dy: 0cm,
  gutter-width: 0em,
  header-center: false,
  header,
  introduction,
  body,
) = {
  set page(paper: "a4", numbering: none, margin: margin)
  
  show heading: set text(theme-color, 1.15em)

  show heading.where(level: 2): it => stack(
    v(0.4em),
    it,
    v(0.4em),
    line(length: 100%, stroke: 0.05em + theme-color),
    v(0.2em),
  )

  // 列表样式美化
  show list: it => stack(
    spacing: 0.5em,
    ..it.children.map(item => {
      grid(
        columns: (1.2em, 1fr),
        gutter: 0em,
        box({
          h(0.5em)
          text(theme-color)[·] 
        }),
        pad(top: 0.15em, item.body),
      )
    }),
  )

  show link: set text(fill: theme-color)
  set par(justify: true, leading: 0.6em)

  if header-center {
    align(alignment.center, header)
    introduction
  } else {
    grid(
      columns: (auto, 1fr, photograph-width),
      gutter: (gutter-width, 1em),
      [#header #introduction],
    )
  }

  body
}

// 个人信息函数
#let info(color: black, ..infos) = {
  set text(font: (font.mono, font.cjk), fill: color, size: 0.9em)
  set par(justify: false)
  
  let items = infos.pos().map(dir => {
    if type(dir) == str or type(dir) == content {
      dir
    } else if type(dir) == dictionary {
      dir.at("content", default: [])
    } else {
      dir
    }
  })
  
  items.join(h(0.5em) + "·" + h(0.5em))
  v(0.5em)
}

// 日期格式
#let date(body) = text(fill: rgb(128, 128, 128), size: 0.9em, body)

// 技术栈格式
#let tech(body) = block({
  set text(size: 0.95em, fill: rgb("#555555"))
  body
})

// 项目/经历条目
#let item(title, desc, endnote) = {
  v(0.1em)
  grid(
    columns: (55%, 1fr, auto), 
    align: (left, left, right),
    gutter: 0.5em,
    title, desc, endnote,
  )
}

// --- 简历内容部分 ---

// 核心修复：解决中文由于缺少粗体字重导致无法加粗的问题。
// 方案：使用 0.03em 的微弱描边(stroke)来完美模拟加粗效果。
#show strong: it => text(stroke: 0.03em, it)

#show: resume.with(
  [
    // 姓名也添加了微弱描边使其更具辨识度
    #text(size: 1.6em, weight: "black", stroke: 0.03em)[殷智元] 
    #v(0.4em)
  ],
  [
    #text(font: (font.mono, font.cjk), fill: rgb("#444444"), size: 0.95em)[
      男 #h(0.8em) | #h(0.8em) 2741146886\@qq.com #h(0.8em) | #h(0.8em) +86 15597292717 #h(0.8em) | #h(0.8em)
      南京邮电大学 #h(0.8em) | #h(0.8em) 软件工程 #h(0.8em) | #h(0.8em) 2023.09 - 2027.06
    ]
  ]
)

#v(-0.2cm) 

== 求职意向

#grid(
  columns: (55%, 1fr),
  [ *求职岗位*: C++ 后端开发工程师 ],
  [ *到岗时间*: 随时到岗 ],
)
#v(-0.2cm)
== 专业技能

- *编程语言*: 熟练掌握 *C++*，熟悉面向对象编程及泛型编程。深入理解 *C++11/14* 新特性（如智能指针、右值引用、Lambda 表达式、多线程库等），熟悉STL下常见容器。
- *网络编程*: 熟悉 *TCP/IP* 协议栈及 Socket 网络编程，深刻理解 TCP 三次握手、四次挥手及状态机转化过程。
- *并发模型*: 熟悉 Linux 下的 *I/O 多路复用机制*（如 epoll），深入理解其底层机制及 LT/ET 触发模式。
- *操作系统*: 熟悉 *Linux* 常用操作与开发环境，掌握 Shell 脚本编写与基础系统调优。
- *数据库*: 熟悉 *MySQL* 及 *InnoDB* 存储引擎，理解 *B+ Tree* 索引原理、事务隔离级别及基础的 SQL 性能优化。
- *中间件*: 熟悉 *Redis* 核心数据结构及底层原理，理解持久化机制，了解缓存雪崩、击穿、穿透等高并发场景下的应对方案。
- *数据结构与算法*: 熟练掌握常见数据结构（如链表、树、图等）与基础算法。
- *开发辅助*: 熟悉常用Git命令，具备使用 *LLM*（如 ChatGPT, Gemini）辅助预研、代码重构与排障的经验。

#v(-0.2cm)


== 项目经验

#item(
  [#text(size: 1.1em)[*基于 Reactor 模式的高并发网络通讯框架*]],
  [C++ / Linux / Epoll / Redis / 多线程],
  date[2026.03 - 至今]
)
#tech[*项目描述：*提取 Nginx 核心架构实现的高性能 C++ 网络服务器。基于 Master-Worker 多进程与 Epoll 事件驱动模型，实现网络 I/O 与业务逻辑的彻底解耦。]

#v(0.2em)
#tech[*核心技术与工作：*]
- *协议解析与防粘包*：设计“*包头+包体*”自定义应用层协议，在 Epoll (LT 模式) 读事件中基于*有限状态机*精准解析 TCP 字节流，解决网络传输中的粘包与半包问题。
- *连接池与延迟回收*：预分配 Socket 内存池实现资源复用，避免高频内存碎片；针对高并发下由于 Socket 状态复用导致的“串包”和脏数据隐患，引入*延迟回收*策略，保障底层连接安全释放。
- *无锁队列与线程池调度*：封装 POSIX 线程与信号量，构建业务工作线程池。基于 `std::atomic` 与互斥机制实现收发消息队列的并发控制，单机无状态极限吞吐量达 2.7 万 QPS。
- *L1+L2 分布式防 CC 限流架构*：针对单机限流在多进程下失效及高频攻击造成的“业务线程饥饿”问题，设计分布式双层防御体系：
- *L1 本地极速拦截*：位于网关最前沿，在底层 `epoll_wait` 唤醒初期查询本地无锁黑名单，直接断开恶意连接（TCP RST），实现零内存拷贝拦截。
- *L2 Redis 全局校验*：引入 `hiredis` 连接池，通过下发 Lua 脚本保障 INCR 与 EXPIRE 的原子性进行精确探测；超限后动态跨层联动，更新 L1 黑名单。
- *压测性能表现*：经实测，双层限流架构成功防御 40 万次并发攻击并维持极低 CPU 损耗；在全链路 Redis 校验下，单机业务吞吐量仍高达*1.9 万 QPS*且几乎无错误。
- 
#v(0.2em)
#tech[*个人收获：*]
- 深刻理解了 Reactor 网络模型原理与 Linux 操作系统底层的网络 I/O 调度及进程管理机制。
- 积累了高并发场景下的排障与调优经验，培养了通过多级缓存策略（L1 本地+L2 分布式）进行系统降级、保护核心业务资源的架构思维。

#item(
  [#text(size: 1.1em)[*Epoll 服务端配套 Qt6 TCP 客户端（MVP）*]],
  [C++ / Qt6 / QTcpSocket / 自定义协议],
  date[2026.03 - 至今]
)
#tech[*项目描述：*Windows 桌面客户端，对接自研 Linux epoll 服务端（VMware 联调）。按服务端「包头+包体」协议用 `QTcpSocket` 实现连接管理与 Ping 心跳。]
#v(0.2em)
#tech[*核心工作：*]
- *协议封装*：`Protocol` 层实现 Ping 大端组包/解包（8 字节 `COMM_PKG_HEADER`），与 Python 压测脚本及服务端包头定义一致。
- *网络层*：`TcpClient` 封装连接/断开、`readyRead` 接收缓冲；信号槽驱动 UI，支持 5 秒 `QTimer` 自动心跳。
- *联调*：本机 Qt 客户端连接 VM 内 nginx:8080，日志验证 Ping 往返；工程见仓库 `qt-client/`（CMake + Qt6 Widgets/Network）。

#item(
  [#text(size: 1.1em)[*Roc Toolkit 开源音频流传输引擎研究与测试*]],
  [C++ / 网络协议 / 跨平台],
  date[2025.12 - 2026.02]
)
#tech[*项目背景：*Roc Toolkit 是一个专注于低延迟、高可靠性的实时音频流传输开源 C/C++ 库。本项目旨在通过编译部署该引擎源码，深入剖析 RTP/RTCP 协议栈底层交互，并验证前向纠错 (FEC) 算法在弱网环境下的表现。]

#v(0.2em)
#tech[*核心工作：*]
- *C API 深度集成与自定义收发端构建*：脱离现成命令行工具，基于底层 C API 独立开发了音频 Sender 与 Receiver 测试程序。深入梳理了音频流的物理分发逻辑，通过精准控制 `roc_context` (全局上下文) 与 `roc_sender` 的生命周期，实现了本地跨进程的音频流抓取、RTP 打包与网络分发。
- *基于 RS 码的 FEC 弱网对抗实践*：在本地利用 Linux 流量控制工具 (tc) 模拟丢包环境，并引入 LDPC/RS8M 前向纠错机制。深入剖析了其“以空间换时间”的核心思想：在发送端通过线性代数矩阵运算生成冗余包，使接收端能够在 UDP 协议不可靠、且不依赖 TCP 延迟重传的情况下，仅凭收到的部分数据包即可完成矩阵求解与解码，实现低延迟下的零卡顿音频恢复。

#v(0.2em)
#tech[*个人收获：*]
- *工业级接口设计*：体会了成熟开源库如何利用“不透明指针 (Opaque Pointer/Pimpl 模式)”隐藏结构体内部细节，从而在保证跨平台 ABI 稳定性的同时，实现接口与底层复杂 C++ 实现的完美解耦。
- *资源所有权与并发模型*：通过严格配对 `open` 与 `close` API，强化了对 C/C++ 显式资源管理与“所有权 (Ownership) 转移”的工程意识；理解了 Context 对象作为共享上下文，在协调多个网络工作线程及内存池时的无锁/并发安全设计。


/* 注释掉校内经历模块
== 校内经历

#item(
  [*计软网安院科协*],
  [软件研发部部员],
  date[2023.11 - 2024.06]
)
- 参与部门技术沙龙，协助组织校内计算机基础知识竞赛。

#item(
  [*易班工作中心*],
  [APP 部部员],
  date[2023.11 - 2024.06]
)
- 负责公众号技术文章选题、编辑与分发，提升校内技术交流氛围。
*/

== 奖项证书

//- #date[2023.11] 鼎新杯计算机基础竞赛 · 三等奖
//- #date[2024.11] Bit 杯程序设计竞赛 · 三等奖
- #date[2023.12] 英语四级 (CET-4) 证书 

/* 注释掉自我评价模块
== 自我评价

- *压力管理*: 能够应对高强度的开发节奏，在压力环境下保持逻辑清晰，情绪稳定。
- *快速学习*: 具备优秀的自驱力与好奇心，能够快速上手复杂的新技术栈并产出结果。
- *团队协作*: 性格开朗，沟通顺畅，能准确理解业务需求并给出及时的技术反馈。
*/