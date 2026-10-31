const API = {
    baseUrl: window.location.origin.startsWith('http') ? window.location.origin : 'http://localhost:8080',

    getToken() {
        return localStorage.getItem('sms_token');
    },

    setToken(token) {
        localStorage.setItem('sms_token', token);
    },

    getUser() {
        const u = localStorage.getItem('sms_user');
        return u ? JSON.parse(u) : null;
    },

    setUser(user) {
        localStorage.setItem('sms_user', JSON.stringify(user));
    },

    clearAuth() {
        localStorage.removeItem('sms_token');
        localStorage.removeItem('sms_user');
    },

    async request(endpoint, options = {}) {
        const headers = {
            'Content-Type': 'application/json',
            ...(options.headers || {})
        };

        const token = this.getToken();
        if (token) {
            headers['Authorization'] = `Bearer ${token}`;
        }

        try {
            const res = await fetch(`${this.baseUrl}${endpoint}`, {
                ...options,
                headers
            });

            const data = await res.json().catch(() => ({}));

            if (!res.ok) {
                const msg = data.error || `Request failed with status ${res.status}`;
                throw new Error(msg);
            }

            return data;
        } catch (err) {
            console.error(`API Error on ${endpoint}:`, err);
            throw err;
        }
    },

    async login(email, password) {
        const data = await this.request('/api/auth/login', {
            method: 'POST',
            body: JSON.stringify({ email, password })
        });
        if (data.token) {
            this.setToken(data.token);
            this.setUser(data.user);
        }
        return data;
    },

    async registerCustomer(name, email, password, confirmPassword) {
        return await this.request('/api/auth/register-customer', {
            method: 'POST',
            body: JSON.stringify({
                name,
                email,
                password,
                confirm_password: confirmPassword
            })
        });
    },

    async getMe() {
        return await this.request('/api/auth/me', { method: 'GET' });
    },

    async logout() {
        try {
            await this.request('/api/auth/logout', { method: 'POST' });
        } catch (e) {
            // ignore network logout errors
        } finally {
            this.clearAuth();
            window.location.href = '/login.html';
        }
    },

    async changePassword(oldPassword, newPassword) {
        return await this.request('/api/auth/change-password', {
            method: 'POST',
            body: JSON.stringify({
                old_password: oldPassword,
                new_password: newPassword
            })
        });
    },

    // Admin APIs
    async adminCreateAdmin(name, email) {
        return await this.request('/api/admin/create-admin', {
            method: 'POST',
            body: JSON.stringify({ name, email })
        });
    },

    async adminCreateStaff(name, email) {
        return await this.request('/api/admin/create-staff', {
            method: 'POST',
            body: JSON.stringify({ name, email })
        });
    },

    async adminGetAdmins() {
        return await this.request('/api/admin/admins', { method: 'GET' });
    },

    async adminGetStaff() {
        return await this.request('/api/admin/staff', { method: 'GET' });
    },

    async adminGetCustomers() {
        return await this.request('/api/admin/customers', { method: 'GET' });
    },

    async adminToggleStatus(userId, status) {
        return await this.request('/api/admin/toggle-status', {
            method: 'POST',
            body: JSON.stringify({ user_id: userId, status })
        });
    },

    // Staff APIs
    async staffGetCustomers() {
        return await this.request('/api/staff/customers', { method: 'GET' });
    },

    // Customer APIs
    async customerGetProfile() {
        return await this.request('/api/customer/profile', { method: 'GET' });
    }
};

// Route protection helper
async function checkAuth(requiredRole) {
    const token = API.getToken();
    if (!token) {
        window.location.href = '/login.html';
        return null;
    }

    try {
        const meData = await API.getMe();
        const user = meData.user;
        API.setUser(user);

        if (requiredRole && user.role !== requiredRole) {
            alert(`Access restricted. Your role is ${user.role}.`);
            // Redirect to appropriate dashboard
            if (user.role === 'ADMIN') window.location.href = '/admin.html';
            else if (user.role === 'STAFF') window.location.href = '/staff.html';
            else if (user.role === 'CUSTOMER') window.location.href = '/customer.html';
            else window.location.href = '/login.html';
            return null;
        }

        // Render user info in header if elements present
        const userNameEl = document.getElementById('headerUserName');
        if (userNameEl) userNameEl.textContent = user.name;

        const userRoleEl = document.getElementById('headerUserRole');
        if (userRoleEl) {
            userRoleEl.textContent = user.role;
            userRoleEl.className = `user-role-badge role-${user.role.toLowerCase()}`;
        }

        return user;
    } catch (err) {
        API.clearAuth();
        window.location.href = '/login.html';
        return null;
    }
}
