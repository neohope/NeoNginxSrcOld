# Nginx 0.1.0 源码阅读指南

本仓库包含 Nginx 早期版本 **0.1.0** 的完整 C 源码，适合用于学习 Nginx 的整体架构、事件驱动模型以及 HTTP/IMAP 代理实现。

本文档从源码阅读的角度，简要介绍：

- 项目整体目录结构
- 进程/事件/请求处理的主要流程
- 关键模块与核心文件
- 推荐的阅读顺序

---

## 1. 目录结构概览

顶层目录结构大致如下（省略部分细节）：

- `auto/`：自动化配置与构建脚本
  - 探测编译器特性、操作系统特性、第三方库支持（PCRE/OpenSSL/zlib 等）
  - 生成最终 `Makefile` 和平台相关的配置文件
- `conf/`：运行时配置文件
  - `nginx.conf`：主配置文件，是 Nginx 启动时加载的核心配置
  - `mime.types`：MIME 类型映射
  - `koi-win`：字符集映射示例
- `docs/`：官方文档与变更日志
  - `text/README`、`text/LICENSE` 等
  - `xml/`、`xslt/`：用于生成变更文档的 XML/XSLT 文件
- `src/`：全部 C 源码所在目录，是代码阅读的重点
  - `core/`：核心框架（启动、内存、日志、配置解析、连接管理等）
  - `event/`：事件模型与多种 I/O 复用后端（kqueue/epoll/select 等）
  - `http/`：HTTP 协议处理与各类 HTTP 模块
  - `imap/`：IMAP 代理支持
  - `os/`：操作系统相关的抽象层（`unix/` 与 `win32/`）

在阅读源码时，可以把 `src/` 理解为：

- `core/` + `os/` 提供基础运行时与平台适配层
- `event/` 提供通用事件循环和 I/O 调度
- `http/` / `imap/` 在上述基础上实现具体协议和业务逻辑

---

## 2. 启动与配置流程（主流程）

Nginx 的主入口在 [nginx.c](src/core/nginx.c) 中：

- `main()` 函数完成以下关键步骤：
  - 初始化时间、正则库、日志系统
  - 解析命令行参数（如 `-c` 指定配置文件、`-t` 测试配置等）
  - 调用 `ngx_os_init()` 做操作系统相关初始化（见 `src/os/`）
  - 调用 `ngx_init_cycle()` 解析配置文件、创建 `ngx_cycle_t` 运行时上下文
  - 根据配置决定是：
    - 单进程模式：`ngx_single_process_cycle()`
    - master-worker 模式：`ngx_master_process_cycle()`（Unix 平台）

### 2.1 `ngx_cycle` 与配置解析

核心类型 `ngx_cycle_t` 定义在 [ngx_cycle.h](src/core/ngx_cycle.h)，用于描述整个 Nginx 实例的运行状态：

- 维护：监听端口、内存池、日志、模块配置、打开的文件等
- 是配置解析、模块初始化和运行时资源共享的“根对象”

相关关键文件：

- [ngx_conf_file.c](src/core/ngx_conf_file.c)
  - 实现指令解析器，负责解析 `nginx.conf` 中的配置指令
- [ngx_cycle.c](src/core/ngx_cycle.c)
  - `ngx_init_cycle()`：解析配置、初始化模块、打开监听端口
  - 负责在重载配置时构造新 `cycle` 并平滑切换

### 2.2 进程模型

Unix 平台：

- [ngx_process_cycle.c](src/os/unix/ngx_process_cycle.c)
  - `ngx_master_process_cycle()`：master 进程主循环
    - 负责 fork worker 进程、处理信号（重载、退出、重启）
  - `ngx_worker_process_cycle()`：worker 进程主循环
    - 在其中驱动事件模块、处理网络连接与请求
- [ngx_process.c](src/os/unix/ngx_process.c)
  - 负责 spawn/respawn 子进程、维护子进程表
- [ngx_channel.c](src/os/unix/ngx_channel.c)
  - 实现 master 与 worker 之间的进程间通信（本地 socketpair）

Windows 平台：

- [ngx_win32_init.c](src/os/win32/ngx_win32_init.c)
  - 初始化 WinSock、系统版本、CPU 数等
- [ngx_process_cycle.c](src/os/win32/ngx_process_cycle.c)
  - 仅实现单进程模式（早期版本不支持 Windows 下的 master-worker）
- [ngx_service.c](src/os/win32/ngx_service.c)
  - 将 Nginx 作为 Windows 服务运行

