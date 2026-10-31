#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <winsock2.h>
#include <windows.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

#include "crow_all.h"
#include "json.hpp"

#include "database.h"
#include "auth.h"
#include "admin.h"
#include "customer_auth.h"
#include "customer.h"

using namespace std;
using json = nlohmann::json;

static string findFrontendDir()
{
    const vector<string> candidates = {"frontend", "../frontend", "../../frontend"};
    for (const auto& dir : candidates)
    {
        ifstream f(dir + "/login.html");
        if (f.is_open()) return dir;
    }
    return "frontend";
}

static crow::response serveStaticFile(const string& relativePath, const string& contentType)
{
    string baseDir = findFrontendDir();
    string fullPath = baseDir + "/" + relativePath;
    ifstream file(fullPath, ios::binary);
    if (!file.is_open())
    {
        return crow::response(404, "text/plain", "File not found: " + relativePath);
    }
    ostringstream ss;
    ss << file.rdbuf();
    crow::response res(200, contentType, ss.str());
    res.add_header("Access-Control-Allow-Origin", "*");
    return res;
}

static string extractBearerToken(const crow::request& req)
{
    string header = req.get_header_value("Authorization");
    if (header.rfind("Bearer ", 0) == 0)
    {
        return header.substr(7);
    }
    return "";
}

static bool authenticateRequest(
    const crow::request& req,
    UserRecord& outUser,
    crow::response& outRes,
    const string& requiredRole = "")
{
    string token = extractBearerToken(req);
    if (token.empty())
    {
        json err = {{"error", "Unauthorized: Missing Bearer authorization token."}};
        outRes = crow::response(401, "application/json", err.dump());
        outRes.add_header("Access-Control-Allow-Origin", "*");
        return false;
    }

    if (!SessionManager::getInstance().getSession(token, outUser))
    {
        json err = {{"error", "Unauthorized: Session is invalid or has expired."}};
        outRes = crow::response(401, "application/json", err.dump());
        outRes.add_header("Access-Control-Allow-Origin", "*");
        return false;
    }

    if (outUser.status != "ACTIVE")
    {
        json err = {{"error", "Forbidden: Account is inactive. Please contact an administrator."}};
        outRes = crow::response(403, "application/json", err.dump());
        outRes.add_header("Access-Control-Allow-Origin", "*");
        return false;
    }

    if (!requiredRole.empty() && outUser.role != requiredRole)
    {
        json err = {{"error", "Forbidden: Insufficient permissions for this role."}};
        outRes = crow::response(403, "application/json", err.dump());
        outRes.add_header("Access-Control-Allow-Origin", "*");
        return false;
    }

    return true;
}

