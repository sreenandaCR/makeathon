
#include "customer.h"
#include <iostream>

using namespace std;

void Customer::registerCustomer(
    string name,
    string email,
    string phone)
{
    this->name = name;
    this->email = email;
    this->phone = phone;

    cout << "\nCustomer registered successfully!" << endl;
}

void Customer::viewProfile()
{
    cout << "\n--- CUSTOMER PROFILE ---" << endl;

    cout << "Name: " << name << endl;
    cout << "Email: " << email << endl;
    cout << "Phone: " << phone << endl;
}

void Customer::updateProfile(
    string name,
    string phone)
{
    this->name = name;
    this->phone = phone;

    cout << "\nProfile updated successfully!" << endl;
}

void Customer::viewPlans()
{
    cout << "\n--- AVAILABLE SUBSCRIPTION PLANS ---" << endl;

    cout << "1. Basic Plan - Rs. 299/month" << endl;
    cout << "2. Standard Plan - Rs. 599/month" << endl;
    cout << "3. Premium Plan - Rs. 999/month" << endl;
}

void Customer::viewSubscriptions()
{
    cout << "\n--- MY SUBSCRIPTIONS ---" << endl;

    cout << "No subscription records available yet." << endl;
}

void Customer::viewPaymentHistory()
{
    cout << "\n--- PAYMENT HISTORY ---" << endl;

    cout << "No payment records available yet." << endl;
}
