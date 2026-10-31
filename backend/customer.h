
#ifndef CUSTOMER_H
#define CUSTOMER_H

#include <iostream>
#include <string>
#include <vector>
using namespace std;

struct Customer
{
    int customerId;
    string name;
    string email;
    string phone;
};

struct Plan
{
    int planId;
    string planName;
    double price;
    string duration;
};

struct Subscription
{
    int subscriptionId;
    int customerId;
    int planId;
    string status;
};

struct Quotation
{
    int quotationId;
    int customerId;
    int planId;
    string status;
};

struct Invoice
{
    int invoiceId;
    int customerId;
    double amount;
    string status;
};

struct Payment
{
    int paymentId;
    int invoiceId;
    double amount;
    string paymentMethod;
    string status;
};

class CustomerPortal
{
public:
    vector<Plan> plans;
    vector<Subscription> subscriptions;
    vector<Quotation> quotations;
    vector<Invoice> invoices;
    vector<Payment> payments;

    CustomerPortal();

    void viewPlans();
    void viewSubscriptions(int customerId);
    void requestSubscription(int customerId, int planId);
    void viewQuotations(int customerId);
    void viewInvoices(int customerId);
    void viewPaymentHistory(int customerId);
};

#endif
