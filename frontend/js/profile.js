
document.getElementById("profileForm").addEventListener("submit", function(event) {

    event.preventDefault();

    let name = document.getElementById("profileName").value.trim();
    let email = document.getElementById("profileEmail").value.trim();
    let phone = document.getElementById("profilePhone").value.trim();

    let message = document.getElementById("profileMessage");

    if (name === "" || email === "" || phone === "") {
        message.textContent = "Please fill all fields.";
        return;
    }

    message.textContent = "Profile details validated! Backend connection is pending.";

});


document.getElementById("passwordForm").addEventListener("submit", function(event) {

    event.preventDefault();

    let current = document.getElementById("currentPassword").value;
    let newPass = document.getElementById("newPassword").value;
    let confirm = document.getElementById("confirmNewPassword").value;

    let message = document.getElementById("passwordMessage");

    if (current === "" || newPass === "" || confirm === "") {
        message.textContent = "Please fill all fields.";
        return;
    }

    if (newPass.length < 8) {
        message.textContent = "New password must contain at least 8 characters.";
        return;
    }

    if (newPass !== confirm) {
        message.textContent = "New passwords do not match!";
        return;
    }

    message.textContent = "Password form validated! Backend connection is pending.";

});
