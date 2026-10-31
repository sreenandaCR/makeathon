
#include "crow.h"
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>

using namespace std;

struct Customer {
    int id;
    string name, email, phone, password;
};

struct Plan {
    int id;
    string name;
    double price;
    string duration;
};

struct Subscription {
    int id, customerId, planId;
    string planName, status;
};

struct Quotation {
    int id, customerId;
    string planName, amount, validUntil, status;
};

struct Invoice {
    int id, customerId;
    string amount, dueDate, status;
};

struct Payment {
    int id, customerId;
    string amount, method, date, status;
};

vector<Customer> customers;
vector<Plan> plans = {
    {1, "Basic", 299, "1 Month"},
    {2, "Standard", 599, "1 Month"},
    {3, "Premium", 999, "1 Month"}
};
vector<Subscription> subscriptions;
vector<Quotation> quotations;
vector<Invoice> invoices;
vector<Payment> payments;

int nextCustomerId = 1;
int nextSubscriptionId = 1;

string decode(string value) {
    string result;
    for (int i = 0; i < value.length(); i++) {
        if (value[i] == '+')
            result += ' ';
        else if (value[i] == '%' && i + 2 < value.length()) {
            try {
                result += char(stoi(value.substr(i + 1, 2), nullptr, 16));
                i += 2;
            } catch (...) {
                result += value[i];
            }
        } else result += value[i];
    }
    return result;
}

map<string, string> parseForm(string body) {
    map<string, string> data;
    size_t start = 0;

    while (start < body.length()) {
        size_t end = body.find('&', start);
        string item = body.substr(start,
            end == string::npos ? string::npos : end - start);

        size_t equal = item.find('=');

        if (equal != string::npos) {
            data[decode(item.substr(0, equal))] =
                decode(item.substr(equal + 1));
        }

        if (end == string::npos) break;
        start = end + 1;
    }
    return data;
}

crow::response reply(int status, string message) {
    crow::response res(status, message);
    res.add_header("Access-Control-Allow-Origin", "*");
    res.add_header("Access-Control-Allow-Headers", "Content-Type");
    res.add_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    return res;
}

Customer* findCustomer(int id) {
    for (auto& c : customers)
        if (c.id == id) return &c;
    return nullptr;
}

