
#include "customer_auth.h"
#include <iostream>

using namespace std;

bool CustomerAuth::registerCustomer(
    string name,
    string email,
    string password,
    string phone)
{
    if (name.empty() || email.empty() ||
        password.empty() || phone.empty())
    {
        cout << "All fields are required!" << endl;
        return false;
    }

    if (password.length() < 8)
    {
        cout << "Password must contain at least 8 characters!" << endl;
        return false;
    }

    cout << "Customer registration details validated!" << endl;

    // Database insertion will be implemented later.

    return true;
}

bool CustomerAuth::loginCustomer(
    string email,
    string password)
{
    if (email.empty() || password.empty())
    {
        cout << "Email and password are required!" << endl;
        return false;
    }

    cout << "Customer login request received!" << endl;

    // Database authentication will be implemented later.

    return false;
}
