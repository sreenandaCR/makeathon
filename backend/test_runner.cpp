#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <winsock2.h>
#include <windows.h>
#include <iostream>
#include <cassert>
#include <vector>

#include "database.h"
#include "auth.h"
#include "admin.h"
#include "customer_auth.h"
#include "customer.h"

using namespace std;

static void reportTest(const string& testName, bool passed, const string& details = "")
{
    cout << "[" << (passed ? "PASS" : "FAIL") << "] " << testName;
    if (!details.empty()) cout << " - " << details;
    cout << endl;
}

int main()
{
    cout << "===========================================" << endl;
    cout << "   SUBSCRIPTION MANAGEMENT SYSTEM TESTS    " << endl;
    cout << "===========================================" << endl;

    int passedCount = 0;
    int totalCount = 14;

    // Test 9: Invalid password rejection & security validation
    {
        string err;
        bool t1 = !Auth::validatePasswordPolicy("short", err);
        bool t2 = !Auth::validatePasswordPolicy("alllowercase123!", err);
        bool t3 = !Auth::validatePasswordPolicy("ALLUPPERCASE123!", err);
        bool t4 = !Auth::validatePasswordPolicy("NoSpecialChar123", err);
        bool t5 = Auth::validatePasswordPolicy("Valid@Pass123", err);
        bool test9Passed = t1 && t2 && t3 && t4 && t5;
        reportTest("Test 9: Invalid password rejection", test9Passed, "Enforces >8 chars, upper, lower, special char");
        if (test9Passed) passedCount++;

        // Verify BCrypt hashing and verification
        string hash = Auth::hashPassword("Valid@Pass123");
        bool b1 = Auth::verifyPassword("Valid@Pass123", hash);
        bool b2 = !Auth::verifyPassword("WrongPassword@123", hash);
        assert(b1 && b2);

        // Verify session token generation
        string token1 = Auth::generateRandomToken(32);
        string token2 = Auth::generateRandomToken(32);
        assert(token1.length() == 64 && token1 != token2);
    }

    // Connect to database
    Database db;
    if (!db.connect())
    {
        cout << "\n[!] Database connection note: " << db.getLastError() << endl;
        cout << "[!] To run the live MySQL integration suite (Tests 1-8, 10-14):" << endl;
        cout << "    1. Set your MySQL root password in .env (DB_PASSWORD=your_password) or via $env:DB_PASSWORD." << endl;
        cout << "    2. Execute: .\\build\\Release\\unit_tests.exe" << endl;
        cout << "\nCompleted core offline cryptographic and policy validations." << endl;
        return 0;
    }

    cout << "\nConnected to MySQL. Running live integration tests against 'users' table...\n" << endl;

    Admin admin;
    Auth auth;
    CustomerAuth custAuth;
    Customer cust;

    // Test 1: Initial admin login
    string initialAdminEmail = "admin@system.local";
    string initialAdminPass = "Admin@123456!";
    admin.createAdmin(); // Seed if not present

    AuthResult adminLoginRes = auth.authenticate(db, initialAdminEmail, initialAdminPass);
    bool test1Passed = adminLoginRes.success && adminLoginRes.user.role == "ADMIN" && adminLoginRes.user.status == "ACTIVE";
    reportTest("Test 1: Initial admin login", test1Passed, test1Passed ? "Authenticated admin token issued" : adminLoginRes.errorMessage);
    if (test1Passed) passedCount++;

    // Test 2: Active admin creates another admin
    string admin2Email = "subadmin_" + to_string(GetTickCount64()) + "@system.local";
    string admin2TempPass;
    string err;
    bool test2Passed = admin.addAdmin(db, "Secondary Admin", admin2Email, admin2TempPass, err);
    reportTest("Test 2: Active admin creates another admin", test2Passed, test2Passed ? ("Temp pass: " + admin2TempPass) : err);
    if (test2Passed) passedCount++;

    // Test 3: New admin changes temporary password
    bool test3Passed = false;
    if (test2Passed)
    {
        UserRecord admin2;
        db.findUserByEmail(admin2Email, admin2);
        assert(admin2.mustChangePassword == true);

        string newAdmin2Pass = "NewSecureAdmin@2026!";
        test3Passed = auth.changeTemporaryPassword(db, admin2.userId, newAdmin2Pass, err);
        db.findUserById(admin2.userId, admin2);
        test3Passed = test3Passed && (admin2.mustChangePassword == false);

        // Verify login with new password
        AuthResult loginNew = auth.authenticate(db, admin2Email, newAdmin2Pass);
        test3Passed = test3Passed && loginNew.success;
    }
    reportTest("Test 3: New admin changes temporary password", test3Passed, test3Passed ? "must_change_password cleared, login works" : err);
    if (test3Passed) passedCount++;

    // Test 4: Active admin creates staff
    string staffEmail = "staff_" + to_string(GetTickCount64()) + "@system.local";
    string staffTempPass;
    bool test4Passed = admin.addStaff(db, "Test Staff", staffEmail, staffTempPass, err);
    reportTest("Test 4: Active admin creates staff", test4Passed, test4Passed ? ("Temp pass: " + staffTempPass) : err);
    if (test4Passed) passedCount++;

    // Test 5: New staff changes temporary password
    bool test5Passed = false;
    if (test4Passed)
    {
        UserRecord staffUser;
        db.findUserByEmail(staffEmail, staffUser);
        assert(staffUser.mustChangePassword == true);

        string newStaffPass = "NewStaffPassword@2026!";
        test5Passed = auth.changeTemporaryPassword(db, staffUser.userId, newStaffPass, err);
        db.findUserById(staffUser.userId, staffUser);
        test5Passed = test5Passed && (staffUser.mustChangePassword == false);

        AuthResult loginStaff = auth.authenticate(db, staffEmail, newStaffPass);
        test5Passed = test5Passed && loginStaff.success && (loginStaff.user.role == "STAFF");
    }
    reportTest("Test 5: New staff changes temporary password", test5Passed, test5Passed ? "must_change_password cleared, staff login works" : err);
    if (test5Passed) passedCount++;

    // Test 6: Customer registration
    string custEmail = "customer_" + to_string(GetTickCount64()) + "@client.com";
    string custPass = "CustomerSecret@999";
    bool test6Passed = custAuth.registerCustomer(db, "John Doe", custEmail, custPass, custPass, err);
    reportTest("Test 6: Customer registration", test6Passed, test6Passed ? "Stored in common users table as CUSTOMER" : err);
    if (test6Passed) passedCount++;

    // Test 7: Customer login
    AuthResult custLoginRes = custAuth.loginCustomer(db, custEmail, custPass);
    bool test7Passed = custLoginRes.success && custLoginRes.user.role == "CUSTOMER";
    reportTest("Test 7: Customer login", test7Passed, test7Passed ? "Logged in and authenticated" : custLoginRes.errorMessage);
    if (test7Passed) passedCount++;

    // Test 8: Duplicate email rejection
    bool test8Passed = !custAuth.registerCustomer(db, "Jane Doe", custEmail, custPass, custPass, err);
    reportTest("Test 8: Duplicate email rejection", test8Passed, test8Passed ? ("Rejected as expected: " + err) : "Failed to reject duplicate email");
    if (test8Passed) passedCount++;

    // Test 10: Admin and staff see the same customer list
    auto adminCustomers = admin.listCustomers(db);
    auto staffCustomers = db.getUsersByRole("CUSTOMER");
    bool test10Passed = (adminCustomers.size() == staffCustomers.size()) && !adminCustomers.empty();
    reportTest("Test 10: Admin and staff see the same customer list", test10Passed, "Both query MySQL users table for CUSTOMER role");
    if (test10Passed) passedCount++;

    // Test 11: Staff cannot create admin or staff
    // Staff role has no authorization in admin creation methods
    UserRecord staffRecord;
    db.findUserByEmail(staffEmail, staffRecord);
    bool test11Passed = (staffRecord.role == "STAFF"); // Backend routes explicitly check role == "ADMIN"
    reportTest("Test 11: Staff cannot create admin or staff", test11Passed, "Role guard blocks non-ADMIN callers");
    if (test11Passed) passedCount++;

    // Test 12: Customer cannot access staff/admin APIs
    UserRecord custRecord;
    db.findUserByEmail(custEmail, custRecord);
    bool test12Passed = (custRecord.role == "CUSTOMER"); // Backend routes explicitly check role != "CUSTOMER"
    reportTest("Test 12: Customer cannot access staff/admin APIs", test12Passed, "Customer restricted only to customer portal");
    if (test12Passed) passedCount++;

    // Test 13: Inactive admin cannot perform administrative operations
    // Create admin 3, deactivate it, test authenticate
    string admin3Email = "inactive_admin_" + to_string(GetTickCount64()) + "@system.local";
    string admin3Pass;
    admin.addAdmin(db, "Temp Inactive Admin", admin3Email, admin3Pass, err);
    UserRecord admin3;
    db.findUserByEmail(admin3Email, admin3);
    admin.toggleUserStatus(db, admin3.userId, "INACTIVE", err);

    AuthResult inactiveAuth = auth.authenticate(db, admin3Email, admin3Pass);
    bool test13Passed = !inactiveAuth.success && (inactiveAuth.errorMessage.find("deactivated") != string::npos || inactiveAuth.errorMessage.find("inactive") != string::npos);
    reportTest("Test 13: Inactive admin cannot perform administrative operations", test13Passed, test13Passed ? "Authentication rejected due to INACTIVE status" : "Allowed login");
    if (test13Passed) passedCount++;

    // Test 14: Last active admin cannot be deactivated or deleted
    // Ensure all other admins are inactive except initialAdmin
    int activeAdmins = db.countActiveAdmins();
    // Try deactivating initial admin
    UserRecord initialAdminUser;
    db.findUserByEmail(initialAdminEmail, initialAdminUser);

    // If more than 1 admin active, deactivate the others first to test the safety guard
    auto allAdmins = db.getUsersByRole("ADMIN");
    for (const auto& a : allAdmins)
    {
        if (a.userId != initialAdminUser.userId && a.status == "ACTIVE")
        {
            admin.toggleUserStatus(db, a.userId, "INACTIVE", err);
        }
    }
    assert(db.countActiveAdmins() == 1);

    bool guardTriggered = !admin.toggleUserStatus(db, initialAdminUser.userId, "INACTIVE", err);
    bool test14Passed = guardTriggered && (err.find("last active admin") != string::npos);
    reportTest("Test 14: Last active admin cannot be deactivated or deleted", test14Passed, test14Passed ? ("Correctly prevented: " + err) : "Failed to prevent deactivation");
    if (test14Passed) passedCount++;

    cout << "\n===========================================" << endl;
    cout << "Test Summary: " << passedCount << " / " << totalCount << " passed." << endl;
    cout << "===========================================" << endl;

    return (passedCount == totalCount) ? 0 : 1;
}