static void addCors(crow::response& res)
{
    res.add_header("Access-Control-Allow-Origin", "*");
    res.add_header("Access-Control-Allow-Headers", "Content-Type, Authorization");
    res.add_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
}

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
        cout << "Database connection failed: " << db.getLastError() << endl;
        cout << "Ensure MySQL Server 8.0 is running on port 3306 and DB_PASSWORD / .env is set properly." << endl;
        return 1;
    }

    cout << "Database connected successfully!" << endl;

    // Create predefined Admin account
    Admin admin;

    admin.createAdmin();

    cout << "Backend initialization completed!" << endl;

    crow::SimpleApp app;

    // Handle CORS preflight options
    CROW_ROUTE(app, "/api/<path>")
        .methods(crow::HTTPMethod::Options)
    ([](const crow::request&, string) {
        crow::response res(200);
        addCors(res);
        return res;
    });

    // -------------------------------------------------------------
    // Public Endpoints
    // -------------------------------------------------------------

    // POST /api/auth/login
    CROW_ROUTE(app, "/api/auth/login").methods(crow::HTTPMethod::Post)
    ([&db](const crow::request& req) {
        crow::response res;
        addCors(res);
        try
        {
            auto body = json::parse(req.body);
            string email = body.value("email", "");
            string password = body.value("password", "");

            Auth auth;
            AuthResult authRes = auth.authenticate(db, email, password);
            if (!authRes.success)
            {
                json err = {{"error", authRes.errorMessage}};
                res.code = (authRes.errorMessage.find("inactive") != string::npos) ? 403 : 401;
                res.body = err.dump();
                res.set_header("Content-Type", "application/json");
                return res;
            }

            json data = {
                {"token", authRes.token},
                {"user", {
                    {"user_id", authRes.user.userId},
                    {"name", authRes.user.name},
                    {"email", authRes.user.email},
                    {"role", authRes.user.role},
                    {"status", authRes.user.status},
                    {"must_change_password", authRes.user.mustChangePassword}
                }}
            };
            res.code = 200;
            res.body = data.dump();
            res.set_header("Content-Type", "application/json");
            return res;
        }
        catch (const std::exception& e)
        {
            json err = {{"error", string("Bad Request: ") + e.what()}};
            res.code = 400;
            res.body = err.dump();
            res.set_header("Content-Type", "application/json");
            return res;
        }
    });

    // POST /api/auth/register-customer
    CROW_ROUTE(app, "/api/auth/register-customer").methods(crow::HTTPMethod::Post)
    ([&db](const crow::request& req) {
        crow::response res;
        addCors(res);
        try
        {
            auto body = json::parse(req.body);
            string name = body.value("name", "");
            string email = body.value("email", "");
            string password = body.value("password", "");
            string confirmPassword = body.value("confirm_password", "");

            CustomerAuth custAuth;
            string errorMsg;
            if (!custAuth.registerCustomer(db, name, email, password, confirmPassword, errorMsg))
            {
                json err = {{"error", errorMsg}};
                res.code = 400;
                res.body = err.dump();
                res.set_header("Content-Type", "application/json");
                return res;
            }

            json data = {{"message", "Customer account registered successfully."}};
            res.code = 201;
            res.body = data.dump();
            res.set_header("Content-Type", "application/json");
            return res;
        }
        catch (const std::exception& e)
        {
            json err = {{"error", string("Bad Request: ") + e.what()}};
            res.code = 400;
            res.body = err.dump();
            res.set_header("Content-Type", "application/json");
            return res;
        }
    });

    // -------------------------------------------------------------
    // Authenticated Generic Endpoints (Admin, Staff, Customer)
    // -------------------------------------------------------------

    // GET /api/auth/me
    CROW_ROUTE(app, "/api/auth/me").methods(crow::HTTPMethod::Get)
    ([](const crow::request& req) {
        UserRecord user;
        crow::response res;
        if (!authenticateRequest(req, user, res)) return res;
        addCors(res);

        json data = {
            {"user", {
                {"user_id", user.userId},
                {"name", user.name},
                {"email", user.email},
                {"role", user.role},
                {"status", user.status},
                {"must_change_password", user.mustChangePassword}
            }}
        };
        res.code = 200;
        res.body = data.dump();
        res.set_header("Content-Type", "application/json");
        return res;
    });

    // POST /api/auth/logout
    CROW_ROUTE(app, "/api/auth/logout").methods(crow::HTTPMethod::Post)
    ([](const crow::request& req) {
        string token = extractBearerToken(req);
        if (!token.empty())
        {
            SessionManager::getInstance().invalidateSession(token);
        }
        crow::response res(200, "application/json", json({{"message", "Logged out successfully."}}).dump());
        addCors(res);
        return res;
    });

    // POST /api/auth/change-password
    CROW_ROUTE(app, "/api/auth/change-password").methods(crow::HTTPMethod::Post)
    ([&db](const crow::request& req) {
        UserRecord user;
        crow::response res;
        if (!authenticateRequest(req, user, res)) return res;
        addCors(res);

        try
        {
            auto body = json::parse(req.body);
            string oldPassword = body.value("old_password", "");
            string newPassword = body.value("new_password", "");

            Auth auth;
            string errorMsg;
            bool success = false;

            if (user.mustChangePassword && oldPassword.empty())
            {
                // First-login password change
                success = auth.changeTemporaryPassword(db, user.userId, newPassword, errorMsg);
            }
            else
            {
                // Standard password change requires verifying old password
                success = auth.changePassword(db, user.userId, oldPassword, newPassword, errorMsg);
            }

            if (!success)
            {
                res.code = 400;
                res.body = json({{"error", errorMsg}}).dump();
                res.set_header("Content-Type", "application/json");
                return res;
            }

            // Update user in session
            user.mustChangePassword = false;
            SessionManager::getInstance().updateSessionUser(extractBearerToken(req), user);

            res.code = 200;
            res.body = json({{"message", "Password changed successfully."}}).dump();
            res.set_header("Content-Type", "application/json");
            return res;
        }
        catch (const std::exception& e)
        {
            res.code = 400;
            res.body = json({{"error", string("Bad Request: ") + e.what()}}).dump();
            res.set_header("Content-Type", "application/json");
            return res;
        }
    });

    // -------------------------------------------------------------
    // Admin Endpoints (Admin Only)
    // -------------------------------------------------------------

    // POST /api/admin/create-admin
    CROW_ROUTE(app, "/api/admin/create-admin").methods(crow::HTTPMethod::Post)
    ([&db, &admin](const crow::request& req) {
        UserRecord user;
        crow::response res;
        if (!authenticateRequest(req, user, res, "ADMIN")) return res;
        addCors(res);

        try
        {
            auto body = json::parse(req.body);
            string name = body.value("name", "");
            string email = body.value("email", "");

            string tempPassword;
            string errorMsg;
            if (!admin.addAdmin(db, name, email, tempPassword, errorMsg))
            {
                res.code = 400;
                res.body = json({{"error", errorMsg}}).dump();
                res.set_header("Content-Type", "application/json");
                return res;
            }

            res.code = 201;
            res.body = json({
                {"message", "Admin account created successfully."},
                {"temporary_password", tempPassword},
                {"email", email},
                {"name", name}
            }).dump();
            res.set_header("Content-Type", "application/json");
            return res;
        }
        catch (const std::exception& e)
        {
            res.code = 400;
            res.body = json({{"error", string("Bad Request: ") + e.what()}}).dump();
            res.set_header("Content-Type", "application/json");
            return res;
        }
    });

    // POST /api/admin/create-staff
    CROW_ROUTE(app, "/api/admin/create-staff").methods(crow::HTTPMethod::Post)
    ([&db, &admin](const crow::request& req) {
        UserRecord user;
        crow::response res;
        if (!authenticateRequest(req, user, res, "ADMIN")) return res;
        addCors(res);

        try
        {
            auto body = json::parse(req.body);
            string name = body.value("name", "");
            string email = body.value("email", "");

            string tempPassword;
            string errorMsg;
            if (!admin.addStaff(db, name, email, tempPassword, errorMsg))
            {
                res.code = 400;
                res.body = json({{"error", errorMsg}}).dump();
                res.set_header("Content-Type", "application/json");
                return res;
            }

            res.code = 201;
            res.body = json({
                {"message", "Staff account created successfully."},
                {"temporary_password", tempPassword},
                {"email", email},
                {"name", name}
            }).dump();
            res.set_header("Content-Type", "application/json");
            return res;
        }
        catch (const std::exception& e)
        {
            res.code = 400;
            res.body = json({{"error", string("Bad Request: ") + e.what()}}).dump();
            res.set_header("Content-Type", "application/json");
            return res;
        }
    });

    // GET /api/admin/admins
    CROW_ROUTE(app, "/api/admin/admins").methods(crow::HTTPMethod::Get)
    ([&db, &admin](const crow::request& req) {
        UserRecord user;
        crow::response res;
        if (!authenticateRequest(req, user, res, "ADMIN")) return res;
        addCors(res);

        auto admins = admin.listAdmins(db);
        json list = json::array();
        for (const auto& a : admins)
        {
            list.push_back({
                {"user_id", a.userId},
                {"name", a.name},
                {"email", a.email},
                {"role", a.role},
                {"status", a.status},
                {"must_change_password", a.mustChangePassword},
                {"created_at", a.createdAt}
            });
        }
        res.code = 200;
        res.body = json({{"admins", list}}).dump();
        res.set_header("Content-Type", "application/json");
        return res;
    });

    // GET /api/admin/staff
    CROW_ROUTE(app, "/api/admin/staff").methods(crow::HTTPMethod::Get)
    ([&db, &admin](const crow::request& req) {
        UserRecord user;
        crow::response res;
        if (!authenticateRequest(req, user, res, "ADMIN")) return res;
        addCors(res);

        auto staffList = admin.listStaff(db);
        json list = json::array();
        for (const auto& s : staffList)
        {
            list.push_back({
                {"user_id", s.userId},
                {"name", s.name},
                {"email", s.email},
                {"role", s.role},
                {"status", s.status},
                {"must_change_password", s.mustChangePassword},
                {"created_at", s.createdAt}
            });
        }
        res.code = 200;
        res.body = json({{"staff", list}}).dump();
        res.set_header("Content-Type", "application/json");
        return res;
    });

    // GET /api/admin/customers
    CROW_ROUTE(app, "/api/admin/customers").methods(crow::HTTPMethod::Get)
    ([&db, &admin](const crow::request& req) {
        UserRecord user;
        crow::response res;
        if (!authenticateRequest(req, user, res, "ADMIN")) return res;
        addCors(res);

        auto customers = admin.listCustomers(db);
        json list = json::array();
        for (const auto& c : customers)
        {
            list.push_back({
                {"user_id", c.userId},
                {"name", c.name},
                {"email", c.email},
                {"role", c.role},
                {"status", c.status},
                {"created_at", c.createdAt}
            });
        }
        res.code = 200;
        res.body = json({{"customers", list}}).dump();
        res.set_header("Content-Type", "application/json");
        return res;
    });

    // POST /api/admin/toggle-status
    CROW_ROUTE(app, "/api/admin/toggle-status").methods(crow::HTTPMethod::Post)
    ([&db, &admin](const crow::request& req) {
        UserRecord user;
        crow::response res;
        if (!authenticateRequest(req, user, res, "ADMIN")) return res;
        addCors(res);

        try
        {
            auto body = json::parse(req.body);
            int targetUserId = body.value("user_id", 0);
            string status = body.value("status", "");

            string errorMsg;
            if (!admin.toggleUserStatus(db, targetUserId, status, errorMsg))
            {
                res.code = 400;
                res.body = json({{"error", errorMsg}}).dump();
                res.set_header("Content-Type", "application/json");
                return res;
            }

            res.code = 200;
            res.body = json({{"message", "User status updated successfully."}}).dump();
            res.set_header("Content-Type", "application/json");
            return res;
        }
        catch (const std::exception& e)
        {
            res.code = 400;
            res.body = json({{"error", string("Bad Request: ") + e.what()}}).dump();
            res.set_header("Content-Type", "application/json");
            return res;
        }
    });

    // -------------------------------------------------------------
    // Staff Endpoints (Staff and Admin)
    // -------------------------------------------------------------

    // GET /api/staff/customers (Shared customer list retrieved directly from MySQL)
    CROW_ROUTE(app, "/api/staff/customers").methods(crow::HTTPMethod::Get)
    ([&db](const crow::request& req) {
        UserRecord user;
        crow::response res;
        // Staff or Admin can access shared customer list
        if (!authenticateRequest(req, user, res)) return res;
        addCors(res);

        if (user.role != "STAFF" && user.role != "ADMIN")
        {
            res.code = 403;
            res.body = json({{"error", "Forbidden: Access restricted to Staff and Admin."}}).dump();
            res.set_header("Content-Type", "application/json");
            return res;
        }

        auto customers = db.getUsersByRole("CUSTOMER");
        json list = json::array();
        for (const auto& c : customers)
        {
            list.push_back({
                {"user_id", c.userId},
                {"name", c.name},
                {"email", c.email},
                {"status", c.status},
                {"created_at", c.createdAt}
            });
        }
        res.code = 200;
        res.body = json({{"customers", list}}).dump();
        res.set_header("Content-Type", "application/json");
        return res;
    });

    // -------------------------------------------------------------
    // Customer Endpoints (Customer Only)
    // -------------------------------------------------------------

    // GET /api/customer/profile
    CROW_ROUTE(app, "/api/customer/profile").methods(crow::HTTPMethod::Get)
    ([&db](const crow::request& req) {
        UserRecord user;
        crow::response res;
        if (!authenticateRequest(req, user, res, "CUSTOMER")) return res;
        addCors(res);

        Customer cust;
        UserRecord profile;
        string errorMsg;
        if (!cust.getProfile(db, user.userId, profile, errorMsg))
        {
            res.code = 404;
            res.body = json({{"error", errorMsg}}).dump();
            res.set_header("Content-Type", "application/json");
            return res;
        }

        json data = {
            {"customer", {
                {"user_id", profile.userId},
                {"name", profile.name},
                {"email", profile.email},
                {"status", profile.status},
                {"created_at", profile.createdAt}
            }}
        };
        res.code = 200;
        res.body = data.dump();
        res.set_header("Content-Type", "application/json");
        return res;
    });

    // -------------------------------------------------------------
    // Frontend Static File Serving Routes
    // -------------------------------------------------------------

    CROW_ROUTE(app, "/")([]() {
        return serveStaticFile("login.html", "text/html");
    });

    CROW_ROUTE(app, "/login.html")([]() {
        return serveStaticFile("login.html", "text/html");
    });

    CROW_ROUTE(app, "/register.html")([]() {
        return serveStaticFile("register.html", "text/html");
    });

    CROW_ROUTE(app, "/admin.html")([]() {
        return serveStaticFile("admin.html", "text/html");
    });

    CROW_ROUTE(app, "/staff.html")([]() {
        return serveStaticFile("staff.html", "text/html");
    });

    CROW_ROUTE(app, "/customer.html")([]() {
        return serveStaticFile("customer.html", "text/html");
    });

    CROW_ROUTE(app, "/css/<string>")([](string file) {
        return serveStaticFile("css/" + file, "text/css");
    });

    CROW_ROUTE(app, "/js/<string>")([](string file) {
        return serveStaticFile("js/" + file, "application/javascript");
    });

    // Determine port from environment or default 8080
    char* envPort = getenv("PORT");
    int port = envPort ? std::atoi(envPort) : 8080;

    cout << "Crow REST API Server starting on http://localhost:" << port << endl;
    cout << "Press Ctrl+C to terminate server." << endl;

    app.port(port).multithreaded().run();

    return 0;
}

