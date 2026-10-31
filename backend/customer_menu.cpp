
#include "customer.h"
#include "customer_auth.h"

int main()
{
    CustomerAuth auth;
    CustomerPortal portal;

    int choice;
    int customerId = -1;

    while (true)
    {
        cout << "\n===== CUSTOMER PORTAL =====\n";
        cout << "1. Register\n";
        cout << "2. Login\n";
        cout << "3. Exit\n";
        cout << "Enter choice: ";
        cin >> choice;

        if (choice == 1)
        {
            string name, email, password, phone;

            cin.ignore();

            cout << "Enter name: ";
            getline(cin, name);

            cout << "Enter email: ";
            getline(cin, email);

            cout << "Enter phone: ";
            getline(cin, phone);

            cout << "Enter password: ";
            getline(cin, password);

            auth.registerCustomer(name, email, password, phone);
        }
        else if (choice == 2)
        {
            string email, password;

            cin.ignore();

            cout << "Enter email: ";
            getline(cin, email);

            cout << "Enter password: ";
            getline(cin, password);

            customerId = auth.loginCustomer(email, password);

            if (customerId != -1)
            {
                int option;

                do
                {
                    cout << "\n===== CUSTOMER DASHBOARD =====\n";
                    cout << "1. View Profile\n";
                    cout << "2. Update Profile\n";
                    cout << "3. View Plans\n";
                    cout << "4. Request Subscription\n";
                    cout << "5. View Subscriptions\n";
                    cout << "6. View Quotations\n";
                    cout << "7. View Invoices\n";
                    cout << "8. View Payment History\n";
                    cout << "9. Logout\n";
                    cout << "Enter choice: ";
                    cin >> option;

                    switch (option)
                    {
                        case 1:
                            auth.viewProfile(customerId);
                            break;

                        case 2:
                        {
                            string name, phone;

                            cin.ignore();

                            cout << "Enter new name: ";
                            getline(cin, name);

                            cout << "Enter new phone: ";
                            getline(cin, phone);

                            auth.updateProfile(customerId, name, phone);
                            break;
                        }

                        case 3:
                            portal.viewPlans();
                            break;

                        case 4:
                        {
                            int planId;

                            portal.viewPlans();

                            cout << "Enter Plan ID: ";
                            cin >> planId;

                            portal.requestSubscription(customerId, planId);
                            break;
                        }

                        case 5:
                            portal.viewSubscriptions(customerId);
                            break;

                        case 6:
                            portal.viewQuotations(customerId);
                            break;

                        case 7:
                            portal.viewInvoices(customerId);
                            break;

                        case 8:
                            portal.viewPaymentHistory(customerId);
                            break;

                        case 9:
                            cout << "Logged out successfully!\n";
                            break;

                        default:
                            cout << "Invalid choice!\n";
                    }

                } while (option != 9);
            }
        }
        else if (choice == 3)
        {
            cout << "Exiting Customer Portal...\n";
            break;
        }
        else
        {
            cout << "Invalid choice!\n";
        }
    }

    return 0;
}
