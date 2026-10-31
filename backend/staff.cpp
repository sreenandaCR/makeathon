
#include <iostream>
#include <string>
#include <cpp-httplib/httplib.h>
#include <nlohmann/json.hpp>
#include <mysql/jdbc.h>

using namespace std;
using json = nlohmann::json;

// Shared database connection
sql::Connection* db = nullptr;

void connectDatabase()
{
    sql::Driver* driver =
        get_driver_instance();

    db = driver->connect(
        "tcp://127.0.0.1:3306",
        "root",
        "YOUR_MYSQL_PASSWORD"
    );

    db->setSchema("makeathon");
}

// Authentication must be implemented by your
// common login system, not trusted from the browser.
bool authorizeStaff(
    const httplib::Request& req,
    string& staffID
)
{
    // Replace with verification of a signed session token.
    // Never trust a role supplied by the client.
    return false;
}

int main()
{
    connectDatabase();

    httplib::Server server;

    // Verify staff session
    server.Get("/api/staff/me",
        [](const httplib::Request& req,
           httplib::Response& res)
    {
        string staffID;

        if (!authorizeStaff(req, staffID))
        {
            res.status = 401;
            res.set_content(
                R"({"error":"Unauthorized"})",
                "application/json"
            );
            return;
        }

        res.set_content(
            R"({"role":"STAFF"})",
            "application/json"
        );
    });

    // Dashboard counts
    server.Get("/api/staff/dashboard",
        [](const httplib::Request& req,
           httplib::Response& res)
    {
        string staffID;

        if (!authorizeStaff(req, staffID))
        {
            res.status = 401;
            return;
        }

        json result;

        sql::Statement* stmt = db->createStatement();

        sql::ResultSet* r = stmt->executeQuery(
            "SELECT "
            "(SELECT COUNT(*) FROM users WHERE role='CUSTOMER'),"
            "(SELECT COUNT(*) FROM subscriptions),"
            "(SELECT COUNT(*) FROM invoices),"
            "(SELECT COUNT(*) FROM payments)"
        );

        if (r->next())
        {
            result["customers"] = r->getInt(1);
            result["subscriptions"] = r->getInt(2);
            result["invoices"] = r->getInt(3);
            result["payments"] = r->getInt(4);
        }

        res.set_content(result.dump(), "application/json");

        delete r;
        delete stmt;
    });

    // View customers
    server.Get("/api/staff/customers",
        [](const httplib::Request& req,
           httplib::Response& res)
    {
        string staffID;

        if (!authorizeStaff(req, staffID))
        {
            res.status = 401;
            return;
        }

        json customers = json::array();

        sql::Statement* stmt = db->createStatement();

        sql::ResultSet* r = stmt->executeQuery(
            "SELECT user_id,name,email,phone "
            "FROM users WHERE role='CUSTOMER'"
        );

        while (r->next())
        {
            customers.push_back({
                {"id", r->getInt("user_id")},
                {"name", r->getString("name")},
                {"email", r->getString("email")},
                {"phone", r->getString("phone")}
            });
        }

        res.set_content(customers.dump(), "application/json");

        delete r;
        delete stmt;
    });

    cout << "Staff backend running on port 8080\n";

    server.listen("localhost", 8080);

    delete db;

    return 0;
}