---

## 3. 事件驱动与网络 IO

事件子系统位于 [src/event](src/event)：它负责实现跨平台的事件循环和网络 IO 抽象。

### 3.1 通用事件框架

核心文件：

- [ngx_event.c](src/event/ngx_event.c)
  - 定义事件模块的配置指令（如 `events`、`worker_connections`）
  - 负责初始化所选事件模块（如 epoll/kqueue）
- [ngx_event_timer.c](src/event/ngx_event_timer.c)
  - 管理定时器（基于最小堆/红黑树），用于超时控制
- [ngx_event_posted.c](src/event/ngx_event_posted.c)
  - 处理异步回调、延后执行事件
- [ngx_event_accept.c](src/event/ngx_event_accept.c)
  - 接受新连接，创建 `ngx_connection_t` 并挂到事件循环中

### 3.2 多种 I/O 复用实现

具体的事件后端实现位于 [src/event/modules](src/event/modules)：

- `ngx_epoll_module.c`：Linux epoll 实现
- `ngx_kqueue_module.c`：FreeBSD kqueue 实现
- `ngx_rtsig_module.c`：基于实时信号的事件
- `ngx_select_module.c` / `ngx_poll_module.c`：通用 select/poll 实现
- `ngx_iocp_module.c`（Windows）：IOCP 模型的初始版本

这些模块都实现了统一的 `ngx_event_actions` 接口，被 `ngx_event.c` 统一调度。

### 3.3 OS 抽象层中的网络 IO

在 `src/os/` 下，封装了具体平台的 socket 读写：

- Unix：
  - [ngx_recv.c](src/os/unix/ngx_recv.c)、[ngx_send.c](src/os/unix/ngx_send.c)
  - [ngx_readv_chain.c](src/os/unix/ngx_readv_chain.c)、[ngx_writev_chain.c](src/os/unix/ngx_writev_chain.c)
  - [ngx_linux_sendfile_chain.c](src/os/unix/ngx_linux_sendfile_chain.c)、`ngx_freebsd_sendfile_chain.c` 等
- Windows：
  - [ngx_wsarecv.c](src/os/win32/ngx_wsarecv.c)、[ngx_wsarecv_chain.c](src/os/win32/ngx_wsarecv_chain.c)
  - [ngx_wsasend_chain.c](src/os/win32/ngx_wsasend_chain.c)

通过这些文件，事件模块可以在不关心具体系统调用细节的前提下完成高性能 IO。

---

## 4. HTTP 请求处理流程

HTTP 子系统位于 [src/http](src/http)，早期版本已经具备比较完整的请求处理流水线。

### 4.1 HTTP 框架与上下文

关键文件：

- [ngx_http.c](src/http/ngx_http.c)
  - HTTP 模块的入口与配置解析入口
  - 创建 HTTP 相关的 main/server/location 配置结构
- [ngx_http_request.c](src/http/ngx_http_request.c)
  - 定义 `ngx_http_request_t`，实现 HTTP 请求的状态机
  - 处理请求生命周期：
    - 接收请求行与头部
    - 选择虚拟主机和 location
    - 调用各阶段的模块（rewrite/access/content/filter）
- [ngx_http_core_module.c](src/http/ngx_http_core_module.c)
  - 定义核心指令（`server`、`location`、`root` 等）
  - 负责请求的路由与阶段调度

### 4.2 请求解析与响应

相关辅助文件：

- [ngx_http_parse.c](src/http/ngx_http_parse.c)
  - 实现 HTTP 请求行和头部的解析
- [ngx_http_parse_time.c](src/http/ngx_http_parse_time.c)
  - 解析 HTTP 时间头（`If-Modified-Since` 等）
- [ngx_http_request_body.c](src/http/ngx_http_request_body.c)
  - 请求体读取（文件上传、POST 数据）
- [ngx_http_special_response.c](src/http/ngx_http_special_response.c)
  - 处理错误页、重定向等特殊响应

### 4.3 过滤链与模块

Nginx 通过 filter 链组合输出：

- [ngx_http_header_filter.c](src/http/ngx_http_header_filter.c)
  - 负责构造 HTTP 响应头
- [ngx_http_write_filter.c](src/http/ngx_http_write_filter.c)
  - 最终将缓冲区链写入 socket
- [ngx_http_copy_filter.c](src/http/ngx_http_copy_filter.c)
  - 在文件/内存缓冲间做统一的拷贝处理

