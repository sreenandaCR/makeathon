
const API = "http://localhost:18080";

function encode(data) {
    return new URLSearchParams(data).toString();
}

// CUSTOMER REGISTRATION
const registerForm = document.getElementById("registerForm");

if (registerForm) {
    registerForm.addEventListener("submit", async function(event) {
        event.preventDefault();

        const message = document.getElementById("message");
        const password = document.getElementById("password").value;
        const confirmPassword =
            document.getElementById("confirmPassword").value;

        if (password.length < 8) {
            message.textContent = "Password must be at least 8 characters.";
            return;
        }

        if (password !== confirmPassword) {
            message.textContent = "Passwords do not match!";
            return;
        }

        try {
            const response = await fetch(API + "/register", {
                method: "POST",
                headers: {
                    "Content-Type": "application/x-www-form-urlencoded"
                },
                body: encode({
                    name: document.getElementById("name").value.trim(),
                    email: document.getElementById("email").value.trim(),
                    phone: document.getElementById("phone").value.trim(),
                    password: password
                })
            });

            message.textContent = await response.text();

            if (response.ok) {
                registerForm.reset();
            }
        } catch (error) {
            message.textContent = "Cannot connect to server!";
        }
    });
}

// CUSTOMER LOGIN
const loginForm = document.getElementById("loginForm");

if (loginForm) {
    loginForm.addEventListener("submit", async function(event) {
        event.preventDefault();

        const message = document.getElementById("message");

        try {
            const response = await fetch(API + "/login", {
                method: "POST",
                headers: {
                    "Content-Type": "application/x-www-form-urlencoded"
                },
                body: encode({
                    email: document.getElementById("email").value.trim(),
                    password: document.getElementById("password").value
                })
            });

            const result = await response.text();

            if (response.ok) {
                localStorage.setItem("customerId", result.trim());
                window.location.href = "customer-dashboard.html";
            } else {
                message.textContent = result;
            }
        } catch (error) {
            message.textContent = "Cannot connect to server!";
        }
    });
}
