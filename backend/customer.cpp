
#include "customer.h"

CustomerPortal::CustomerPortal()
{
    // Temporary sample plans
    plans.push_back({1, "Basic", 299, "Monthly"});
    plans.push_back({2, "Standard", 599, "Monthly"});
    plans.push_back({3, "Premium", 999, "Monthly"});
}

void CustomerPortal::viewPlans()
{
    cout << "\n--- AVAILABLE PLANS ---\n";

    for (int i = 0; i < plans.size(); i++)
    {
        cout << plans[i].planId << ". "
             << plans[i].planName
             << " - Rs. " << plans[i].price
             << "/" << plans[i].duration << endl;
    }
}

void CustomerPortal::viewSubscriptions(int customerId)
{
    cout << "\n--- MY SUBSCRIPTIONS ---\n";
    bool found = false;

    for (int i = 0; i < subscriptions.size(); i++)
    {
        if (subscriptions[i].customerId == customerId)
        {
            cout << "Subscription ID: "
                 << subscriptions[i].subscriptionId << endl;
            cout << "Plan ID: "
                 << subscriptions[i].planId << endl;
            cout << "Status: "
                 << subscriptions[i].status << endl;

            found = true;
        }
    }

    if (!found)
        cout << "No subscriptions found.\n";
}

void CustomerPortal::requestSubscription(int customerId, int planId)
{
    bool planExists = false;

    for (int i = 0; i < plans.size(); i++)
    {
        if (plans[i].planId == planId)
        {
            planExists = true;
            break;
        }
    }

    if (!planExists)
    {
        cout << "Invalid plan ID!\n";
        return;
    }

    subscriptions.push_back({
        (int)subscriptions.size() + 1,
        customerId,
        planId,
        "Pending"
    });

    cout << "Subscription request submitted!\n";
}

void CustomerPortal::viewQuotations(int customerId)
{
    cout << "\n--- MY QUOTATIONS ---\n";
    bool found = false;

    for (int i = 0; i < quotations.size(); i++)
    {
        if (quotations[i].customerId == customerId)
        {
            cout << "Quotation ID: "
                 << quotations[i].quotationId << endl;
            cout << "Status: "
                 << quotations[i].status << endl;
            found = true;
        }
    }

    if (!found)
        cout << "No quotations found.\n";
}

void CustomerPortal::viewInvoices(int customerId)
{
    cout << "\n--- MY INVOICES ---\n";
    bool found = false;

    for (int i = 0; i < invoices.size(); i++)
    {
        if (invoices[i].customerId == customerId)
        {
            cout << "Invoice ID: "
                 << invoices[i].invoiceId << endl;
            cout << "Amount: Rs. "
                 << invoices[i].amount << endl;
            cout << "Status: "
                 << invoices[i].status << endl;
            found = true;
        }
    }

    if (!found)
        cout << "No invoices found.\n";
}

void CustomerPortal::viewPaymentHistory(int customerId)
{
    cout << "\n--- PAYMENT HISTORY ---\n";
    bool found = false;

    for (int i = 0; i < payments.size(); i++)
    {
        for (int j = 0; j < invoices.size(); j++)
        {
            if (payments[i].invoiceId == invoices[j].invoiceId &&
                invoices[j].customerId == customerId)
            {
                cout << "Payment ID: "
                     << payments[i].paymentId << endl;
                cout << "Amount: Rs. "
                     << payments[i].amount << endl;
                cout << "Method: "
                     << payments[i].paymentMethod << endl;
                cout << "Status: "
                     << payments[i].status << endl;

                found = true;
            }
        }
    }

    if (!found)
        cout << "No payment records found.\n";
}