HTTP 模块示例（位于 `src/http/modules/`）：

- `ngx_http_static_handler.c`：静态文件服务
- `ngx_http_index_handler.c`：index 处理（如 `index index.html`）
- `proxy/` 子目录：早期 HTTP 反向代理实现（缓存、上游服务器等）

---

## 5. IMAP 代理模块

IMAP 相关代码位于 [src/imap](src/imap)：

- [ngx_imap.c](src/imap/ngx_imap.c)
  - IMAP 模块入口、配置解析、上下文创建
- [ngx_imap_handler.c](src/imap/ngx_imap_handler.c)
  - 处理 IMAP 会话状态机与命令
- [ngx_imap_proxy.c](src/imap/ngx_imap_proxy.c)
  - 实现 IMAP 反向代理，将客户端连接转发到后端邮件服务器
- [ngx_imap_parse.c](src/imap/ngx_imap_parse.c)
  - 解析 IMAP 协议命令

IMAP 的整体结构与 HTTP 类似：

- 底层仍然依赖 `event` + `os` 的事件与 IO 抽象
- 上层实现协议解析与会话状态管理

---

## 6. 核心工具与基础设施

在 `src/core/` 下有大量通用数据结构和工具，是 Nginx 框架的基础：

- 内存管理
  - [ngx_palloc.c](src/core/ngx_palloc.c)：内存池分配器（`ngx_pool_t`）
  - [ngx_array.c](src/core/ngx_array.c)、[ngx_list.c](src/core/ngx_list.c)：动态数组与链表
  - [ngx_slab.c](src/core/ngx_slab.c)：slab 分配器（多进程共享内存上的分配）
- 日志与错误处理
  - [ngx_log.c](src/core/ngx_log.c)：日志级别、日志输出、错误码转换
- 字符串与工具
  - [ngx_string.c](src/core/ngx_string.c)：自定义字符串类型 `ngx_str_t` 及相关操作
  - [ngx_times.c](src/core/ngx_times.c)：时间缓存与更新时间逻辑
- 容器与算法
  - [ngx_rbtree.c](src/core/ngx_rbtree.c)：红黑树实现
  - [ngx_radix_tree.c](src/core/ngx_radix_tree.c)：前缀树，用于 IP 地址等前缀匹配

这些基础模块在 Nginx 全局复用，是阅读其他模块前的推荐预备知识。

---

## 7. 推荐阅读顺序

如果希望系统地理解 Nginx 0.1.0 的设计，可以按以下顺序阅读：

1. **主流程与配置**
   - [nginx.c](src/core/nginx.c)
   - [ngx_cycle.c](src/core/ngx_cycle.c)
   - [ngx_conf_file.c](src/core/ngx_conf_file.c)
2. **基础设施**
   - 内存池：[ngx_palloc.c](src/core/ngx_palloc.c)
   - 日志：[ngx_log.c](src/core/ngx_log.c)
   - 字符串与时间：[ngx_string.c](src/core/ngx_string.c)、[ngx_times.c](src/core/ngx_times.c)
3. **进程与 OS 层**
   - Unix：`src/os/unix/ngx_process*.c`、`ngx_socket.c`、`ngx_linux_sendfile_chain.c`
   - Windows（可选）：`src/os/win32/ngx_win32_init.c`、`ngx_wsarecv*.c`
4. **事件子系统**
   - [ngx_event.c](src/event/ngx_event.c)
   - `src/event/modules/` 下对应平台的事件模块（如 `ngx_epoll_module.c`）
5. **HTTP 子系统**
   - [ngx_http.c](src/http/ngx_http.c)
   - [ngx_http_request.c](src/http/ngx_http_request.c)
   - [ngx_http_core_module.c](src/http/ngx_http_core_module.c)
   - 常见模块：`ngx_http_static_handler.c`、`ngx_http_proxy_*` 等
6. **IMAP（可选）**
   - `src/imap/` 下的几个文件，理解 Nginx 在非 HTTP 协议上的复用能力

---

## 8. 备注

- 本仓库为早期版本，部分平台/功能仍为 stub 或实验性质实现（尤其是 Windows 支持）。
- 阅读时建议对照 `conf/nginx.conf`，从配置项到代码路径建立映射，有助于理解模块化设计与配置系统的工作方式。

如需针对某个模块（例如 HTTP 代理、事件模块、共享内存等）进行更细致的拆解，可以在对应目录下逐个文件深入阅读。

