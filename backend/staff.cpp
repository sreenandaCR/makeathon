
#include <iostream>
#include <string>
#include <map>
#include <mutex>
#include <random>
#include <memory>

#include <cpp-httplib/httplib.h>
#include <nlohmann/json.hpp>
#include <mysql/jdbc.h>

using namespace std;
using json = nlohmann::json;

// ---------------- GLOBAL VARIABLES ----------------

sql::Connection* db = nullptr;

mutex dbMutex;
mutex tokenMutex;

map<string, int> staffSessions;

// ---------------- DATABASE CONNECTION ----------------

void connectDatabase()
{
    sql::Driver* driver = get_driver_instance();

    db = driver->connect(
        "tcp://127.0.0.1:3306",
        "root",
        "YOUR_MYSQL_PASSWORD"
    );

    db->setSchema("subscription_management");

    cout << "Database connected successfully!" << endl;
}

// ---------------- JSON RESPONSE ----------------

void sendJSON(
    httplib::Response& res,
    const json& data,
    int status = 200
)
{
    res.status = status;

    res.set_content(
        data.dump(),
        "application/json"
    );
}

// ---------------- TOKEN GENERATION ----------------

string generateToken()
{
    static random_device rd;
    static mt19937 generator(rd());

    uniform_int_distribution<int> distribution(0, 15);

    string token;

    const string characters = "0123456789abcdef";

    for (int i = 0; i < 64; i++)
    {
        token += characters[distribution(generator)];
    }

    return token;
}

// ---------------- STAFF AUTHORIZATION ----------------

bool authorizeStaff(
    const httplib::Request& req,
    int& staffID
)
{
    if (!req.has_header("Authorization"))
        return false;

    string header = req.get_header_value("Authorization");

    string prefix = "Bearer ";

    if (header.rfind(prefix, 0) != 0)
        return false;

    string token = header.substr(prefix.length());

    lock_guard<mutex> lock(tokenMutex);

    auto it = staffSessions.find(token);

    if (it == staffSessions.end())
        return false;

    staffID = it->second;

    return true;
}

// ---------------- STAFF LOGIN ----------------

void staffLogin(httplib::Server& server)
{
    server.Post(
        "/api/staff/login",
        [](const httplib::Request& req, httplib::Response& res)
        {
            try
            {
                json body = json::parse(req.body);

                string email = body.value("email", "");
                string passwordHash = body.value("passwordHash", "");

                if (email.empty() || passwordHash.empty())
                {
                    sendJSON(res,
                    {
                        {"error", "Email and password are required"}
                    }, 400);

                    return;
                }

                int id = 0;

                {
                    lock_guard<mutex> lock(dbMutex);

                    unique_ptr<sql::PreparedStatement> stmt(
                        db->prepareStatement(
                            "SELECT user_id FROM users "
                            "WHERE email=? AND password_hash=? "
                            "AND role='STAFF' AND status='ACTIVE'"
                        )
                    );

                    stmt->setString(1, email);
                    stmt->setString(2, passwordHash);

                    unique_ptr<sql::ResultSet> result(
                        stmt->executeQuery()
                    );

                    if (!result->next())
                    {
                        sendJSON(res,
                        {
                            {"error", "Invalid staff credentials"}
                        }, 401);

                        return;
                    }

                    id = result->getInt("user_id");
                }

                string token = generateToken();

                {
                    lock_guard<mutex> lock(tokenMutex);
                    staffSessions[token] = id;
                }

                sendJSON(res,
                {
                    {"message", "Login successful"},
                    {"token", token},
                    {"role", "STAFF"}
                });
            }
            catch (exception& e)
            {
                sendJSON(res,
                {
                    {"error", e.what()}
                }, 500);
            }
        }
    );
}

// ---------------- VERIFY STAFF ----------------

