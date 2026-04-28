// ESP32 MiniWebServer Framework - https://github.com/LemurDaniel/ESP32__MiniWebServer-Framework
// Copyright © 2026, Daniel Landau
// MIT License

// This is work in progress!

#pragma once

#include <Arduino.h>
#include <stdlib.h>
#include <mbedtls/md.h>

#include <vector>
#include <string>

#include <router.h>
#include <utility.wifi.h>

#define ADMIN_CREDENTIALS_FILE "/admin_credentials.json"

namespace ESP32WebServer
{
    class TokenManager
    {
    public:
        std::string DEFAULT_ADMIN_USER = "admin";
        std::string DEFAULT_ADMIN_PWD = "admin";

        // TODO: Add salt 🧂 for password storage
        std::string DEFAULT_ADMIN_SALT = "5B63F3F0104D1649B8E1A9C9E5F2A1"; // Random salt for password hashing

        static TokenManager &instance()
        {
            static TokenManager _instance;
            return _instance;
        }

        void addToken(const std::string &token)
        {
            const unsigned long expiry = (millis() / 1000) + 3600;
            ADMIN_TOKENS.push_back({token, expiry});
        }

        bool checkToken(const std::string &authToken)
        {
            unsigned long currentTime = millis() / 1000;

            for (auto token = ADMIN_TOKENS.begin(); token != ADMIN_TOKENS.end();)
            {
                if (currentTime > token->second)
                {
                    // Delete expired token
                    token = ADMIN_TOKENS.erase(token);
                }
                else
                {
                    if (token->first == authToken)
                    {
                        return true;
                    }
                    ++token;
                }
            }
            return false;
        }

    private:
        TokenManager() {} // Privater Konstruktor

        // Verhindere Kopien des Singletons (Wichtig!)
        TokenManager(const TokenManager &) = delete;
        void operator=(const TokenManager &) = delete;

        std::vector<std::pair<std::string, unsigned long>> ADMIN_TOKENS;
    };

    inline void get_AdminLogin(const ESP32WebServer::Request &req, ESP32WebServer::Response &res)
    {
        std::string adminPage = R"html(
<!DOCTYPE html>
<html lang="de">

<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Admin Login</title>
    <style>
        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background-color: #f4f7f6;
            display: flex;
            justify-content: center;
            align-items: center;
            height: 100vh;
            margin: 0;
        }

        .login-container {
            background: white;
            padding: 2rem;
            border-radius: 8px;
            box-shadow: 0 4px 6px rgba(0, 0, 0, 0.1);
            width: 100%;
            max-width: 400px;
        }

        h2 {
            text-align: center;
            color: #333;
            margin-bottom: 1.5rem;
        }

        .form-group {
            margin-bottom: 1rem;
        }

        label {
            display: block;
            margin-bottom: 0.5rem;
            color: #666;
        }

        input {
            width: 100%;
            padding: 0.75rem;
            border: 1px solid #ddd;
            border-radius: 4px;
            box-sizing: border-box;
        }

        button {
            width: 100%;
            padding: 0.75rem;
            background-color: #007bff;
            border: none;
            border-radius: 4px;
            color: white;
            font-size: 1rem;
            cursor: pointer;
            transition: background 0.3s;
        }

        button:hover {
            background-color: #0056b3;
        }
    </style>
</head>

<body>

    <div class="login-container">
        <h2>Admin Bereich</h2>
        <form id="form-login">
            <div class="form-group">
                <label for="username">Benutzername</label>
                <input type="text" id="username" name="username" required>
            </div>
            <div class="form-group">
                <label for="password">Passwort</label>
                <input type="password" id="password" name="password" required>
            </div>
            <button type="submit">Einloggen</button>
        </form>
    </div>

    <script>
        // Override form submission to send JSON data
        window.onload = function () {
            document.getElementById("form-login").onsubmit = function () {
                postLogin();
                return false;
            };

            fetch('/admin/logged_in', {
                method: 'GET'
            }).then(res => {
                if (res.ok) {
                    window.location.replace('/admin/dashboard');
                }
            });
        };

