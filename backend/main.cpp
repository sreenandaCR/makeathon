
#include <iostream>
#include "database.h"
#include "auth.h"
#include "admin.h"

using namespace std;

int main()
{
    cout << "================================" << endl;
    cout << "  SUBSCRIPTION MANAGEMENT SYSTEM" << endl;
    cout << "================================" << endl;

    cout << "Starting Backend..." << endl;

    // Database connection
    Database db;

    if (!db.connect())
    {
        cout << "Database connection failed!" << endl;
        return 1;
    }

    cout << "Database connected successfully!" << endl;

    // Create predefined Admin account
    Admin admin;

    admin.createAdmin();

    cout << "Backend initialization completed!" << endl;

    return 0;
}
