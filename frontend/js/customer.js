
<!DOCTYPE html>
<html>
<head>
    <title>Customer Registration</title>
    <link rel="stylesheet" href="css/customer.css">
</head>

<body>

    <div class="container">

        <h1>Create Your Account</h1>
        <p>Register to manage your subscriptions.</p>

        <form id="registerForm">

            <label>Full Name</label>
            <input type="text" id="name" placeholder="Enter your name" required>

            <label>Email Address</label>
            <input type="email" id="email" placeholder="Enter your email" required>

            <label>Password</label>
            <input type="password" id="password" placeholder="Create a password" required>

            <label>Confirm Password</label>
            <input type="password" id="confirmPassword" placeholder="Confirm your password" required>

            <button type="submit">Create Account</button>

        </form>

        <p>Already have an account? <a href="login.html">Login</a></p>

        <p id="message"></p>

    </div>

    <script src="js/customer.js"></script>

</body>
</html>
