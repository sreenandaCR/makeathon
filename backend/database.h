#ifndef DATABASE_H
#define DATABASE_H

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <winsock2.h>
#include <windows.h>
#include <mysql.h>

#ifndef my_bool
typedef char my_bool;
#endif

#include <string>
#include <vector>
#include <mutex>

struct UserRecord
{
    int userId = 0;
    std::string name;
    std::string email;
    std::string passwordHash;
    std::string role;   // "ADMIN", "STAFF", "CUSTOMER"
    std::string status; // "ACTIVE", "INACTIVE"
    bool mustChangePassword = false;
    std::string createdAt;
};

class Database
{
public:
    Database();
    ~Database();

    bool connect();
    void disconnect();
    bool isConnected();

    // Query & Account operations using prepared statements
    bool findUserByEmail(const std::string& email, UserRecord& outUser);
    bool findUserById(int userId, UserRecord& outUser);
    bool insertUser(
        const std::string& name,
        const std::string& email,
        const std::string& passwordHash,
        const std::string& role,
        const std::string& status,
        bool mustChangePassword,
        int& outUserId
    );
    bool updatePassword(int userId, const std::string& newPasswordHash, bool mustChangePassword);
    bool updateUserStatus(int userId, const std::string& status);
    std::vector<UserRecord> getUsersByRole(const std::string& role);
    std::vector<UserRecord> getAllUsers();
    int countActiveAdmins();
    bool isEmailTaken(const std::string& email);

    std::string getLastError() const;

private:
    MYSQL* conn;
    mutable std::mutex dbMutex;
    std::string lastError;

    std::string host;
    int port;
    std::string user;
    std::string password;
    std::string dbName;

    void loadConfig();
    bool initializeTables();
};

#endif // DATABASE_H