void staffMe(httplib::Server& server)
{
    server.Get(
        "/api/staff/me",
        [](const httplib::Request& req, httplib::Response& res)
        {
            int staffID;

            if (!authorizeStaff(req, staffID))
            {
                sendJSON(res,
                {
                    {"error", "Unauthorized"}
                }, 401);

                return;
            }

            sendJSON(res,
            {
                {"staffID", staffID},
                {"role", "STAFF"}
            });
        }
    );
}

// ---------------- DASHBOARD ----------------

void staffDashboard(httplib::Server& server)
{
    server.Get(
        "/api/staff/dashboard",
        [](const httplib::Request& req, httplib::Response& res)
        {
            int staffID;

            if (!authorizeStaff(req, staffID))
            {
                sendJSON(res,
                {
                    {"error", "Unauthorized"}
                }, 401);

                return;
            }

            try
            {
                lock_guard<mutex> lock(dbMutex);

                unique_ptr<sql::Statement> stmt(
                    db->createStatement()
                );

                unique_ptr<sql::ResultSet> result(
                    stmt->executeQuery(
                        "SELECT "
                        "(SELECT COUNT(*) FROM users WHERE role='CUSTOMER'),"
                        "(SELECT COUNT(*) FROM subscriptions),"
                        "(SELECT COUNT(*) FROM invoices),"
                        "(SELECT COUNT(*) FROM payments)"
                    )
                );

                result->next();

                sendJSON(res,
                {
                    {"customers", result->getInt(1)},
                    {"subscriptions", result->getInt(2)},
                    {"invoices", result->getInt(3)},
                    {"payments", result->getInt(4)}
                });
            }
            catch (exception& e)
            {
                sendJSON(res,
                {
                    {"error", e.what()}
                }, 500);
            }
        }
    );
}

// ---------------- CUSTOMERS ----------------

void staffCustomers(httplib::Server& server)
{
    server.Get(
        "/api/staff/customers",
        [](const httplib::Request& req, httplib::Response& res)
        {
            int staffID;

            if (!authorizeStaff(req, staffID))
            {
                sendJSON(res,
                {
                    {"error", "Unauthorized"}
                }, 401);

                return;
            }

            try
            {
                lock_guard<mutex> lock(dbMutex);

                json customers = json::array();

                unique_ptr<sql::Statement> stmt(
                    db->createStatement()
                );

                unique_ptr<sql::ResultSet> result(
                    stmt->executeQuery(
                        "SELECT user_id, name, email, phone "
                        "FROM users WHERE role='CUSTOMER'"
                    )
                );

                while (result->next())
                {
                    customers.push_back(
                    {
                        {"id", result->getInt("user_id")},
                        {"name", result->getString("name")},
                        {"email", result->getString("email")},
                        {"phone", result->getString("phone")}
                    });
                }

                sendJSON(res, customers);
            }
            catch (exception& e)
            {
                sendJSON(res,
                {
                    {"error", e.what()}
                }, 500);
            }
        }
    );
}

// ---------------- SUBSCRIPTIONS ----------------

void staffSubscriptions(httplib::Server& server)
{
    server.Get(
        "/api/staff/subscriptions",
        [](const httplib::Request& req, httplib::Response& res)
        {
            int staffID;

            if (!authorizeStaff(req, staffID))
            {
                sendJSON(res,
                {
                    {"error", "Unauthorized"}
                }, 401);

                return;
            }

            try
            {
                lock_guard<mutex> lock(dbMutex);

                json subscriptions = json::array();

                unique_ptr<sql::Statement> stmt(
                    db->createStatement()
                );

                unique_ptr<sql::ResultSet> result(
                    stmt->executeQuery(
                        "SELECT s.subscription_id, s.customer_id, "
                        "u.name, s.plan_name, s.status, "
                        "s.start_date, s.end_date "
                        "FROM subscriptions s "
                        "JOIN users u ON s.customer_id = u.user_id"
                    )
                );

                while (result->next())
                {
                    subscriptions.push_back(
                    {
                        {"id", result->getInt("subscription_id")},
                        {"customer_id", result->getInt("customer_id")},
                        {"customer", result->getString("name")},
                        {"plan", result->getString("plan_name")},
                        {"status", result->getString("status")},
                        {"startDate", result->getString("start_date")},
                        {"endDate", result->getString("end_date")}
                    });
                }

                sendJSON(res, subscriptions);
            }
            catch (exception& e)
            {
                sendJSON(res,
                {
                    {"error", e.what()}
                }, 500);
            }
        }
    );
}