        async function postLogin() {
            const form = document.getElementById('form-login');

            try {
                const res = await fetch('admin/login', {
                    method: 'POST',
                    headers: {
                        'Content-Type': 'application/json'
                    },
                    body: JSON.stringify({
                        username: form.username.value,
                        password: form.password.value
                    })
                });

                if(!res.ok) {
                    alert('Login fehlgeschlagen: ' + res.statusText);
                    return;
                }

                const json = await res.json();
                const token = json.token;

                document.cookie = `adminToken=${token}; path=/; max-age=3600`; // Cookie valid for 1 hour

                window.location.replace('/admin/dashboard')
            }
            catch (error) {
                console.error('Fehler beim Login:', error);
            }

        }

    </script>

</body>
</html>
)html";

        res.html(adminPage);
    }

    inline void get_AdminDashboard(const ESP32WebServer::Request &req, ESP32WebServer::Response &res)
    {
        std::string dashboardPage = R"html(
<!DOCTYPE html>
<html lang="en">

<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32 Admin Panel</title>

    <link rel="stylesheet" href="https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.4.0/css/all.min.css">

    <style>
        :root {
            --bg-color: #f0f2f5;
            --card-bg: #ffffff;
            --text-main: #2c3e50;
            --primary: #3498db;
            --danger: #e74c3c;
            --border-radius: 12px;
        }

        body {
            font-family: 'Segoe UI', Tahoma, sans-serif;
            background-color: var(--bg-color);
            color: var(--text-main);
            margin: 0;
            padding: 20px;
        }

        .container {
            max-width: 1000px;
            margin: 0 auto;
        }

        header {
            display: flex;
            justify-content: space-between;
            align-items: baseline;
            margin-bottom: 30px;
            padding-bottom: 15px;
            border-bottom: 2px solid #ddd;
        }

        .header-left {
            display: flex;
            align-items: center;
            gap: 15px;
        }

        .grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(320px, 1fr));
            gap: 25px;
        }

        .card {
            background: var(--card-bg);
            padding: 25px;
            border-radius: var(--border-radius);
            box-shadow: 0 4px 20px rgba(0, 0, 0, 0.08);
            position: relative;
        }

        .card h2 {
            margin-top: 0;
            font-size: 1rem;
            color: #95a5a6;
            text-transform: uppercase;
            letter-spacing: 1.2px;
            display: flex;
            align-items: center;
            gap: 10px;
        }

        .card h2 i {
            color: var(--primary);
            font-size: 1.2rem;
        }

        .info-group {
            margin: 20px 0;
        }

        .label {
            display: block;
            font-size: 0.8rem;
            color: #bdc3c7;
            text-transform: uppercase;
            font-weight: bold;
        }

        .value {
            font-size: 1.3rem;
            font-weight: 600;
            color: var(--text-main);
            display: flex;
            align-items: center;
            gap: 10px;
        }

        .btn {
            display: inline-flex;
            align-items: center;
            gap: 8px;
            padding: 12px 20px;
            background: var(--primary);
            color: white;
            text-decoration: none;
            border-radius: 6px;
            font-weight: 600;
            border: none;
            cursor: pointer;
            transition: transform 0.1s, background 0.2s;
        }

        .btn:hover {
            background: #2980b9;
            transform: translateY(-1px);
        }

        .btn-danger {
            background: var(--danger);
        }

        .btn-danger:hover {
            background: #c0392b;
        }

        .form-group {
            margin-top: 15px;
        }

        input {
            width: 100%;
            padding: 10px;
            margin: 8px 0 15px 0;
            border: 1px solid #dcdde1;
            border-radius: 6px;
            box-sizing: border-box;
            background: #f9f9f9;
        }

        input:focus {
            outline: none;
            border-color: var(--primary);
            background: #fff;
        }

        select {
            width: 100%;
            padding: 10px;
            margin: 8px 0 15px 0;
            border: 1px solid #dcdde1;
            border-radius: 6px;
            box-sizing: border-box;
            background: #f9f9f9;
            font-size: 1rem;
            color: var(--text-main);
            cursor: pointer;
        }

        select:focus {
            outline: none;
            border-color: var(--primary);
            background: #fff;
        }

        #wifi-form {
            margin-top: 20px;
            border-top: 1px solid #eee;
            padding-top: 15px;
            display: none;
        }

        .network-list {
            margin-top: 15px;
            display: flex;
            flex-direction: column;
            gap: 10px;
        }

        .network-item {
            display: flex;
            align-items: center;
            gap: 10px;
            padding: 10px 12px;
            background: #f4f6f8;
            border-radius: 8px;
            border: 1px solid #e0e0e0;
        }

        .network-item .net-info {
            flex: 1;
            min-width: 0;
        }

        .network-item .net-ssid {
            font-weight: 600;
            font-size: 0.95rem;
            white-space: nowrap;
            overflow: hidden;
            text-overflow: ellipsis;
        }

        .network-item .net-meta {
            font-size: 0.78rem;
            color: #95a5a6;
            display: flex;
            gap: 12px;
            margin-top: 2px;
            align-items: center;
        }

        .network-item .net-pwd {
            font-family: monospace;
            letter-spacing: 2px;
            font-size: 0.85rem;
        }

        .network-item .btn-icon {
            background: none;
            border: none;
            cursor: pointer;
            color: #bdc3c7;
            font-size: 1rem;
            padding: 4px 6px;
            border-radius: 4px;
            transition: color 0.2s;
            flex-shrink: 0;
        }

        .network-item .btn-icon:hover {
            color: var(--danger);
        }

        .network-item .btn-icon.toggle-pwd:hover {
            color: var(--primary);
        }

        .add-network-form {
            margin-top: 15px;
            border-top: 1px solid #eee;
            padding-top: 15px;
            display: none;
        }

        .btn-row {
            display: flex;
            gap: 10px;
            margin-top: 5px;
        }

        .btn-sm {
            padding: 8px 14px;
            font-size: 0.85rem;
        }

        .btn-secondary {
            background: #7f8c8d;
        }

        .btn-secondary:hover {
            background: #636e72;
        }

        .btn-success {
            background: #27ae60;
        }

        .btn-success:hover {
            background: #219a52;
        }

        .modal-overlay {
            display: none;
            position: fixed;
            inset: 0;
            background: rgba(0, 0, 0, 0.45);
            z-index: 1000;
            align-items: center;
            justify-content: center;
        }

        .modal-overlay.visible {
            display: flex;
        }

        .modal {
            background: var(--card-bg);
            border-radius: var(--border-radius);
            padding: 35px 30px 25px;
            max-width: 420px;
            width: 90%;
            box-shadow: 0 12px 40px rgba(0, 0, 0, 0.2);
            text-align: center;
            animation: pop-in 0.2s ease;
        }

        @keyframes pop-in {
            from {
                transform: scale(0.85);
                opacity: 0;
            }

            to {
                transform: scale(1);
                opacity: 1;
            }
        }

        .modal-icon {
            font-size: 2.8rem;
            color: var(--primary);
            margin-bottom: 15px;
        }

        .modal h3 {
            margin: 0 0 10px;
            font-size: 1.25rem;
            color: var(--text-main);
        }

        .modal p {
            color: #7f8c8d;
            font-size: 0.95rem;
            margin: 0 0 25px;
        }

        .modal .btn-row {
            justify-content: center;
        }
    </style>
