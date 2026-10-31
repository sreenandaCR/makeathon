
const API = "http://localhost:8080/api/staff";

function getToken() {
    return sessionStorage.getItem("token");
}

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
        throw new Error("Request failed: " + response.status);
    }

    return response.json();
}

function showSection(sectionId) {
    document.querySelectorAll(".section").forEach(section => {
        section.classList.add("hidden");
    });

    document.getElementById(sectionId).classList.remove("hidden");

    if (sectionId === "customers") loadCustomers();
    if (sectionId === "subscriptions") loadSubscriptions();
    if (sectionId === "invoices") loadInvoices();
    if (sectionId === "dashboard") loadDashboard();
}

async function loadDashboard() {
    try {
        const data = await apiRequest("/dashboard");

        document.getElementById("customerCount").textContent =
            data.customers;

        document.getElementById("subscriptionCount").textContent =
            data.subscriptions;

        document.getElementById("invoiceCount").textContent =
            data.invoices;

        document.getElementById("paymentCount").textContent =
            data.payments;
    } catch (error) {
        console.error(error);
    }
}

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
        alert("Unable to load customers.");
    }
}

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
        alert("Unable to load subscriptions.");
    }
}

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
        alert("Unable to load invoices.");
    }
}

document.getElementById("paymentForm").addEventListener(
    "submit", async function(event) {
        event.preventDefault();

        const payment = {
            invoice_id: Number(document.getElementById("invoiceId").value),
            amount: Number(document.getElementById("amount").value),
            method: document.getElementById("paymentMethod").value
        };

        try {
            await apiRequest("/payments", {
                method: "POST",
                body: JSON.stringify(payment)
            });

            document.getElementById("paymentMessage").textContent =
                "Payment recorded successfully.";

            this.reset();
            loadDashboard();
        } catch (error) {
            document.getElementById("paymentMessage").textContent =
                "Unable to record payment.";
        }
    }
);

async function loadReports() {
    try {
        const data = await apiRequest("/reports");

        document.getElementById("reportResult").textContent =
            "Total Revenue: " + data.revenue +
            " | Pending Invoices: " + data.pending_invoices;
    } catch (error) {
        alert("Unable to generate reports.");
    }
}

function escapeHTML(value) {
    return String(value ?? "").replace(/[&<>"']/g, char => ({
        "&": "&amp;",
        "<": "&lt;",
        ">": "&gt;",
        '"': "&quot;",
        "'": "&#39;"
    })[char]);
}

function logout() {
    sessionStorage.removeItem("token");
    window.location.href = "../login.html";
}

// Check staff authentication
async function initializeStaff() {
    if (!getToken()) {
        window.location.href = "../login.html";
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
        logout();
    }
}

initializeStaff();
