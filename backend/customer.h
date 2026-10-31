
#ifndef CUSTOMER_H
#define CUSTOMER_H

#include <string>
using namespace std;

class Customer
{
private:
    int customerId;
    string name;
    string email;
    string phone;

public:
    // Register customer
    void registerCustomer(
        string name,
        string email,
        string phone
    );

    // Display customer details
    void viewProfile();

    // Update customer details
    void updateProfile(
        string name,
        string phone
    );

    // View available subscription plans
    void viewPlans();

    // View customer subscriptions
    void viewSubscriptions();

    // View payment history
    void viewPaymentHistory();
};

#endif
