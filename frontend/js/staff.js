
const API = "http://localhost:8080/api/staff";

// Get authentication token
function getToken() {
    return sessionStorage.getItem("token");
}

// Common API request function
async function apiRequest(path, options = {}) {

    const response = await fetch(API + path, {
        ...options,
        headers: {
            "Content-Type": "application/json",
            "Authorization": "Bearer " + getToken(),
            ...options.headers
        }
    });

    if (response.status === 401 || response.status === 403) {
        alert("Session expired or access denied.");
        logout();
        throw new Error("Access denied");
    }

    if (!response.ok) {
        const error = await response.json().catch(() => ({}));
        throw new Error(error.error || "Request failed: " + response.status);
    }

    return response.json();
}

// Show selected section
function showSection(sectionId) {

    document.querySelectorAll(".section").forEach(section => {
        section.classList.add("hidden");
    });

    const selected = document.getElementById(sectionId);

    if (!selected) return;

    selected.classList.remove("hidden");

    if (sectionId === "dashboard") loadDashboard();
    if (sectionId === "customers") loadCustomers();
    if (sectionId === "subscriptions") loadSubscriptions();
    if (sectionId === "invoices") loadInvoices();
    if (sectionId === "reports") loadReports();
}

// Dashboard
async function loadDashboard() {

    try {
        const data = await apiRequest("/dashboard");

        document.getElementById("customerCount").textContent =
            data.customers ?? 0;

        document.getElementById("subscriptionCount").textContent =
            data.subscriptions ?? 0;

        document.getElementById("invoiceCount").textContent =
            data.invoices ?? 0;

        document.getElementById("paymentCount").textContent =
            data.payments ?? 0;

    } catch (error) {
        console.error("Dashboard error:", error);
    }
}

// Customers
async function loadCustomers() {

    try {
        const customers = await apiRequest("/customers");

        document.getElementById("customerTable").innerHTML =
            customers.map(c => `
                <tr>
                    <td>${escapeHTML(c.id)}</td>
                    <td>${escapeHTML(c.name)}</td>
                    <td>${escapeHTML(c.email)}</td>
                    <td>${escapeHTML(c.phone)}</td>
                </tr>
            `).join("");

    } catch (error) {
        console.error(error);
        alert("Unable to load customers.");
    }
}

// Subscriptions
async function loadSubscriptions() {

    try {
        const data = await apiRequest("/subscriptions");

        document.getElementById("subscriptionTable").innerHTML =
            data.map(s => `
                <tr>
                    <td>${escapeHTML(s.id)}</td>
                    <td>${escapeHTML(s.customer_id)}</td>
                    <td>${escapeHTML(s.plan)}</td>
                    <td>${escapeHTML(s.status)}</td>
                </tr>
            `).join("");

    } catch (error) {
        console.error(error);
        alert("Unable to load subscriptions.");
    }
}

// Invoices
async function loadInvoices() {

    try {
        const data = await apiRequest("/invoices");

        document.getElementById("invoiceTable").innerHTML =
            data.map(i => `
                <tr>
                    <td>${escapeHTML(i.id)}</td>
                    <td>${escapeHTML(i.customer_id)}</td>
                    <td>${escapeHTML(i.amount)}</td>
                    <td>${escapeHTML(i.status)}</td>
                </tr>
            `).join("");

    } catch (error) {
        console.error(error);
        alert("Unable to load invoices.");
    }
}

// Record payment
document.getElementById("paymentForm").addEventListener(
    "submit",
    async function(event) {

        event.preventDefault();

        const payment = {
            invoice_id: Number(
                document.getElementById("invoiceId").value
            ),

            amount: Number(
                document.getElementById("amount").value
            ),

            method:
                document.getElementById("paymentMethod").value
        };

        if (payment.invoice_id <= 0 || payment.amount <= 0) {
            document.getElementById("paymentMessage").textContent =
                "Enter valid payment details.";
            return;
        }

        try {
            const result = await apiRequest("/payments", {
                method: "POST",
                body: JSON.stringify(payment)
            });

            document.getElementById("paymentMessage").textContent =
                result.message || "Payment recorded successfully.";

            this.reset();

            loadDashboard();
            loadInvoices();

        } catch (error) {
            console.error(error);

            document.getElementById("paymentMessage").textContent =
                error.message || "Unable to record payment.";
        }
    }
);

// Reports
async function loadReports() {

    try {
        const data = await apiRequest("/reports");

        document.getElementById("reportResult").textContent =
            "Total Revenue: " + (data.revenue ?? 0) +
            " | Paid Invoices: " + (data.paidInvoices ?? 0) +
            " | Pending Invoices: " + (data.pendingInvoices ?? 0) +
            " | Overdue Invoices: " + (data.overdueInvoices ?? 0);

    } catch (error) {
        console.error(error);
        alert("Unable to generate reports.");
    }
}

// Prevent HTML injection
function escapeHTML(value) {

    return String(value ?? "").replace(/[&<>"']/g, char => ({
        "&": "&amp;",
        "<": "&lt;",
        ">": "&gt;",
        '"': "&quot;",
        "'": "&#39;"
    })[char]);

}

// Logout
function logout() {

    sessionStorage.removeItem("token");

    window.location.href = "login.html";
}

// Initialize staff dashboard
async function initializeStaff() {

    if (!getToken()) {
        window.location.href = "login.html";
        return;
    }

    try {
        const user = await apiRequest("/me");

        if (user.role !== "STAFF") {
            logout();
            return;
        }

        loadDashboard();

    } catch (error) {
        console.error(error);

        sessionStorage.removeItem("token");
        window.location.href = "login.html";
    }
}

initializeStaff();
