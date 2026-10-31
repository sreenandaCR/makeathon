#ifndef CUSTOMER_H
#define CUSTOMER_H

#include <string>
#include "database.h"

class Customer
{
public:
    Customer() = default;

    bool getProfile(Database& db, int customerId, UserRecord& outProfile, std::string& outError);
    bool changePassword(
        Database& db,
        int customerId,
        const std::string& oldPassword,
        const std::string& newPassword,
        std::string& outError
    );
};

#endif // CUSTOMER_H