</head>

<body>
    <div class="container">
        <header>
            <div class="header-left">
                <h1>Admin Dashboard</h1>
            </div>
            <div style="display:flex; gap:10px;">
                <button onclick="confirmRestart()" class="btn btn-success"><i class="fa-solid fa-power-off"></i> Restart
                    Device</button>
                <a href="/logout" class="btn btn-danger"><i class="fa-solid fa-right-from-bracket"></i> Logout</a>
            </div>
        </header>

        <div class="grid">
            <div class="card">
                <h2><i class="fa-solid fa-wifi"></i> Active Connection</h2>
                <div class="info-group">
                    <span class="label">SSID</span>
                    <div class="value" id="ssid-value">---</div>
                </div>
                <div class="info-group">
                    <span class="label">Password</span>
                    <div class="value" id="password-value">---</div>
                </div>
                <div class="info-group">
                    <span class="label">Signal Strength</span>
                    <div class="value"><i class="fa-solid fa-signal"></i> <span id="rssi-value">0</span></div>
                </div>
                <div class="info-group">
                    <span class="label">IP Address</span>
                    <div class="value" style="font-family: monospace; font-size: 1.1rem;" id="ip-value">0.0.0.0</div>
                </div>
            </div>

            <div class="card">
                <h2><i class="fa-solid fa-list"></i> Saved Networks</h2>
                <div class="network-list" id="network-list">
                    <div style="color:#bdc3c7; font-size:0.9rem;">Loading...</div>
                </div>
                <div class="add-network-form" id="add-network-form">
                    <div class="form-group">
                        <label class="label">Network</label>
                        <select id="wifi-ssid">
                            <option value="">-- Scanning... --</option>
                        </select>
                    </div>
                    <div class="form-group">
                        <label class="label">Password</label>
                        <input type="password" id="wifi-password" placeholder="WiFi Password">
                    </div>
                    <div class="btn-row">
                        <button onclick="addNetwork()" class="btn btn-sm"><i class="fa-solid fa-floppy-disk"></i>
                            Save</button>
                        <button onclick="scanWiFi()" class="btn btn-sm btn-secondary"><i class="fa-solid fa-rotate"></i>
                            Rescan</button>
                        <button onclick="toggleAddForm(false)" class="btn btn-sm btn-secondary"><i
                                class="fa-solid fa-xmark"></i> Cancel</button>
                    </div>
                </div>
                <div style="margin-top:15px;">
                    <button onclick="toggleAddForm(true)" class="btn btn-sm" id="btn-add-network">
                        <i class="fa-solid fa-plus"></i> Add Network
                    </button>
                </div>
            </div>

            <div class="card">
                <h2><i class="fa-solid fa-user-shield"></i> Security</h2>
                <form action="/admin/update-auth" method="POST">
                    <div class="form-group">
                        <label class="label">Admin Username</label>
                        <input type="text" name="admin_user" placeholder="Username" required>
                    </div>
                    <div class="form-group">
                        <label class="label">New Password</label>
                        <input type="password" name="admin_pwd" placeholder="Leave empty to keep current">
                    </div>
                    <button type="submit" class="btn"><i class="fa-solid fa-floppy-disk"></i> Update Auth</button>
                </form>
            </div>
        </div>
    </div>

    <!-- Network saved popup -->
    <div class="modal-overlay" id="modal-wifi-saved">
        <div class="modal">
            <div class="modal-icon"><i class="fa-solid fa-circle-check"></i></div>
            <h3>Network saved</h3>
            <p>The network was added. Changes take effect after a restart.</p>
            <div class="btn-row">
                <button onclick="closeModal('modal-wifi-saved')" class="btn btn-secondary btn-sm">Later</button>
                <button onclick="doRestart()" class="btn btn-sm"><i class="fa-solid fa-power-off"></i> Restart
                    Now</button>
            </div>
        </div>
    </div>

    <!-- Restart confirm popup -->
    <div class="modal-overlay" id="modal-restart">
        <div class="modal">
            <div class="modal-icon" style="color:var(--danger)"><i class="fa-solid fa-triangle-exclamation"></i></div>
            <h3>Restart device?</h3>
            <p>The ESP32 will be restarted. The connection will be interrupted.</p>
            <div class="btn-row">
                <button onclick="closeModal('modal-restart')" class="btn btn-secondary btn-sm">Cancel</button>
                <button onclick="doRestart()" class="btn btn-danger btn-sm"><i class="fa-solid fa-power-off"></i>
                    Restart</button>
            </div>
        </div>
    </div>

    <script>
        function openModal(id) {
            document.getElementById(id).classList.add('visible');
        }

        function closeModal(id) {
            document.getElementById(id).classList.remove('visible');
        }

        function confirmRestart() {
            openModal('modal-restart');
        }

        function escapeHtml(str) {
            return str.replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;').replace(/"/g, '&quot;');
        }

        function toggleAddForm(open) {
            const form = document.getElementById('add-network-form');
            const btn = document.getElementById('btn-add-network');
            form.style.display = open ? 'block' : 'none';
            btn.style.display = open ? 'none' : 'inline-flex';
            if (open) scanWiFi();
        }

        async function doRestart() {
            closeModal('modal-wifi-saved');
            closeModal('modal-restart');
            await fetch('/admin/restart', { method: 'POST' }).catch(() => { });
        }

        async function loadWiFiConfig() {
            try {
                const res = await fetch('/admin/wifi/active');
                const json = await res.json();
                document.getElementById('ssid-value').innerText = json.SSID || "N/A";
                document.getElementById('password-value').innerText = json.Password || "Not Set";
                document.getElementById('ip-value').innerText = json.IPAddress || "";
                document.getElementById('rssi-value').innerText = json.SignalStrength || "0 dBm";
            } catch (e) {
                console.error("Fetch error", e);
            }
        }

        var scanWiFiRunning = false;
        async function scanWiFi() {
            if (scanWiFiRunning) return;
            scanWiFiRunning = true;

            const select = document.getElementById('wifi-ssid');
            select.innerHTML = '<option value="">-- Scanning... --</option>';
            try {
                const res = await fetch('/admin/wifi/scan');
                const json = await res.json();
                select.innerHTML = json.networks
                    .map(e => `<option value="${escapeHtml(e.SSID)}">(${escapeHtml(String(e.SignalStrength))}) ${escapeHtml(e.SSID)}</option>`)
                    .join('');
            } catch (e) {
                console.error("Scan error", e);
                select.innerHTML = '<option value="">-- Scan failed --</option>';
            } finally {
                scanWiFiRunning = false;
            }
        }

        async function addNetwork() {
            const ssid = document.getElementById('wifi-ssid').value;
            const password = document.getElementById('wifi-password').value;
            if (!ssid) return;

            await fetch('/admin/wifi/network', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ ssid, password })
            });

            document.getElementById('wifi-password').value = '';
            toggleAddForm(false);
            loadNetworks();
            openModal('modal-wifi-saved');
        }

        async function removeNetwork(ssid) {
            if (!confirm(`Remove "${ssid}" from saved networks?`)) return;
            await fetch('/admin/wifi/network', {
                method: 'DELETE',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ ssid })
            }).catch(() => { });
            loadNetworks();
        }

        function renderNetworkList(networks) {
            const list = document.getElementById('network-list');
            if (!networks || networks.length === 0) {
                list.innerHTML = '<div style="color:#bdc3c7; font-size:0.9rem;">No saved networks.</div>';
                return;
            }
            list.innerHTML = networks.map((net, i) => `
                <div class="network-item" id="net-item-${i}">
                    <div class="net-info">
                        <div class="net-ssid">${escapeHtml(net.SSID)}</div>
                    </div>
                    <button class="btn-icon" title="Remove network"
                        onclick="removeNetwork('${net.SSID}')">
                        <i class="fa-solid fa-trash"></i>
                    </button>
                </div>
            `).join('');
        }

        async function loadNetworks() {
            try {
                const res = await fetch('/admin/wifi/networks');
                const json = await res.json();
                renderNetworkList(json.networks || []);
            } catch (e) {
                console.error("Networks fetch error", e);
                document.getElementById('network-list').innerHTML =
                    '<div style="color:#e74c3c; font-size:0.9rem;">Failed to load networks.</div>';
            }
        }

        window.onload = () => { loadWiFiConfig(); loadNetworks(); };
    </script>