// ---------------- INVOICES ----------------

void staffInvoices(httplib::Server& server)
{
    server.Get(
        "/api/staff/invoices",
        [](const httplib::Request& req, httplib::Response& res)
        {
            int staffID;

            if (!authorizeStaff(req, staffID))
            {
                sendJSON(res,
                {
                    {"error", "Unauthorized"}
                }, 401);

                return;
            }

            try
            {
                lock_guard<mutex> lock(dbMutex);

                json invoices = json::array();

                unique_ptr<sql::Statement> stmt(
                    db->createStatement()
                );

                unique_ptr<sql::ResultSet> result(
                    stmt->executeQuery(
                        "SELECT i.invoice_id, i.customer_id, "
                        "u.name, i.amount, i.invoice_date, "
                        "i.due_date, i.status "
                        "FROM invoices i "
                        "JOIN users u ON i.customer_id = u.user_id"
                    )
                );

                while (result->next())
                {
                    invoices.push_back(
                    {
                        {"id", result->getInt("invoice_id")},
                        {"customer_id", result->getInt("customer_id")},
                        {"customer", result->getString("name")},
                        {"amount", result->getDouble("amount")},
                        {"date", result->getString("invoice_date")},
                        {"dueDate", result->getString("due_date")},
                        {"status", result->getString("status")}
                    });
                }

                sendJSON(res, invoices);
            }
            catch (exception& e)
            {
                sendJSON(res,
                {
                    {"error", e.what()}
                }, 500);
            }
        }
    );
}

// ---------------- RECORD PAYMENT ----------------

void staffPayments(httplib::Server& server)
{
    server.Post(
        "/api/staff/payments",
        [](const httplib::Request& req, httplib::Response& res)
        {
            int staffID;

            if (!authorizeStaff(req, staffID))
            {
                sendJSON(res,
                {
                    {"error", "Unauthorized"}
                }, 401);

                return;
            }

            try
            {
                json body = json::parse(req.body);

                int invoiceID = body.value("invoiceId", 0);
                double amount = body.value("amount", 0.0);
                string method = body.value("paymentMethod", "");

                if (invoiceID <= 0 || amount <= 0 || method.empty())
                {
                    sendJSON(res,
                    {
                        {"error", "Invalid payment details"}
                    }, 400);

                    return;
                }

                lock_guard<mutex> lock(dbMutex);

                db->setAutoCommit(false);

                try
                {
                    unique_ptr<sql::PreparedStatement> check(
                        db->prepareStatement(
                            "SELECT amount, status FROM invoices "
                            "WHERE invoice_id=? FOR UPDATE"
                        )
                    );

                    check->setInt(1, invoiceID);

                    unique_ptr<sql::ResultSet> result(
                        check->executeQuery()
                    );

                    if (!result->next())
                    {
                        db->rollback();
                        db->setAutoCommit(true);

                        sendJSON(res,
                        {
                            {"error", "Invoice not found"}
                        }, 404);

                        return;
                    }

                    double invoiceAmount =
                        result->getDouble("amount");

                    string status =
                        result->getString("status");

                    if (status == "PAID" ||
                        amount != invoiceAmount)
                    {
                        db->rollback();
                        db->setAutoCommit(true);

                        sendJSON(res,
                        {
                            {"error",
                             "Invoice already paid or amount does not match"}
                        }, 400);

                        return;
                    }

                    unique_ptr<sql::PreparedStatement> insert(
                        db->prepareStatement(
                            "INSERT INTO payments "
                            "(invoice_id, amount, payment_method) "
                            "VALUES (?, ?, ?)"
                        )
                    );

                    insert->setInt(1, invoiceID);
                    insert->setDouble(2, amount);
                    insert->setString(3, method);

                    insert->executeUpdate();

                    unique_ptr<sql::PreparedStatement> update(
                        db->prepareStatement(
                            "UPDATE invoices SET status='PAID' "
                            "WHERE invoice_id=?"
                        )
                    );

                    update->setInt(1, invoiceID);
                    update->executeUpdate();

                    db->commit();
                    db->setAutoCommit(true);

                    sendJSON(res,
                    {
                        {"message", "Payment recorded successfully"}
                    });
                }
                catch (...)
                {
                    db->rollback();
                    db->setAutoCommit(true);
                    throw;
                }
            }
            catch (exception& e)
            {
                sendJSON(res,
                {
                    {"error", e.what()}
                }, 500);
            }
        }
    );
}

