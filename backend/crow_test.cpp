
#include "crow.h"
#include <iostream>

using namespace std;

int main()
{
    crow::SimpleApp app;

    CROW_ROUTE(app, "/")
    ([]()
    {
        return "Subscription Management System Server is Running!";
    });

    cout << "Server starting..." << endl;

    app.port(18080).multithreaded().run();

    return 0;
}