</body>

</html>
)html";

        res.html(dashboardPage);
    }
    /**
    ******************************************************************************
    ******************************************************************************
    * Handle Login logic for admin panel
    *
    */
    inline void setDefaultAdminCredentials(std::string username, std::string password)
    {
        TokenManager::instance().DEFAULT_ADMIN_USER = username;
        TokenManager::instance().DEFAULT_ADMIN_PWD = password;
    }

    inline void setDefaultAdminSalt(std::string salt)
    {
        TokenManager::instance().DEFAULT_ADMIN_SALT = salt;
    }

    inline std::string randomString()
    {
        std::string charSet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
        std::string result;
        for (int i = 0; i < 16; i++)
        {
            int index = random(0, charSet.size());
            result += charSet[index];
        }
        return result;
    }

    inline std::string generateSHA256(const std::string &text)
    {
        std::vector<uint8_t> hash(32);

        mbedtls_md_context_t ctx;
        mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;

        mbedtls_md_init(&ctx);
        mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(md_type), 0);
        mbedtls_md_starts(&ctx);
        mbedtls_md_update(&ctx, (const unsigned char *)text.c_str(), text.length());
        mbedtls_md_finish(&ctx, hash.data());
        mbedtls_md_free(&ctx);

        std::string hashStr;
        for (uint8_t byte : hash)
        {
            char buf[3];
            snprintf(buf, sizeof(buf), "%02x", byte);
            hashStr += buf;
        }

        return hashStr;
    }

    /**
    ******************************************************************************
    ******************************************************************************
    * Handle Login route for admin panel
    *
    */

    inline bool isTokenValid(const ESP32WebServer::Request &req, ESP32WebServer::Response &res)
    {
        Serial.println("Checking authentication for admin route");

        if (req.cookies.find("adminToken") == req.cookies.end())
        {
            Serial.println("No admin token found in cookies");
            return false;
        }

        const std::string authToken = req.cookies.at("adminToken");

        if (!TokenManager::instance().checkToken(authToken))
        {
            Serial.println("Invalid or expired admin token");
            return false;
        }

        return true;
    }

    inline void verify_AdminAuth(const ESP32WebServer::Request &req, ESP32WebServer::Response &res)
    {
        if (!isTokenValid(req, res))
        {
            res.status(401).text("Unauthorized: Invalid or expired token");
        }
        else
        {
            res.status(200).text("Authenticated");
        }
    }

    inline void is_Authenticated(const ESP32WebServer::Request &req, ESP32WebServer::Response &res)
    {
        if (!isTokenValid(req, res))
        {
            res.header("Location", "/admin").status(302).text("Unauthorized: No token provided").finalize();
        }
    }

    inline void post_AdminLogin(const ESP32WebServer::Request &req, ESP32WebServer::Response &res)
    {

        if (req.body.isNull())
        {
            Serial.println("Invalid JSON in login request");
            res.status(400).text("Invalid JSON");
            return;
        }

        if (!req.body["username"].is<std::string>() && !req.body["password"].is<std::string>())
        {
            Serial.println("Missing username or password in login request");
            res.status(400).text("Invalid username or password");
            return;
        }

        std::string admin_user = "admin";
        std::string admin_pwd = generateSHA256("admin");

        std::string username = req.body["username"].as<std::string>();
        std::string password = generateSHA256(req.body["password"].as<std::string>());

        // Read stored credentials from file
        if (fileExists(ADMIN_CREDENTIALS_FILE))
        {
            Serial.println("Reading admin credentials from file");
            const JsonDocument doc = readJsonFile(ADMIN_CREDENTIALS_FILE);
            if (doc["admin_user"].is<std::string>() && doc["admin_pwd"].is<std::string>())
            {
                admin_user = doc["admin_user"].as<std::string>();
                admin_pwd = doc["admin_pwd"].as<std::string>();
            }
        }
        else
        {
            Serial.println("Admin credentials file not found, using default credentials");
        }

        Serial.printf("Login attempt with username: %s\n", username.c_str());
        Serial.printf("Expected username: %s\n", admin_user.c_str());
        Serial.printf("Received password hash: %s\n", password.c_str());
        Serial.printf("Expected password hash: %s\n", admin_pwd.c_str());
        if (username != admin_user || password != admin_pwd)
        {
            Serial.println("Invalid username or password");
            res.status(401).text("Invalid username or password");
            return;
        }

        const std::string token = generateSHA256(username + randomString() + String(millis()).c_str());
        const unsigned long tokenExpiresAt = millis() + 3600000; // Token valid for 1 hour

        TokenManager::instance().addToken(token);

        JsonDocument doc;
        doc["token"] = token;

        res.json(doc);
    }

    /**
     ******************************************************************************
     ******************************************************************************
     * Handle Admin Credentials update route for admin panel
     *
     */

    inline void post_AdminUpdateAuth(Request const &req, Response &res)
    {
        if (req.body.isNull())
        {
            res.status(400).text("Invalid JSON");
            return;
        }

        if (!req.body["admin_user"].is<std::string>() || !req.body["admin_pwd"].is<std::string>())
        {
            res.status(400).text("Missing admin_user or admin_pwd");
            return;
        }

        // TODO: Implement admin credential update logic
        std::string newAdminUser = req.body["admin_user"].as<std::string>();
        std::string newAdminPwd = req.body["admin_pwd"].as<std::string>();

        const std::string hashedPwd = generateSHA256(newAdminPwd);

        res.OK().text("Admin credentials updated");
    }

    inline void post_AdminRestart(Request const &req, Response &res)
    {
        res.OK().text("Restarting...");
        delay(1000);
        ESP.restart();
    }

    /**
     ******************************************************************************
     ******************************************************************************
     * Handle WiFi config routes for admin panel
     *
     */
    inline void get_WiFiActive(Request const &req, Response &res)
    {
        Serial.println("Fetching current WiFi configuration for admin dashboard");
        ESP32WebServer::WiFiConfig wifiConfig = WiFiUtility::instance().getActiveWiFi();

        JsonDocument doc;
        doc["SSID"] = wifiConfig.ssid;
        doc["SignalStrength"] = wifiConfig.signalStrength;
        doc["IPAddress"] = wifiConfig.ipAddress;
        doc["Password"] = wifiConfig.password;

        res.OK().json(doc);
    }

    inline void get_WiFiScan(Request const &req, Response &res)
    {
        std::vector<ESP32WebServer::WiFiConfig> options = WiFiUtility::instance().scanNetworks();

        JsonDocument doc;
        JsonArray arr = doc["networks"].to<JsonArray>();
        for (const WiFiConfig &opt : options)
        {
            JsonObject obj = arr.add<JsonObject>();
            obj["SSID"] = opt.ssid;
            obj["SignalStrength"] = opt.signalStrength;
            obj["IPAddress"] = opt.ipAddress;
        }

        res.OK().json(doc);
    }

    inline void get_WiFiSavedNetworks(Request const &req, Response &res)
    {
        std::vector<ESP32WebServer::WiFiConfig> options = WiFiUtility::instance().getSavedNetworks();

        JsonDocument doc;
        JsonArray arr = doc["networks"].to<JsonArray>();
        for (const WiFiConfig &opt : options)
        {
            JsonObject obj = arr.add<JsonObject>();
            obj["SSID"] = opt.ssid;
            obj["SignalStrength"] = opt.signalStrength;
            obj["IPAddress"] = opt.ipAddress;
        }

        res.OK().json(doc);
    }

    inline void delete_WiFiSavedNetwork(Request const &req, Response &res)
    {
        if (req.body.isNull())
        {
            res.status(400).text("Invalid JSON");
            return;
        }

        if (req.body.isNull() || !req.body["ssid"].is<std::string>())
        {
            res.status(400).text("Missing ssid");
            return;
        }

        std::string ssid = req.body["ssid"].as<std::string>();
        WiFiUtility::instance().removeWiFiConfig(ssid);
        res.OK().text("Network removed");
    }

    inline void post_WiFiSavedNetwork(Request const &req, Response &res)
    {
        if (req.body.isNull())
        {
            res.status(400).text("Invalid JSON");
            return;
        }

        if (!req.body["ssid"].is<std::string>() || !req.body["password"].is<std::string>())
        {
            res.status(400).text("Missing ssid or password");
            return;
        }

        std::string ssid = req.body["ssid"].as<std::string>();
        std::string password = req.body["password"].as<std::string>();

        WiFiUtility::instance().addWiFiConfig(ssid, password);

        res.OK().text("WiFi config updated");
    }

    class AdminRouter : public ESP32WebServer::Router
    {
    public:
        AdminRouter()
        {
            // Perform login and return token
            route("POST", "/admin/login", post_AdminLogin);
            route("GET", "/admin/logged_in", verify_AdminAuth);

            // Returns the html sites
            route("GET", "/admin", get_AdminLogin);
            route("GET", "/admin/dashboard", {is_Authenticated, get_AdminDashboard});

            // Return 401 if the token is not valid or missing for any /admin/* route
            route("POST", "/admin/auth", {is_Authenticated, post_AdminUpdateAuth});

            // Wifi config routes for admin dashboard
            route("POST", "/admin/restart", {is_Authenticated, post_AdminRestart});

            // Get al WiFi Networks in reach of the device
            route("GET", "/admin/wifi/scan", {is_Authenticated, get_WiFiScan});
            route("GET", "/admin/wifi/active", {is_Authenticated, get_WiFiActive});

            // Get all WiFi networks to possibly connect to
            route("GET", "/admin/wifi/networks", {is_Authenticated, get_WiFiSavedNetworks});

            // Add or Remove a WiFi-Config
            route("POST", "/admin/wifi/network", {is_Authenticated, post_WiFiSavedNetwork});
            route("DELETE", "/admin/wifi/network", {is_Authenticated, delete_WiFiSavedNetwork});
        }
    };
}