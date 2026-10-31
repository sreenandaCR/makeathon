
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
            cout << "\nQuotation ID: "
                 << quotations[i].quotationId << endl;

            cout << "Plan ID: "
                 << quotations[i].planId << endl;

            cout << "Amount: Rs. "
                 << quotations[i].amount << endl;

            cout << "Valid Until: "
                 << quotations[i].validUntil << endl;

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
            cout << "\nInvoice ID: "
                 << invoices[i].invoiceId << endl;

            cout << "Amount: Rs. "
                 << invoices[i].amount << endl;

            cout << "Due Date: "
                 << invoices[i].dueDate << endl;

            cout << "Payment Status: "
                 << invoices[i].status << endl;

            found = true;
        }
    }

    if (!found)
    {
        cout << "No invoices found.\n";
    }
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
                cout << "\nPayment ID: "
                     << payments[i].paymentId << endl;

                cout << "Invoice ID: "
                     << payments[i].invoiceId << endl;

                cout << "Amount: Rs. "
                     << payments[i].amount << endl;

                cout << "Payment Method: "
                     << payments[i].paymentMethod << endl;

                cout << "Payment Date: "
                     << payments[i].paymentDate << endl;

                cout << "Status: "
                     << payments[i].status << endl;

                found = true;
            }
        }
    }

    if (!found)
    {
        cout << "No payment records found.\n";
    }
}


void CustomerPortal::acceptQuotation(int customerId, int quotationId)
{
    for (int i = 0; i < quotations.size(); i++)
    {
        if (quotations[i].quotationId == quotationId &&
            quotations[i].customerId == customerId)
        {
            if (quotations[i].status != "Pending")
            {
                cout << "This quotation cannot be accepted.\n";
                return;
            }

            quotations[i].status = "Accepted";

            cout << "Quotation accepted successfully!\n";
            return;
        }
    }

    cout << "Quotation not found!\n";
}

void CustomerPortal::rejectQuotation(int customerId, int quotationId)
{
    for (int i = 0; i < quotations.size(); i++)
    {
        if (quotations[i].quotationId == quotationId &&
            quotations[i].customerId == customerId)
        {
            if (quotations[i].status != "Pending")
            {
                cout << "This quotation cannot be rejected.\n";
                return;
            }

            quotations[i].status = "Rejected";

            cout << "Quotation rejected successfully!\n";
            return;
        }
    }

    cout << "Quotation not found!\n";
}
