#include "database.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <cstdlib>

static void parseEnvLine(const std::string& line, std::string& outKey, std::string& outVal)
{
    if (line.empty() || line[0] == '#') return;
    auto eqPos = line.find('=');
    if (eqPos == std::string::npos) return;
    outKey = line.substr(0, eqPos);
    outVal = line.substr(eqPos + 1);
    // trim carriage return / spaces
    while (!outKey.empty() && (outKey.back() == ' ' || outKey.back() == '\r')) outKey.pop_back();
    while (!outKey.empty() && outKey.front() == ' ') outKey.erase(outKey.begin());
    while (!outVal.empty() && (outVal.back() == ' ' || outVal.back() == '\r')) outVal.pop_back();
    while (!outVal.empty() && outVal.front() == ' ') outVal.erase(outVal.begin());
}

static void tryLoadEnvFile(const std::string& filepath)
{
    std::ifstream file(filepath);
    if (!file.is_open()) return;
    std::string line;
    while (std::getline(file, line))
    {
        std::string key, val;
        parseEnvLine(line, key, val);
        if (!key.empty())
        {
            _putenv_s(key.c_str(), val.c_str());
        }
    }
}

Database::Database() : conn(nullptr), port(3306)
{
    loadConfig();
}

Database::~Database()
{
    disconnect();
}

void Database::loadConfig()
{
    // Try to load .env from several plausible locations
    tryLoadEnvFile(".env");
    tryLoadEnvFile("../.env");
    tryLoadEnvFile("backend/.env");

    char* envHost = getenv("DB_HOST");
    host = envHost ? envHost : "127.0.0.1";

    char* envPort = getenv("DB_PORT");
    port = envPort ? std::atoi(envPort) : 3306;

    char* envUser = getenv("DB_USER");
    user = envUser ? envUser : "root";

    char* envPass = getenv("DB_PASSWORD");
    password = envPass ? envPass : "";

    char* envName = getenv("DB_NAME");
    dbName = envName ? envName : "subscription_management";
}

bool Database::connect()
{
    std::lock_guard<std::mutex> lock(dbMutex);
    loadConfig();

    if (conn)
    {
        mysql_close(conn);
        conn = nullptr;
    }

    conn = mysql_init(nullptr);
    if (!conn)
    {
        lastError = "mysql_init failed: insufficient memory";
        return false;
    }

    // First try connecting directly to the targeted database
    if (mysql_real_connect(conn, host.c_str(), user.c_str(), password.c_str(),
                           dbName.c_str(), port, nullptr, 0))
    {
        return initializeTables();
    }

    // If that failed because the database does not exist, connect without database and create it
    if (mysql_real_connect(conn, host.c_str(), user.c_str(), password.c_str(),
                           nullptr, port, nullptr, 0))
    {
        std::string createDbQuery = "CREATE DATABASE IF NOT EXISTS " + dbName + ";";
        if (mysql_query(conn, createDbQuery.c_str()) != 0)
        {
            lastError = mysql_error(conn);
            return false;
        }

        if (mysql_select_db(conn, dbName.c_str()) != 0)
        {
            lastError = mysql_error(conn);
            return false;
        }

        return initializeTables();
    }

    lastError = mysql_error(conn);
    return false;
}

bool Database::initializeTables()
{
    const char* schemaQuery =
        "CREATE TABLE IF NOT EXISTS users ("
        "  user_id INT AUTO_INCREMENT PRIMARY KEY,"
        "  name VARCHAR(100) NOT NULL,"
        "  email VARCHAR(150) UNIQUE NOT NULL,"
        "  password_hash VARCHAR(255) NOT NULL,"
        "  role ENUM('ADMIN', 'STAFF', 'CUSTOMER') NOT NULL,"
        "  status ENUM('ACTIVE', 'INACTIVE') DEFAULT 'ACTIVE',"
        "  must_change_password BOOLEAN DEFAULT FALSE,"
        "  created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP"
        ");";

    if (mysql_query(conn, schemaQuery) != 0)
    {
        lastError = mysql_error(conn);
        return false;
    }
    return true;
}

void Database::disconnect()
{
    std::lock_guard<std::mutex> lock(dbMutex);
    if (conn)
    {
        mysql_close(conn);
        conn = nullptr;
    }
}

