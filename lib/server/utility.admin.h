
#pragma once

#include <Arduino.h>
#include <router.h>
#include <string>

namespace ESP32WebServer
{
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
            box-shadow: 0 4px 6px rgba(0,0,0,0.1);
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
    <form action="/admin/login" method="POST">
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
    <style>
        :root {
            --bg-color: #f4f7f6;
            --card-bg: #ffffff;
            --text-main: #333;
            --primary: #3498db;
            --danger: #e74c3c;
            --border-radius: 10px;
        }
        body { font-family: 'Segoe UI', sans-serif; background-color: var(--bg-color); color: var(--text-main); margin: 0; padding: 20px; }
        .container { max-width: 900px; margin: 0 auto; }
        header { display: flex; justify-content: space-between; align-items: center; margin-bottom: 30px; }
        h1 { margin: 0; font-size: 1.8rem; color: #2c3e50; }
        
        .grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(300px, 1fr)); gap: 20px; }
        .card { background: var(--card-bg); padding: 25px; border-radius: var(--border-radius); box-shadow: 0 4px 15px rgba(0,0,0,0.05); }
        .card h2 { margin-top: 0; font-size: 1.1rem; color: #7f8c8d; text-transform: uppercase; letter-spacing: 1px; }
        
        .info-group { margin: 15px 0; }
        .label { display: block; font-size: 0.85rem; color: #95a5a6; }
        .value { font-size: 1.2rem; font-weight: bold; color: var(--text-main); }
        
        .btn { 
            display: inline-block; padding: 10px 20px; margin-top: 15px; 
            background: var(--primary); color: white; text-decoration: none; 
            border-radius: 5px; font-weight: 600; border: none; cursor: pointer;
            transition: opacity 0.2s;
        }
        .btn:hover { opacity: 0.8; }
        .btn-danger { background: var(--danger); }

        .form-group { margin-top: 15px; padding-top: 15px; border-top: 1px solid #eee; }
        input { width: 100%; padding: 8px; margin: 5px 0 10px 0; border: 1px solid #ddd; border-radius: 4px; box-sizing: border-box; }
    </style>
</head>
<body>
    <div class="container">
        <header>
            <h1>Admin Dashboard</h1>
            <a href="/logout" class="btn btn-danger" style="margin-top:0">Logout</a>
        </header>

        <div class="grid">
            <div class="card">
                <h2>Network Connection</h2>
                <div class="info-group">
                    <span class="label">Connected to (SSID)</span>
                    <div class="value" id="ssid-value">Loading...</div>
                </div>
                <div class="info-group">
                    <span class="label">IP Address</span>
                    <div class="value" id="ip-value">0.0.0.0</div>
                </div>
                <a href="/admin/change-wifi" class="btn">Change WiFi</a>
            </div>

            <div class="card">
                <h2>Admin Credentials</h2>
                <form action="/admin/update-auth" method="POST">
                    <div class="info-group">
                        <span class="label">Admin Username</span>
                        <input type="text" name="admin_user" value="admin" required>
                    </div>
                    <div class="info-group">
                        <span class="label">New Password</span>
                        <input type="password" name="admin_pwd" placeholder="Leave empty to keep current">
                    </div>
                    <button type="submit" class="btn">Update Credentials</button>
                </form>
            </div>
            
            <div class="card">
                <h2>System Status</h2>
                <div class="info-group">
                    <span class="label">Device Uptime</span>
                    <div class="value" id="uptime-value">0h 0m 0s</div>
                </div>
                <div class="info-group">
                    <span class="label">Signal Strength</span>
                    <div class="value" id="rssi-value">0 dBm</div>
                </div>
            </div>
        </div>
    </div>

    <script>
        async function loadData() {
            try {
                // Placeholder for real API call to /admin/config
                // const res = await fetch('/admin/config');
                // const data = await res.json();
                
                const data = {
                    ssid: "MyHomeNetwork_5G",
                    ip: "192.168.1.42",
                    uptime: "2d 4h 12m",
                    rssi: "-65"
                };

                document.getElementById('ssid-value').innerText = data.ssid;
                document.getElementById('ip-value').innerText = data.ip;
                document.getElementById('uptime-value').innerText = data.uptime;
                document.getElementById('rssi-value').innerText = data.rssi + " dBm";
            } catch (e) {
                console.error("Load failed", e);
            }
        }
        window.onload = loadData;
    </script>
</body>
</html>
)html";

        res.html(dashboardPage);
    }

    inline void post_AdminLogin(const ESP32WebServer::Request &req, ESP32WebServer::Response &res)
    {
        // TODO TODO TODO: Authentifizierung implementieren
        get_AdminDashboard(req, res);
    }

    class AdminRouter : public ESP32WebServer::Router
    {
    public:
        AdminRouter()
        {
            add("GET", "/admin", get_AdminLogin);
            // add("GET", "/admin/config", get_AdminConfig); TODO
            add("POST", "/admin/login", post_AdminLogin);
        }
    };
}