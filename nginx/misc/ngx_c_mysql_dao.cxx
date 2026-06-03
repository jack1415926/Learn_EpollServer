
#include "ngx_c_mysql_dao.h"

#include "ngx_c_mysql_connpool.h"
#include "ngx_func.h"

#include <cstring>
#include <string>

// 使用预处理语句（Prepared Statement）防止 SQL 注入，面试可重点讲
static int ExecRegisterStmt(MYSQL *conn, const char *username, const char *password)
{
    const char *sql = "INSERT INTO users (username, password) VALUES (?, ?)";

    MYSQL_STMT *stmt = mysql_stmt_init(conn);
    if (stmt == nullptr) {
        ngx_log_stderr(0, "RegisterUser: mysql_stmt_init 失败");
        return NGX_DB_ERR_FAILED;
    }

    if (mysql_stmt_prepare(stmt, sql, static_cast<unsigned long>(strlen(sql))) != 0) {
        ngx_log_stderr(0, "RegisterUser: prepare 失败: %s", mysql_stmt_error(stmt));
        mysql_stmt_close(stmt);
        return NGX_DB_ERR_FAILED;
    }

    MYSQL_BIND bind[2];
    memset(bind, 0, sizeof(bind));

    unsigned long username_len = static_cast<unsigned long>(strlen(username));
    unsigned long password_len = static_cast<unsigned long>(strlen(password));

    bind[0].buffer_type = MYSQL_TYPE_STRING;
    bind[0].buffer = const_cast<char *>(username);
    bind[0].buffer_length = username_len;
    bind[0].length = &username_len;

    bind[1].buffer_type = MYSQL_TYPE_STRING;
    bind[1].buffer = const_cast<char *>(password);
    bind[1].buffer_length = password_len;
    bind[1].length = &password_len;

    if (mysql_stmt_bind_param(stmt, bind) != 0) {
        ngx_log_stderr(0, "RegisterUser: bind 失败: %s", mysql_stmt_error(stmt));
        mysql_stmt_close(stmt);
        return NGX_DB_ERR_FAILED;
    }

    if (mysql_stmt_execute(stmt) != 0) {
        unsigned int err = mysql_stmt_errno(stmt);
        ngx_log_stderr(0, "RegisterUser: execute 失败(%u): %s", err, mysql_stmt_error(stmt));
        mysql_stmt_close(stmt);
        if (err == 1062) {
            return NGX_DB_ERR_DUPLICATE;
        }
        return NGX_DB_ERR_FAILED;
    }

    mysql_stmt_close(stmt);
    return NGX_DB_OK;
}

static int ExecLoginStmt(MYSQL *conn, const char *username, const char *password)
{
    const char *sql = "SELECT id FROM users WHERE username = ? AND password = ? LIMIT 1";

    MYSQL_STMT *stmt = mysql_stmt_init(conn);
    if (stmt == nullptr) {
        return NGX_DB_ERR_FAILED;
    }

    if (mysql_stmt_prepare(stmt, sql, static_cast<unsigned long>(strlen(sql))) != 0) {
        ngx_log_stderr(0, "VerifyLogin: prepare 失败: %s", mysql_stmt_error(stmt));
        mysql_stmt_close(stmt);
        return NGX_DB_ERR_FAILED;
    }

    MYSQL_BIND bind[2];
    memset(bind, 0, sizeof(bind));

    unsigned long username_len = static_cast<unsigned long>(strlen(username));
    unsigned long password_len = static_cast<unsigned long>(strlen(password));

    bind[0].buffer_type = MYSQL_TYPE_STRING;
    bind[0].buffer = const_cast<char *>(username);
    bind[0].buffer_length = username_len;
    bind[0].length = &username_len;

    bind[1].buffer_type = MYSQL_TYPE_STRING;
    bind[1].buffer = const_cast<char *>(password);
    bind[1].buffer_length = password_len;
    bind[1].length = &password_len;

    if (mysql_stmt_bind_param(stmt, bind) != 0) {
        mysql_stmt_close(stmt);
        return NGX_DB_ERR_FAILED;
    }

    if (mysql_stmt_execute(stmt) != 0) {
        ngx_log_stderr(0, "VerifyLogin: execute 失败: %s", mysql_stmt_error(stmt));
        mysql_stmt_close(stmt);
        return NGX_DB_ERR_FAILED;
    }

    MYSQL_BIND result_bind;
    memset(&result_bind, 0, sizeof(result_bind));
    int64_t user_id = 0;
    bool is_null = false;
    result_bind.buffer_type = MYSQL_TYPE_LONGLONG;
    result_bind.buffer = reinterpret_cast<char *>(&user_id);
    result_bind.is_null = &is_null;

    if (mysql_stmt_bind_result(stmt, &result_bind) != 0) {
        mysql_stmt_close(stmt);
        return NGX_DB_ERR_FAILED;
    }

    int fetch_code = mysql_stmt_fetch(stmt);
    mysql_stmt_close(stmt);

    if (fetch_code == 0 && !is_null) {
        return NGX_DB_OK;
    }
    if (fetch_code == MYSQL_NO_DATA) {
        return NGX_DB_ERR_FAILED;
    }
    return NGX_DB_ERR_FAILED;
}

int CMysqlDao::RegisterUser(MYSQL *conn, const char *username, const char *password)
{
    if (conn == nullptr || username == nullptr || password == nullptr) {
        return NGX_DB_ERR_FAILED;
    }
  // 演示项目明文存密码；生产环境应使用 bcrypt 等哈希
    return ExecRegisterStmt(conn, username, password);
}

int CMysqlDao::VerifyLogin(MYSQL *conn, const char *username, const char *password)
{
    if (conn == nullptr || username == nullptr || password == nullptr) {
        return NGX_DB_ERR_FAILED;
    }
    return ExecLoginStmt(conn, username, password);
}