bool Database::isConnected()
{
    std::lock_guard<std::mutex> lock(dbMutex);
    return (conn != nullptr && mysql_ping(conn) == 0);
}

std::string Database::getLastError() const
{
    return lastError;
}

bool Database::findUserByEmail(const std::string& email, UserRecord& outUser)
{
    std::lock_guard<std::mutex> lock(dbMutex);
    if (!conn) return false;

    const char* sql = "SELECT user_id, name, email, password_hash, role, status, must_change_password, created_at "
                      "FROM users WHERE email = ? LIMIT 1";

    MYSQL_STMT* stmt = mysql_stmt_init(conn);
    if (!stmt) return false;

    if (mysql_stmt_prepare(stmt, sql, (unsigned long)strlen(sql)) != 0)
    {
        lastError = mysql_stmt_error(stmt);
        mysql_stmt_close(stmt);
        return false;
    }

    MYSQL_BIND paramBind[1];
    std::memset(paramBind, 0, sizeof(paramBind));
    unsigned long emailLen = (unsigned long)email.length();

    paramBind[0].buffer_type = MYSQL_TYPE_STRING;
    paramBind[0].buffer = (void*)email.c_str();
    paramBind[0].buffer_length = emailLen;
    paramBind[0].length = &emailLen;

    if (mysql_stmt_bind_param(stmt, paramBind) != 0)
    {
        lastError = mysql_stmt_error(stmt);
        mysql_stmt_close(stmt);
        return false;
    }

    if (mysql_stmt_execute(stmt) != 0)
    {
        lastError = mysql_stmt_error(stmt);
        mysql_stmt_close(stmt);
        return false;
    }

    int id = 0;
    char nameBuf[128] = {0};
    char emailBuf[160] = {0};
    char passBuf[260] = {0};
    char roleBuf[32] = {0};
    char statusBuf[32] = {0};
    my_bool mustChange = 0;
    char createdAtBuf[64] = {0};

    unsigned long nameLen = 0, emailOutLen = 0, passLen = 0, roleLen = 0, statusLen = 0, createdLen = 0;

    MYSQL_BIND resultBind[8];
    std::memset(resultBind, 0, sizeof(resultBind));

    resultBind[0].buffer_type = MYSQL_TYPE_LONG;
    resultBind[0].buffer = &id;

    resultBind[1].buffer_type = MYSQL_TYPE_STRING;
    resultBind[1].buffer = nameBuf;
    resultBind[1].buffer_length = sizeof(nameBuf);
    resultBind[1].length = &nameLen;

    resultBind[2].buffer_type = MYSQL_TYPE_STRING;
    resultBind[2].buffer = emailBuf;
    resultBind[2].buffer_length = sizeof(emailBuf);
    resultBind[2].length = &emailOutLen;

    resultBind[3].buffer_type = MYSQL_TYPE_STRING;
    resultBind[3].buffer = passBuf;
    resultBind[3].buffer_length = sizeof(passBuf);
    resultBind[3].length = &passLen;

    resultBind[4].buffer_type = MYSQL_TYPE_STRING;
    resultBind[4].buffer = roleBuf;
    resultBind[4].buffer_length = sizeof(roleBuf);
    resultBind[4].length = &roleLen;

    resultBind[5].buffer_type = MYSQL_TYPE_STRING;
    resultBind[5].buffer = statusBuf;
    resultBind[5].buffer_length = sizeof(statusBuf);
    resultBind[5].length = &statusLen;

    resultBind[6].buffer_type = MYSQL_TYPE_TINY;
    resultBind[6].buffer = &mustChange;

    resultBind[7].buffer_type = MYSQL_TYPE_STRING;
    resultBind[7].buffer = createdAtBuf;
    resultBind[7].buffer_length = sizeof(createdAtBuf);
    resultBind[7].length = &createdLen;

    if (mysql_stmt_bind_result(stmt, resultBind) != 0)
    {
        lastError = mysql_stmt_error(stmt);
        mysql_stmt_close(stmt);
        return false;
    }

    mysql_stmt_store_result(stmt);

    bool found = false;
    if (mysql_stmt_fetch(stmt) == 0)
    {
        found = true;
        outUser.userId = id;
        outUser.name = std::string(nameBuf, nameLen);
        outUser.email = std::string(emailBuf, emailOutLen);
        outUser.passwordHash = std::string(passBuf, passLen);
        outUser.role = std::string(roleBuf, roleLen);
        outUser.status = std::string(statusBuf, statusLen);
        outUser.mustChangePassword = (mustChange != 0);
        outUser.createdAt = std::string(createdAtBuf, createdLen);
    }

    mysql_stmt_close(stmt);
    return found;
}

