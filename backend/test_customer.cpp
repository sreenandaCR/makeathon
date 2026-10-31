
#include "customer_auth.h"
#include "customer.h"

int main()
{
    CustomerAuth auth;

    cout << "\n--- CUSTOMER REGISTRATION ---\n";

    auth.registerCustomer(
        "Anu",
        "anu@gmail.com",
        "password123",
        "9876543210"
    );

    cout << "\n--- CUSTOMER LOGIN ---\n";

    int customerId = auth.loginCustomer(
        "anu@gmail.com",
        "password123"
    );

    if (customerId != -1)
    {
        cout << "\n--- VIEW PROFILE ---\n";
        auth.viewProfile(customerId);
    }

    return 0;
}
