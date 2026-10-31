#include "customer.h"
#include "auth.h"

bool Customer::getProfile(Database& db, int customerId, UserRecord& outProfile, std::string& outError)
{
    if (!db.findUserById(customerId, outProfile))
    {
        outError = "Customer account not found.";
        return false;
    }

    if (outProfile.role != "CUSTOMER")
    {
        outError = "User is not a customer.";
        return false;
    }

    outProfile.passwordHash.clear();
    return true;
}

bool Customer::changePassword(
    Database& db,
    int customerId,
    const std::string& oldPassword,
    const std::string& newPassword,
    std::string& outError)
{
    Auth auth;
    return auth.changePassword(db, customerId, oldPassword, newPassword, outError);
}
