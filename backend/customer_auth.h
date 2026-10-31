
#ifndef CUSTOMER_AUTH_H
#define CUSTOMER_AUTH_H

#include <string>
using namespace std;

class CustomerAuth
{
public:

    // Customer registration
    bool registerCustomer(
        string name,
        string email,
        string password,
        string phone
    );

    // Customer login
    bool loginCustomer(
        string email,
        string password
    );

};

#endif
