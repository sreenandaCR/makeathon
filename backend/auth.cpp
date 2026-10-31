
#include "auth.h"
#include <iostream>

using namespace std;

bool Auth::login(string email, string password)
{
    cout << "Login request received" << endl;

    // Database authentication will be implemented here.

    return false;
}

bool Auth::addStaff(
    string name,
    string email,
    string password)
{
    cout << "Staff creation request received" << endl;

    // Staff account creation will be implemented here.

    return false;
}
