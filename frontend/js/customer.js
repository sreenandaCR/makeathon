
document.getElementById("registerForm").addEventListener("submit", function(event) {

    event.preventDefault();

    let name = document.getElementById("name").value.trim();
    let email = document.getElementById("email").value.trim();
    let phone = document.getElementById("phone").value.trim();
    let password = document.getElementById("password").value;
    let confirmPassword = document.getElementById("confirmPassword").value;

    let message = document.getElementById("message");

    if (name === "" || email === "" || phone === "" ||
        password === "" || confirmPassword === "") {

        message.textContent = "Please fill all fields.";
        return;
    }

    if (password.length < 8) {
        message.textContent = "Password must contain at least 8 characters.";
        return;
    }

    if (password !== confirmPassword) {
        message.textContent = "Passwords do not match!";
        return;
    }

    message.textContent = "Form validated! Backend connection is not set up yet.";

});