bool Database::findUserById(int userId, UserRecord& outUser)
{
    std::lock_guard<std::mutex> lock(dbMutex);
    if (!conn) return false;

    const char* sql = "SELECT user_id, name, email, password_hash, role, status, must_change_password, created_at "
                      "FROM users WHERE user_id = ? LIMIT 1";

    MYSQL_STMT* stmt = mysql_stmt_init(conn);
    if (!stmt) return false;

    if (mysql_stmt_prepare(stmt, sql, (unsigned long)strlen(sql)) != 0)
    {
        lastError = mysql_stmt_error(stmt);
        mysql_stmt_close(stmt);
        return false;
    }

    MYSQL_BIND paramBind[1];
    std::memset(paramBind, 0, sizeof(paramBind));
    paramBind[0].buffer_type = MYSQL_TYPE_LONG;
    paramBind[0].buffer = &userId;

    if (mysql_stmt_bind_param(stmt, paramBind) != 0)
    {
        lastError = mysql_stmt_error(stmt);
        mysql_stmt_close(stmt);
        return false;
    }

    if (mysql_stmt_execute(stmt) != 0)
    {
        lastError = mysql_stmt_error(stmt);
        mysql_stmt_close(stmt);
        return false;
    }

    int id = 0;
    char nameBuf[128] = {0};
    char emailBuf[160] = {0};
    char passBuf[260] = {0};
    char roleBuf[32] = {0};
    char statusBuf[32] = {0};
    my_bool mustChange = 0;
    char createdAtBuf[64] = {0};

    unsigned long nameLen = 0, emailOutLen = 0, passLen = 0, roleLen = 0, statusLen = 0, createdLen = 0;

    MYSQL_BIND resultBind[8];
    std::memset(resultBind, 0, sizeof(resultBind));

    resultBind[0].buffer_type = MYSQL_TYPE_LONG;
    resultBind[0].buffer = &id;

    resultBind[1].buffer_type = MYSQL_TYPE_STRING;
    resultBind[1].buffer = nameBuf;
    resultBind[1].buffer_length = sizeof(nameBuf);
    resultBind[1].length = &nameLen;

    resultBind[2].buffer_type = MYSQL_TYPE_STRING;
    resultBind[2].buffer = emailBuf;
    resultBind[2].buffer_length = sizeof(emailBuf);
    resultBind[2].length = &emailOutLen;

    resultBind[3].buffer_type = MYSQL_TYPE_STRING;
    resultBind[3].buffer = passBuf;
    resultBind[3].buffer_length = sizeof(passBuf);
    resultBind[3].length = &passLen;

    resultBind[4].buffer_type = MYSQL_TYPE_STRING;
    resultBind[4].buffer = roleBuf;
    resultBind[4].buffer_length = sizeof(roleBuf);
    resultBind[4].length = &roleLen;

    resultBind[5].buffer_type = MYSQL_TYPE_STRING;
    resultBind[5].buffer = statusBuf;
    resultBind[5].buffer_length = sizeof(statusBuf);
    resultBind[5].length = &statusLen;

    resultBind[6].buffer_type = MYSQL_TYPE_TINY;
    resultBind[6].buffer = &mustChange;

    resultBind[7].buffer_type = MYSQL_TYPE_STRING;
    resultBind[7].buffer = createdAtBuf;
    resultBind[7].buffer_length = sizeof(createdAtBuf);
    resultBind[7].length = &createdLen;

    if (mysql_stmt_bind_result(stmt, resultBind) != 0)
    {
        lastError = mysql_stmt_error(stmt);
        mysql_stmt_close(stmt);
        return false;
    }

    mysql_stmt_store_result(stmt);

    bool found = false;
    if (mysql_stmt_fetch(stmt) == 0)
    {
        found = true;
        outUser.userId = id;
        outUser.name = std::string(nameBuf, nameLen);
        outUser.email = std::string(emailBuf, emailOutLen);
        outUser.passwordHash = std::string(passBuf, passLen);
        outUser.role = std::string(roleBuf, roleLen);
        outUser.status = std::string(statusBuf, statusLen);
        outUser.mustChangePassword = (mustChange != 0);
        outUser.createdAt = std::string(createdAtBuf, createdLen);
    }

    mysql_stmt_close(stmt);
    return found;
}

