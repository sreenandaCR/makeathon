
#include "customer_auth.h"

CustomerAuth::CustomerAuth()
{
    nextCustomerId = 1;
}

bool CustomerAuth::registerCustomer(
    string name, string email, string password, string phone)
{
    if (name.empty() || email.empty() ||
        password.empty() || phone.empty())
    {
        cout << "All fields are required!\n";
        return false;
    }

    if (password.length() < 8)
    {
        cout << "Password must contain at least 8 characters!\n";
        return false;
    }

    // Check whether email already exists
    for (int i = 0; i < customers.size(); i++)
    {
        if (customers[i].email == email)
        {
            cout << "Email already registered!\n";
            return false;
        }
    }

    Customer newCustomer;

    newCustomer.customerId = nextCustomerId++;
    newCustomer.name = name;
    newCustomer.email = email;
    newCustomer.phone = phone;

    customers.push_back(newCustomer);
    passwords.push_back(password);

    cout << "Registration successful!\n";
    cout << "Your Customer ID is: "
         << newCustomer.customerId << endl;

    return true;
}

int CustomerAuth::loginCustomer(string email, string password)
{
    for (int i = 0; i < customers.size(); i++)
    {
        if (customers[i].email == email &&
            passwords[i] == password)
        {
            cout << "Login successful!\n";
            return customers[i].customerId;
        }
    }

    cout << "Invalid email or password!\n";
    return -1;
}

void CustomerAuth::viewProfile(int customerId)
{
    for (int i = 0; i < customers.size(); i++)
    {
        if (customers[i].customerId == customerId)
        {
            cout << "\n--- CUSTOMER PROFILE ---\n";
            cout << "Customer ID: "
                 << customers[i].customerId << endl;
            cout << "Name: " << customers[i].name << endl;
            cout << "Email: " << customers[i].email << endl;
            cout << "Phone: " << customers[i].phone << endl;
            return;
        }
    }

    cout << "Customer not found!\n";
}

void CustomerAuth::updateProfile(
    int customerId, string name, string phone)
{
    for (int i = 0; i < customers.size(); i++)
    {
        if (customers[i].customerId == customerId)
        {
            if (name.empty() || phone.empty())
            {
                cout << "Name and phone cannot be empty!\n";
                return;
            }

            customers[i].name = name;
            customers[i].phone = phone;

            cout << "Profile updated successfully!\n";
            return;
        }
    }

    cout << "Customer not found!\n";
}
