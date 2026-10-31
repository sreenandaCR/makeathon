#include "customer_auth.h"
#include <iostream>

bool CustomerAuth::registerCustomer(
    Database& db,
    const std::string& name,
    const std::string& email,
    const std::string& password,
    const std::string& confirmPassword,
    std::string& outError)
{
    if (name.empty())
    {
        outError = "Full name is required.";
        return false;
    }

    if (email.empty() || email.find('@') == std::string::npos || email.find('.') == std::string::npos)
    {
        outError = "A valid email address is required.";
        return false;
    }

    if (password != confirmPassword)
    {
        outError = "Passwords do not match.";
        return false;
    }

    // Backend password complexity validation
    if (!Auth::validatePasswordPolicy(password, outError))
    {
        return false;
    }

    // Enforce email uniqueness across all user roles
    if (db.isEmailTaken(email))
    {
        outError = "An account with this email address already exists.";
        return false;
    }

    std::string hash = Auth::hashPassword(password);
    int newId = 0;
    // Customers are active by default, and must_change_password = false
    if (!db.insertUser(name, email, hash, "CUSTOMER", "ACTIVE", false, newId))
    {
        outError = "Registration failed: " + db.getLastError();
        return false;
    }

    return true;
}

AuthResult CustomerAuth::loginCustomer(
    Database& db,
    const std::string& email,
    const std::string& password)
{
    Auth auth;
    AuthResult res = auth.authenticate(db, email, password);
    if (res.success && res.user.role != "CUSTOMER")
    {
        res.success = false;
        res.errorMessage = "This login portal is for customers only.";
        SessionManager::getInstance().invalidateSession(res.token);
        res.token.clear();
    }
    return res;
}