bool Database::insertUser(
    const std::string& name,
    const std::string& email,
    const std::string& passwordHash,
    const std::string& role,
    const std::string& status,
    bool mustChangePassword,
    int& outUserId)
{
    std::lock_guard<std::mutex> lock(dbMutex);
    if (!conn) return false;

    const char* sql = "INSERT INTO users (name, email, password_hash, role, status, must_change_password) "
                      "VALUES (?, ?, ?, ?, ?, ?)";

    MYSQL_STMT* stmt = mysql_stmt_init(conn);
    if (!stmt) return false;

    if (mysql_stmt_prepare(stmt, sql, (unsigned long)strlen(sql)) != 0)
    {
        lastError = mysql_stmt_error(stmt);
        mysql_stmt_close(stmt);
        return false;
    }

    MYSQL_BIND paramBind[6];
    std::memset(paramBind, 0, sizeof(paramBind));

    unsigned long nameLen = (unsigned long)name.length();
    unsigned long emailLen = (unsigned long)email.length();
    unsigned long passLen = (unsigned long)passwordHash.length();
    unsigned long roleLen = (unsigned long)role.length();
    unsigned long statusLen = (unsigned long)status.length();
    my_bool mustChangeVal = mustChangePassword ? 1 : 0;

    paramBind[0].buffer_type = MYSQL_TYPE_STRING;
    paramBind[0].buffer = (void*)name.c_str();
    paramBind[0].buffer_length = nameLen;
    paramBind[0].length = &nameLen;

    paramBind[1].buffer_type = MYSQL_TYPE_STRING;
    paramBind[1].buffer = (void*)email.c_str();
    paramBind[1].buffer_length = emailLen;
    paramBind[1].length = &emailLen;

    paramBind[2].buffer_type = MYSQL_TYPE_STRING;
    paramBind[2].buffer = (void*)passwordHash.c_str();
    paramBind[2].buffer_length = passLen;
    paramBind[2].length = &passLen;

    paramBind[3].buffer_type = MYSQL_TYPE_STRING;
    paramBind[3].buffer = (void*)role.c_str();
    paramBind[3].buffer_length = roleLen;
    paramBind[3].length = &roleLen;

    paramBind[4].buffer_type = MYSQL_TYPE_STRING;
    paramBind[4].buffer = (void*)status.c_str();
    paramBind[4].buffer_length = statusLen;
    paramBind[4].length = &statusLen;

    paramBind[5].buffer_type = MYSQL_TYPE_TINY;
    paramBind[5].buffer = &mustChangeVal;

    if (mysql_stmt_bind_param(stmt, paramBind) != 0)
    {
        lastError = mysql_stmt_error(stmt);
        mysql_stmt_close(stmt);
        return false;
    }

    if (mysql_stmt_execute(stmt) != 0)
    {
        lastError = mysql_stmt_error(stmt);
        mysql_stmt_close(stmt);
        return false;
    }

    outUserId = (int)mysql_stmt_insert_id(stmt);
    mysql_stmt_close(stmt);
    return true;
}

bool Database::updatePassword(int userId, const std::string& newPasswordHash, bool mustChangePassword)
{
    std::lock_guard<std::mutex> lock(dbMutex);
    if (!conn) return false;

    const char* sql = "UPDATE users SET password_hash = ?, must_change_password = ? WHERE user_id = ?";

    MYSQL_STMT* stmt = mysql_stmt_init(conn);
    if (!stmt) return false;

    if (mysql_stmt_prepare(stmt, sql, (unsigned long)strlen(sql)) != 0)
    {
        lastError = mysql_stmt_error(stmt);
        mysql_stmt_close(stmt);
        return false;
    }

    MYSQL_BIND paramBind[3];
    std::memset(paramBind, 0, sizeof(paramBind));

    unsigned long passLen = (unsigned long)newPasswordHash.length();
    my_bool mustChangeVal = mustChangePassword ? 1 : 0;

    paramBind[0].buffer_type = MYSQL_TYPE_STRING;
    paramBind[0].buffer = (void*)newPasswordHash.c_str();
    paramBind[0].buffer_length = passLen;
    paramBind[0].length = &passLen;

    paramBind[1].buffer_type = MYSQL_TYPE_TINY;
    paramBind[1].buffer = &mustChangeVal;

    paramBind[2].buffer_type = MYSQL_TYPE_LONG;
    paramBind[2].buffer = &userId;

    if (mysql_stmt_bind_param(stmt, paramBind) != 0)
    {
        lastError = mysql_stmt_error(stmt);
        mysql_stmt_close(stmt);
        return false;
    }

    if (mysql_stmt_execute(stmt) != 0)
    {
        lastError = mysql_stmt_error(stmt);
        mysql_stmt_close(stmt);
        return false;
    }

    mysql_stmt_close(stmt);
    return true;
}

