#include "admin.h"
#include "auth.h"
#include <iostream>
#include <cstdlib>

void Admin::createAdmin()
{
    Database db;
    if (!db.connect())
    {
        std::cout << "[Admin] Could not connect to database for initial admin verification." << std::endl;
        return;
    }

    auto existingAdmins = db.getUsersByRole("ADMIN");
    if (!existingAdmins.empty())
    {
        std::cout << "[Admin] Predefined Admin account already exists (" << existingAdmins.size() << " admin(s) found)." << std::endl;
        return;
    }

    char* envName = getenv("ADMIN_NAME");
    std::string adminName = envName ? envName : "System Administrator";

    char* envEmail = getenv("ADMIN_EMAIL");
    std::string adminEmail = envEmail ? envEmail : "admin@system.local";

    char* envPassword = getenv("ADMIN_PASSWORD");
    std::string adminPassword = envPassword ? envPassword : "Admin@123456!";

    std::string hash = Auth::hashPassword(adminPassword);
    int newId = 0;
    if (db.insertUser(adminName, adminEmail, hash, "ADMIN", "ACTIVE", false, newId))
    {
        std::cout << "[Admin] Predefined Admin created successfully!" << std::endl;
        std::cout << "[Admin] Initial Admin Email: " << adminEmail << std::endl;
        if (!envPassword)
        {
            std::cout << "[Admin] Initial Admin Default Password: " << adminPassword << std::endl;
            std::cout << "[Admin] (Note: You can override this using the ADMIN_PASSWORD environment variable)" << std::endl;
        }
    }
    else
    {
        std::cout << "[Admin] Failed to create initial admin: " << db.getLastError() << std::endl;
    }
}

bool Admin::addAdmin(
    Database& db,
    const std::string& name,
    const std::string& email,
    std::string& outTempPassword,
    std::string& outError)
{
    if (name.empty())
    {
        outError = "Name cannot be empty.";
        return false;
    }

    if (email.empty() || email.find('@') == std::string::npos)
    {
        outError = "Valid email address is required.";
        return false;
    }

    if (db.isEmailTaken(email))
    {
        outError = "A user with this email address already exists.";
        return false;
    }

    outTempPassword = Auth::generateTemporaryPassword();
    std::string hash = Auth::hashPassword(outTempPassword);

    int newId = 0;
    // New admin accounts receive a temporary password and must_change_password = true
    if (!db.insertUser(name, email, hash, "ADMIN", "ACTIVE", true, newId))
    {
        outError = "Database error: " + db.getLastError();
        return false;
    }

    return true;
}

bool Admin::addStaff(
    Database& db,
    const std::string& name,
    const std::string& email,
    std::string& outTempPassword,
    std::string& outError)
{
    if (name.empty())
    {
        outError = "Name cannot be empty.";
        return false;
    }

    if (email.empty() || email.find('@') == std::string::npos)
    {
        outError = "Valid email address is required.";
        return false;
    }

    if (db.isEmailTaken(email))
    {
        outError = "A user with this email address already exists.";
        return false;
    }

    outTempPassword = Auth::generateTemporaryPassword();
    std::string hash = Auth::hashPassword(outTempPassword);

    int newId = 0;
    // New staff accounts receive a temporary password and must_change_password = true
    if (!db.insertUser(name, email, hash, "STAFF", "ACTIVE", true, newId))
    {
        outError = "Database error: " + db.getLastError();
        return false;
    }

    return true;
}

std::vector<UserRecord> Admin::listAdmins(Database& db)
{
    auto users = db.getUsersByRole("ADMIN");
    for (auto& u : users) u.passwordHash.clear();
    return users;
}

std::vector<UserRecord> Admin::listStaff(Database& db)
{
    auto users = db.getUsersByRole("STAFF");
    for (auto& u : users) u.passwordHash.clear();
    return users;
}

std::vector<UserRecord> Admin::listCustomers(Database& db)
{
    auto users = db.getUsersByRole("CUSTOMER");
    for (auto& u : users) u.passwordHash.clear();
    return users;
}

bool Admin::toggleUserStatus(
    Database& db,
    int targetUserId,
    const std::string& newStatus,
    std::string& outError)
{
    if (newStatus != "ACTIVE" && newStatus != "INACTIVE")
    {
        outError = "Invalid status. Must be ACTIVE or INACTIVE.";
        return false;
    }

    UserRecord target;
    if (!db.findUserById(targetUserId, target))
    {
        outError = "Target user not found.";
        return false;
    }

    // Critical Requirement: Prevent deactivation or deletion of the last active admin.
    if (target.role == "ADMIN" && newStatus == "INACTIVE")
    {
        int activeAdmins = db.countActiveAdmins();
        if (activeAdmins <= 1 && target.status == "ACTIVE")
        {
            outError = "Cannot deactivate the last active admin.";
            return false;
        }
    }

    if (!db.updateUserStatus(targetUserId, newStatus))
    {
        outError = "Database error: " + db.getLastError();
        return false;
    }

    return true;
}
