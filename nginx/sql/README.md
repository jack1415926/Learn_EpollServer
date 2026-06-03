# MySQL 持久化联调说明

## 1. 安装依赖（Ubuntu / Debian VM）

```bash
sudo apt update
sudo apt install -y mysql-server libmysqlclient-dev
sudo systemctl start mysql
```

## 2. 建库建表与业务账号

```bash
cd ~/Learn_EpollServer-main/nginx/sql
sudo mysql -u root -p < init_users.sql
```

默认创建：

- 库：`epoll_db`
- 用户：`epoll_user` / `epoll_pass`（与 [`nginx.conf`](../nginx.conf) `[Mysql]` 段一致）

## 3. 配置 nginx.conf

确认 `[Mysql]` 段与上面账号一致，例如：

```ini
[Mysql]
MysqlHost = 127.0.0.1
MysqlPort = 3306
MysqlUser = epoll_user
MysqlPassword = epoll_pass
MysqlDatabase = epoll_db
MysqlPoolSize = 16
```

## 4. 编译与启动

```bash
cd ~/Learn_EpollServer-main/nginx
make clean && make
./nginx
```

日志中应出现：`CMysqlConnPool::Init() 成功`（每个 Worker 一条）。

**注意**：连接池在 **Worker fork 之后** 初始化，不要在 `main()` 里 Init。

## 5. 验证数据

注册/登录成功后：

```bash
mysql -u epoll_user -pepoll_pass -e "SELECT id,username,created_at FROM epoll_db.users;"
```

## 6. 回包错误码（`iType` / `iResult`）

| 值 | 含义 |
|----|------|
| 0 | 成功 |
| 1 | 用户名重复（注册） |
| 2 | SQL/业务失败 |
| 3 | 连接池获取超时 |

## 7. 连接数估算

`WorkerProcesses × MysqlPoolSize` = 总 MySQL 连接上限（例如 4×16=64），请保证 `max_connections` 足够。

## 8. 面试要点

- `std::shared_ptr<MYSQL>` + 自定义 Deleter：作用域结束 **归还队列**，不 `mysql_close`
- `std::unique_lock` + `condition_variable::wait_for`：空闲连接等待与超时
- 预处理语句 `MYSQL_STMT`：防 SQL 注入
- fork 后 per-worker 独立连接池