// ---------------- REPORTS ----------------

void staffReports(httplib::Server& server)
{
    server.Get(
        "/api/staff/reports",
        [](const httplib::Request& req, httplib::Response& res)
        {
            int staffID;

            if (!authorizeStaff(req, staffID))
            {
                sendJSON(res,
                {
                    {"error", "Unauthorized"}
                }, 401);

                return;
            }

            try
            {
                lock_guard<mutex> lock(dbMutex);

                unique_ptr<sql::Statement> stmt(
                    db->createStatement()
                );

                unique_ptr<sql::ResultSet> result(
                    stmt->executeQuery(
                        "SELECT "
                        "(SELECT COALESCE(SUM(amount), 0) FROM payments) AS revenue,"
                        "(SELECT COUNT(*) FROM invoices WHERE status='PAID') AS paidInvoices,"
                        "(SELECT COUNT(*) FROM invoices WHERE status='PENDING') AS pendingInvoices,"
                        "(SELECT COUNT(*) FROM invoices WHERE status='OVERDUE') AS overdueInvoices"
                    )
                );

                result->next();

                sendJSON(res,
                {
                    {"revenue", result->getDouble("revenue")},
                    {"paidInvoices", result->getInt("paidInvoices")},
                    {"pendingInvoices", result->getInt("pendingInvoices")},
                    {"overdueInvoices", result->getInt("overdueInvoices")}
                });
            }
            catch (exception& e)
            {
                sendJSON(res,
                {
                    {"error", e.what()}
                }, 500);
            }
        }
    );
}

// ---------------- LOGOUT ----------------

void staffLogout(httplib::Server& server)
{
    server.Post(
        "/api/staff/logout",
        [](const httplib::Request& req, httplib::Response& res)
        {
            int staffID;

            if (!authorizeStaff(req, staffID))
            {
                sendJSON(res,
                {
                    {"error", "Unauthorized"}
                }, 401);

                return;
            }

            string header = req.get_header_value("Authorization");
            string token = header.substr(7);

            {
                lock_guard<mutex> lock(tokenMutex);
                staffSessions.erase(token);
            }

            sendJSON(res,
            {
                {"message", "Logged out successfully"}
            });
        }
    );
}

// ---------------- REGISTER ROUTES ----------------

void registerStaffRoutes(httplib::Server& server)
{
    staffLogin(server);
    staffMe(server);
    staffDashboard(server);
    staffCustomers(server);
    staffSubscriptions(server);
    staffInvoices(server);
    staffPayments(server);
    staffReports(server);
    staffLogout(server);
}

// ---------------- MAIN ----------------

int main()
{
    try
    {
        connectDatabase();

        httplib::Server server;

        registerStaffRoutes(server);

        cout << "Staff backend running at http://localhost:8080"
             << endl;

        server.listen("127.0.0.1", 8080);

        delete db;
        db = nullptr;
    }
    catch (exception& e)
    {
        cerr << "Server error: " << e.what() << endl;

        if (db != nullptr)
        {
            delete db;
            db = nullptr;
        }

        return 1;
    }

    return 0;
}
