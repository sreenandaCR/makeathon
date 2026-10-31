
#ifndef AUTH_H
#define AUTH_H

#include <string>
#include <unordered_map>
#include <mutex>
#include "database.h"

struct AuthResult
{
    bool success = false;
    std::string token;
    UserRecord user;
    std::string errorMessage;
};

class SessionManager
{
public:
    static SessionManager& getInstance();

    std::string createSession(const UserRecord& user);
    bool getSession(const std::string& token, UserRecord& outUser);
    void updateSessionUser(const std::string& token, const UserRecord& user);
    void invalidateSession(const std::string& token);

private:
    SessionManager() = default;
    std::mutex sessionMutex;
    std::unordered_map<std::string, UserRecord> sessions;
};

class Auth
{
public:
    // Existing signatures preserved
    bool login(std::string email, std::string password);
    bool addStaff(
        std::string name,
        std::string email,
        std::string password
    );

    // Enhanced multi-role authentication methods
    AuthResult authenticate(Database& db, const std::string& email, const std::string& password);
    bool changePassword(
        Database& db,
        int userId,
        const std::string& oldPassword,
        const std::string& newPassword,
        std::string& outError
    );
    bool changeTemporaryPassword(
        Database& db,
        int userId,
        const std::string& newPassword,
        std::string& outError
    );

    // Cryptographic & validation helpers
    static bool validatePasswordPolicy(const std::string& password, std::string& outError);
    static std::string hashPassword(const std::string& password);
    static bool verifyPassword(const std::string& password, const std::string& hash);
    static std::string generateRandomToken(size_t length = 32);
    static std::string generateTemporaryPassword();
};

#endif // AUTH_H

