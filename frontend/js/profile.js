
const API = "http://localhost:18080";
const customerId = localStorage.getItem("customerId");

const nameInput = document.getElementById("name");
const emailInput = document.getElementById("email");
const phoneInput = document.getElementById("phone");
const form = document.getElementById("profileForm");
const message = document.getElementById("message");

async function loadProfile() {
    if (!customerId) {
        window.location.href = "login.html";
        return;
    }

    try {
        const response = await fetch(
            `${API}/profile?id=${encodeURIComponent(customerId)}`
        );

        if (!response.ok) throw new Error("Unable to load profile");

        const details = (await response.text()).split("\t");

        nameInput.value = details[1] || "";
        emailInput.value = details[2] || "";
        phoneInput.value = details[3] || "";

    } catch (error) {
        message.textContent = "Unable to load profile.";
    }
}

form.addEventListener("submit", async function(event) {
    event.preventDefault();

    try {
        const response = await fetch(`${API}/profile/update`, {
            method: "POST",
            headers: {
                "Content-Type": "application/x-www-form-urlencoded"
            },
            body: new URLSearchParams({
                id: customerId,
                name: nameInput.value.trim(),
                email: emailInput.value.trim(),
                phone: phoneInput.value.trim()
            })
        });

        message.textContent = await response.text();

    } catch (error) {
        message.textContent = "Cannot connect to server.";
    }
});

loadProfile();
