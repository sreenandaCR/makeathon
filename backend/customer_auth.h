
#ifndef CUSTOMER_AUTH_H
#define CUSTOMER_AUTH_H

#include "customer.h"
#include <vector>
#include <string>
using namespace std;

class CustomerAuth
{
private:
    vector<Customer> customers;
    int nextCustomerId;
    vector<string> passwords;

public:
    CustomerAuth();

    bool registerCustomer(string name, string email,
                          string password, string phone);

    int loginCustomer(string email, string password);

    void viewProfile(int customerId);
    void updateProfile(int customerId, string name, string phone);
};

#endif
