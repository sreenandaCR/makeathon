#ifndef ADMIN_H
#define ADMIN_H

#include <string>
#include <vector>
#include "database.h"

class Admin
{
public:
    Admin() = default;

    // Preserved method called in main.cpp
    void createAdmin();

    // Administrative operations
    bool addAdmin(
        Database& db,
        const std::string& name,
        const std::string& email,
        std::string& outTempPassword,
        std::string& outError
    );

    bool addStaff(
        Database& db,
        const std::string& name,
        const std::string& email,
        std::string& outTempPassword,
        std::string& outError
    );

    std::vector<UserRecord> listAdmins(Database& db);
    std::vector<UserRecord> listStaff(Database& db);
    std::vector<UserRecord> listCustomers(Database& db);

    bool toggleUserStatus(
        Database& db,
        int targetUserId,
        const std::string& newStatus,
        std::string& outError
    );
};

#endif // ADMIN_H
