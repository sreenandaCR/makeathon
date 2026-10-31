
#include "auth.h"
#include <bcrypt/BCrypt.hpp>
#include <iostream>
#include <random>
#include <sstream>
#include <iomanip>
#include <cctype>

SessionManager& SessionManager::getInstance()
{
    static SessionManager instance;
    return instance;
}

std::string SessionManager::createSession(const UserRecord& user)
{
    std::lock_guard<std::mutex> lock(sessionMutex);
    std::string token = Auth::generateRandomToken(32);
    // Erase sensitive password hash from in-memory session object
    UserRecord safeUser = user;
    safeUser.passwordHash.clear();
    sessions[token] = safeUser;
    return token;
}

bool SessionManager::getSession(const std::string& token, UserRecord& outUser)
{
    std::lock_guard<std::mutex> lock(sessionMutex);
    auto it = sessions.find(token);
    if (it != sessions.end())
    {
        outUser = it->second;
        return true;
    }
    return false;
}

void SessionManager::updateSessionUser(const std::string& token, const UserRecord& user)
{
    std::lock_guard<std::mutex> lock(sessionMutex);
    auto it = sessions.find(token);
    if (it != sessions.end())
    {
        UserRecord safeUser = user;
        safeUser.passwordHash.clear();
        it->second = safeUser;
    }
}

void SessionManager::invalidateSession(const std::string& token)
{
    std::lock_guard<std::mutex> lock(sessionMutex);
    sessions.erase(token);
}

// Preserve existing method signature
bool Auth::login(std::string email, std::string password)
{
    Database db;
    if (!db.connect()) return false;
    AuthResult res = authenticate(db, email, password);
    return res.success;
}

// Preserve existing method signature
bool Auth::addStaff(std::string name, std::string email, std::string password)
{
    Database db;
    if (!db.connect()) return false;
    std::string error;
    if (!validatePasswordPolicy(password, error)) return false;
    if (db.isEmailTaken(email)) return false;

    std::string hash = hashPassword(password);
    int newId = 0;
    return db.insertUser(name, email, hash, "STAFF", "ACTIVE", true, newId);
}

AuthResult Auth::authenticate(Database& db, const std::string& email, const std::string& password)
{
    AuthResult result;
    result.success = false;

    if (email.empty() || password.empty())
    {
        result.errorMessage = "Email and password are required.";
        return result;
    }

    UserRecord user;
    if (!db.findUserByEmail(email, user))
    {
        result.errorMessage = "Invalid email or password.";
        return result;
    }

    if (!verifyPassword(password, user.passwordHash))
    {
        result.errorMessage = "Invalid email or password.";
        return result;
    }

    if (user.status != "ACTIVE")
    {
        result.errorMessage = "Your account is deactivated. Please contact an administrator.";
        return result;
    }

    // Success: create authenticated session
    result.success = true;
    result.token = SessionManager::getInstance().createSession(user);
    result.user = user;
    result.user.passwordHash.clear(); // do not expose hash
    return result;
}

bool Auth::validatePasswordPolicy(const std::string& password, std::string& outError)
{
    // Requirement: Password must contain more than 8 characters, uppercase, lowercase and a special character.
    if (password.length() <= 8)
    {
        outError = "Password must be more than 8 characters long.";
        return false;
    }

    bool hasUpper = false;
    bool hasLower = false;
    bool hasSpecial = false;

    const std::string specialChars = "!@#$%^&*()_+-=[]{}|;':\",./<>?`~\\";

    for (char c : password)
    {
        if (std::isupper(static_cast<unsigned char>(c))) hasUpper = true;
        else if (std::islower(static_cast<unsigned char>(c))) hasLower = true;
        else if (specialChars.find(c) != std::string::npos) hasSpecial = true;
    }

    if (!hasUpper)
    {
        outError = "Password must contain at least one uppercase letter.";
        return false;
    }
    if (!hasLower)
    {
        outError = "Password must contain at least one lowercase letter.";
        return false;
    }
    if (!hasSpecial)
    {
        outError = "Password must contain at least one special character (e.g. !@#$%^&*).";
        return false;
    }

    return true;
}

std::string Auth::hashPassword(const std::string& password)
{
    return BCrypt::generateHash(password);
}

bool Auth::verifyPassword(const std::string& password, const std::string& hash)
{
    if (hash.empty() || password.empty()) return false;
    return BCrypt::validatePassword(password, hash);
}

bool Auth::changePassword(
    Database& db,
    int userId,
    const std::string& oldPassword,
    const std::string& newPassword,
    std::string& outError)
{
    UserRecord user;
    if (!db.findUserById(userId, user))
    {
        outError = "User not found.";
        return false;
    }

    if (!verifyPassword(oldPassword, user.passwordHash))
    {
        outError = "Current password is incorrect.";
        return false;
    }

    if (!validatePasswordPolicy(newPassword, outError))
    {
        return false;
    }

    std::string newHash = hashPassword(newPassword);
    if (!db.updatePassword(userId, newHash, false))
    {
        outError = "Failed to update password in database.";
        return false;
    }

    return true;
}

bool Auth::changeTemporaryPassword(
    Database& db,
    int userId,
    const std::string& newPassword,
    std::string& outError)
{
    UserRecord user;
    if (!db.findUserById(userId, user))
    {
        outError = "User not found.";
        return false;
    }

    if (!validatePasswordPolicy(newPassword, outError))
    {
        return false;
    }

    std::string newHash = hashPassword(newPassword);
    if (!db.updatePassword(userId, newHash, false))
    {
        outError = "Failed to update password in database.";
        return false;
    }

    return true;
}

std::string Auth::generateRandomToken(size_t length)
{
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<unsigned int> dis(0, 255);

    std::stringstream ss;
    for (size_t i = 0; i < length; ++i)
    {
        ss << std::hex << std::setw(2) << std::setfill('0') << dis(gen);
    }
    return ss.str();
}

std::string Auth::generateTemporaryPassword()
{
    // Generate a strong temporary password meeting the policy (> 8 chars, uppercase, lowercase, special char)
    const std::string uppers = "ABCDEFGHJKLMNPQRSTUVWXYZ";
    const std::string lowers = "abcdefghijkmnopqrstuvwxyz";
    const std::string digits = "23456789";
    const std::string specials = "!@#$%^&*";
    const std::string all = uppers + lowers + digits + specials;

    std::random_device rd;
    std::mt19937 gen(rd());

    std::string pwd;
    pwd += uppers[gen() % uppers.size()];
    pwd += lowers[gen() % lowers.size()];
    pwd += digits[gen() % digits.size()];
    pwd += specials[gen() % specials.size()];

    for (int i = 0; i < 8; ++i)
    {
        pwd += all[gen() % all.size()];
    }

    std::shuffle(pwd.begin(), pwd.end(), gen);
    return pwd;
}

