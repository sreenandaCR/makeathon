#ifndef CUSTOMER_AUTH_H
#define CUSTOMER_AUTH_H

#include <string>
#include "database.h"
#include "auth.h"

class CustomerAuth
{
public:
    CustomerAuth() = default;

    bool registerCustomer(
        Database& db,
        const std::string& name,
        const std::string& email,
        const std::string& password,
        const std::string& confirmPassword,
        std::string& outError
    );

    AuthResult loginCustomer(
        Database& db,
        const std::string& email,
        const std::string& password
    );
};

#endif // CUSTOMER_AUTH_H