bool Database::updateUserStatus(int userId, const std::string& status)
{
    std::lock_guard<std::mutex> lock(dbMutex);
    if (!conn) return false;

    const char* sql = "UPDATE users SET status = ? WHERE user_id = ?";

    MYSQL_STMT* stmt = mysql_stmt_init(conn);
    if (!stmt) return false;

    if (mysql_stmt_prepare(stmt, sql, (unsigned long)strlen(sql)) != 0)
    {
        lastError = mysql_stmt_error(stmt);
        mysql_stmt_close(stmt);
        return false;
    }

    MYSQL_BIND paramBind[2];
    std::memset(paramBind, 0, sizeof(paramBind));

    unsigned long statusLen = (unsigned long)status.length();

    paramBind[0].buffer_type = MYSQL_TYPE_STRING;
    paramBind[0].buffer = (void*)status.c_str();
    paramBind[0].buffer_length = statusLen;
    paramBind[0].length = &statusLen;

    paramBind[1].buffer_type = MYSQL_TYPE_LONG;
    paramBind[1].buffer = &userId;

    if (mysql_stmt_bind_param(stmt, paramBind) != 0)
    {
        lastError = mysql_stmt_error(stmt);
        mysql_stmt_close(stmt);
        return false;
    }

    if (mysql_stmt_execute(stmt) != 0)
    {
        lastError = mysql_stmt_error(stmt);
        mysql_stmt_close(stmt);
        return false;
    }

    mysql_stmt_close(stmt);
    return true;
}

