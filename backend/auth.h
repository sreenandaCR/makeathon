
#ifndef AUTH_H
#define AUTH_H

#include <string>
using namespace std;

class Auth
{
public:
    bool login(string email, string password);

    bool addStaff(
        string name,
        string email,
        string password
    );
};

#endif