int main() {
    crow::SimpleApp app;

    // CORS
    CROW_ROUTE(app, "/<path>")
        .methods(crow::HTTPMethod::OPTIONS)
    ([](const crow::request&, string) {
        return reply(204, "");
    });

    // HOME
    CROW_ROUTE(app, "/")
    ([] {
        return reply(200, "Customer server is running!");
    });

    // REGISTER
    CROW_ROUTE(app, "/register")
        .methods(crow::HTTPMethod::POST)
    ([](const crow::request& req) {
        auto d = parseForm(req.body);

        if (!d.count("name") || !d.count("email") ||
            !d.count("phone") || !d.count("password"))
            return reply(400, "All fields are required!");

        if (d["name"].empty() || d["email"].empty() ||
            d["phone"].empty() || d["password"].length() < 8)
            return reply(400, "Invalid details or short password.");

        for (auto& c : customers)
            if (c.email == d["email"])
                return reply(409, "Email already registered!");

        Customer c;
        c.id = nextCustomerId++;
        c.name = d["name"];
        c.email = d["email"];
        c.phone = d["phone"];
        c.password = d["password"];

        customers.push_back(c);

        return reply(200,
            "Registration successful! Customer ID: " + to_string(c.id));
    });

    // LOGIN
    CROW_ROUTE(app, "/login")
        .methods(crow::HTTPMethod::POST)
    ([](const crow::request& req) {
        auto d = parseForm(req.body);

        if (!d.count("email") || !d.count("password"))
            return reply(400, "Enter email and password!");

        for (auto& c : customers) {
            if (c.email == d["email"] &&
                c.password == d["password"])
                return reply(200, to_string(c.id));
        }

        return reply(401, "Invalid email or password!");
    });

    // VIEW PROFILE
    CROW_ROUTE(app, "/profile")
    ([](const crow::request& req) {
        int id = atoi(req.url_params.get("id") ?
                      req.url_params.get("id") : "0");

        Customer* c = findCustomer(id);
        if (!c) return reply(404, "Customer not found!");

        return reply(200,
            to_string(c->id) + "\t" + c->name + "\t" +
            c->email + "\t" + c->phone);
    });

    // UPDATE PROFILE
    CROW_ROUTE(app, "/profile/update")
        .methods(crow::HTTPMethod::POST)
    ([](const crow::request& req) {
        auto d = parseForm(req.body);
        int id = d.count("id") ? atoi(d["id"].c_str()) : 0;

        Customer* c = findCustomer(id);
        if (!c) return reply(404, "Customer not found!");

        if (d.count("name") && !d["name"].empty())
            c->name = d["name"];

        if (d.count("phone") && !d["phone"].empty())
            c->phone = d["phone"];

        return reply(200, "Profile updated successfully!");
    });

    // VIEW PLANS
    CROW_ROUTE(app, "/plans")
    ([] {
        string output = "ID\tPLAN\tPRICE\tDURATION\n";

        for (auto& p : plans) {
            output += to_string(p.id) + "\t" +
                p.name + "\t" + to_string((int)p.price) +
                "\t" + p.duration + "\n";
        }

        return reply(200, output);
    });

    // REQUEST SUBSCRIPTION
    CROW_ROUTE(app, "/subscription/request")
        .methods(crow::HTTPMethod::POST)
    ([](const crow::request& req) {
        auto d = parseForm(req.body);

        int cid = d.count("customerId") ?
                  atoi(d["customerId"].c_str()) : 0;
        int pid = d.count("planId") ?
                  atoi(d["planId"].c_str()) : 0;

        if (!findCustomer(cid))
            return reply(404, "Customer not found!");

        for (auto& p : plans) {
            if (p.id == pid) {
                subscriptions.push_back({
                    nextSubscriptionId++, cid, pid,
                    p.name, "Pending staff approval"
                });

                return reply(200, "Subscription request submitted!");
            }
        }

        return reply(404, "Plan not found!");
    });

    // VIEW SUBSCRIPTIONS
    CROW_ROUTE(app, "/subscriptions")
    ([](const crow::request& req) {
        int id = atoi(req.url_params.get("id") ?
                      req.url_params.get("id") : "0");

        string output = "ID\tPLAN\tSTATUS\n";

        for (auto& s : subscriptions) {
            if (s.customerId == id) {
                output += to_string(s.id) + "\t" +
                    s.planName + "\t" + s.status + "\n";
            }
        }

        return reply(200, output);
    });

    // VIEW QUOTATIONS
    CROW_ROUTE(app, "/quotations")
    ([](const crow::request& req) {
        int id = atoi(req.url_params.get("id") ?
                      req.url_params.get("id") : "0");

        string output = "ID\tPLAN\tAMOUNT\tVALID UNTIL\tSTATUS\n";

        for (auto& q : quotations) {
            if (q.customerId == id) {
                output += to_string(q.id) + "\t" +
                    q.planName + "\t" + q.amount + "\t" +
                    q.validUntil + "\t" + q.status + "\n";
            }
        }

        return reply(200, output);
    });

    // ACCEPT / REJECT QUOTATION
    CROW_ROUTE(app, "/quotation/action")
        .methods(crow::HTTPMethod::POST)
    ([](const crow::request& req) {
        auto d = parseForm(req.body);

        int cid = d.count("customerId") ?
                  atoi(d["customerId"].c_str()) : 0;
        int qid = d.count("quotationId") ?
                  atoi(d["quotationId"].c_str()) : 0;

        string action = d.count("action") ? d["action"] : "";

        for (auto& q : quotations) {
            if (q.id == qid && q.customerId == cid &&
                q.status == "Pending") {

                if (action == "accept")
                    q.status = "Accepted";
                else if (action == "reject")
                    q.status = "Rejected";
                else
                    return reply(400, "Invalid action!");

                return reply(200, "Quotation " + q.status + "!");
            }
        }

        return reply(404, "Quotation not found!");
    });

    // VIEW INVOICES
    CROW_ROUTE(app, "/invoices")
    ([](const crow::request& req) {
        int id = atoi(req.url_params.get("id") ?
                      req.url_params.get("id") : "0");

        string output = "ID\tAMOUNT\tDUE DATE\tSTATUS\n";

        for (auto& i : invoices) {
            if (i.customerId == id) {
                output += to_string(i.id) + "\t" +
                    i.amount + "\t" + i.dueDate + "\t" +
                    i.status + "\n";
            }
        }

        return reply(200, output);
    });

    // VIEW PAYMENT HISTORY
    CROW_ROUTE(app, "/payments")
    ([](const crow::request& req) {
        int id = atoi(req.url_params.get("id") ?
                      req.url_params.get("id") : "0");

        string output = "ID\tAMOUNT\tMETHOD\tDATE\tSTATUS\n";

        for (auto& p : payments) {
            if (p.customerId == id) {
                output += to_string(p.id) + "\t" +
                    p.amount + "\t" + p.method + "\t" +
                    p.date + "\t" + p.status + "\n";
            }
        }

        return reply(200, output);
    });

    cout << "Customer server running on port 18080\n";
    app.port(18080).multithreaded().run();

    return 0;
}