std::vector<UserRecord> Database::getUsersByRole(const std::string& role)
{
    std::lock_guard<std::mutex> lock(dbMutex);
    std::vector<UserRecord> results;
    if (!conn) return results;

    const char* sql = "SELECT user_id, name, email, password_hash, role, status, must_change_password, created_at "
                      "FROM users WHERE role = ? ORDER BY user_id ASC";

    MYSQL_STMT* stmt = mysql_stmt_init(conn);
    if (!stmt) return results;

    if (mysql_stmt_prepare(stmt, sql, (unsigned long)strlen(sql)) != 0)
    {
        lastError = mysql_stmt_error(stmt);
        mysql_stmt_close(stmt);
        return results;
    }

    MYSQL_BIND paramBind[1];
    std::memset(paramBind, 0, sizeof(paramBind));
    unsigned long roleInLen = (unsigned long)role.length();

    paramBind[0].buffer_type = MYSQL_TYPE_STRING;
    paramBind[0].buffer = (void*)role.c_str();
    paramBind[0].buffer_length = roleInLen;
    paramBind[0].length = &roleInLen;

    if (mysql_stmt_bind_param(stmt, paramBind) != 0 || mysql_stmt_execute(stmt) != 0)
    {
        lastError = mysql_stmt_error(stmt);
        mysql_stmt_close(stmt);
        return results;
    }

    int id = 0;
    char nameBuf[128] = {0};
    char emailBuf[160] = {0};
    char passBuf[260] = {0};
    char roleBuf[32] = {0};
    char statusBuf[32] = {0};
    my_bool mustChange = 0;
    char createdAtBuf[64] = {0};

    unsigned long nameLen = 0, emailOutLen = 0, passLen = 0, roleLen = 0, statusLen = 0, createdLen = 0;

    MYSQL_BIND resultBind[8];
    std::memset(resultBind, 0, sizeof(resultBind));

    resultBind[0].buffer_type = MYSQL_TYPE_LONG;
    resultBind[0].buffer = &id;

    resultBind[1].buffer_type = MYSQL_TYPE_STRING;
    resultBind[1].buffer = nameBuf;
    resultBind[1].buffer_length = sizeof(nameBuf);
    resultBind[1].length = &nameLen;

    resultBind[2].buffer_type = MYSQL_TYPE_STRING;
    resultBind[2].buffer = emailBuf;
    resultBind[2].buffer_length = sizeof(emailBuf);
    resultBind[2].length = &emailOutLen;

    resultBind[3].buffer_type = MYSQL_TYPE_STRING;
    resultBind[3].buffer = passBuf;
    resultBind[3].buffer_length = sizeof(passBuf);
    resultBind[3].length = &passLen;

    resultBind[4].buffer_type = MYSQL_TYPE_STRING;
    resultBind[4].buffer = roleBuf;
    resultBind[4].buffer_length = sizeof(roleBuf);
    resultBind[4].length = &roleLen;

    resultBind[5].buffer_type = MYSQL_TYPE_STRING;
    resultBind[5].buffer = statusBuf;
    resultBind[5].buffer_length = sizeof(statusBuf);
    resultBind[5].length = &statusLen;

    resultBind[6].buffer_type = MYSQL_TYPE_TINY;
    resultBind[6].buffer = &mustChange;

    resultBind[7].buffer_type = MYSQL_TYPE_STRING;
    resultBind[7].buffer = createdAtBuf;
    resultBind[7].buffer_length = sizeof(createdAtBuf);
    resultBind[7].length = &createdLen;

    if (mysql_stmt_bind_result(stmt, resultBind) == 0 && mysql_stmt_store_result(stmt) == 0)
    {
        while (mysql_stmt_fetch(stmt) == 0)
        {
            UserRecord u;
            u.userId = id;
            u.name = std::string(nameBuf, nameLen);
            u.email = std::string(emailBuf, emailOutLen);
            u.passwordHash = std::string(passBuf, passLen);
            u.role = std::string(roleBuf, roleLen);
            u.status = std::string(statusBuf, statusLen);
            u.mustChangePassword = (mustChange != 0);
            u.createdAt = std::string(createdAtBuf, createdLen);
            results.push_back(u);
        }
    }

    mysql_stmt_close(stmt);
    return results;
}

std::vector<UserRecord> Database::getAllUsers()
{
    std::lock_guard<std::mutex> lock(dbMutex);
    std::vector<UserRecord> results;
    if (!conn) return results;

    const char* sql = "SELECT user_id, name, email, password_hash, role, status, must_change_password, created_at "
                      "FROM users ORDER BY user_id ASC";

    if (mysql_query(conn, sql) != 0)
    {
        lastError = mysql_error(conn);
        return results;
    }

    MYSQL_RES* res = mysql_store_result(conn);
    if (!res) return results;

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res)))
    {
        UserRecord u;
        u.userId = row[0] ? std::atoi(row[0]) : 0;
        u.name = row[1] ? row[1] : "";
        u.email = row[2] ? row[2] : "";
        u.passwordHash = row[3] ? row[3] : "";
        u.role = row[4] ? row[4] : "";
        u.status = row[5] ? row[5] : "";
        u.mustChangePassword = (row[6] && std::string(row[6]) == "1");
        u.createdAt = row[7] ? row[7] : "";
        results.push_back(u);
    }
    mysql_free_result(res);
    return results;
}

int Database::countActiveAdmins()
{
    std::lock_guard<std::mutex> lock(dbMutex);
    if (!conn) return 0;

    const char* sql = "SELECT COUNT(*) FROM users WHERE role = 'ADMIN' AND status = 'ACTIVE'";
    if (mysql_query(conn, sql) != 0)
    {
        lastError = mysql_error(conn);
        return 0;
    }

    MYSQL_RES* res = mysql_store_result(conn);
    if (!res) return 0;

    int count = 0;
    MYSQL_ROW row = mysql_fetch_row(res);
    if (row && row[0])
    {
        count = std::atoi(row[0]);
    }
    mysql_free_result(res);
    return count;
}

bool Database::isEmailTaken(const std::string& email)
{
    UserRecord u;
    return findUserByEmail(email, u);
}
