// ESP32 MiniWebServer Framework - https://github.com/LemurDaniel/ESP32__MiniWebServer-Framework
// Copyright © 2026, Daniel Landau
// MIT License

// This is work in progress!

#pragma once

#include <Arduino.h>
#include <router.h>
#include <string>

#include <utility.wifi.h>

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

    // <link rel="stylesheet" href="https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.4.0/css/all.min.css">
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
            --bg-color: #f0f2f5;
            --card-bg: #ffffff;
            --text-main: #2c3e50;
            --primary: #3498db;
            --danger: #e74c3c;
            --border-radius: 12px;
        }
        body { font-family: 'Segoe UI', Tahoma, sans-serif; background-color: var(--bg-color); color: var(--text-main); margin: 0; padding: 20px; }
        .container { max-width: 1000px; margin: 0 auto; }
        
        header { 
            display: flex; justify-content: space-between; align-items: baseline; 
            margin-bottom: 30px; padding-bottom: 15px; border-bottom: 2px solid #ddd;
        }
        .header-left { display: flex; align-items: center; gap: 15px; }
        .uptime-display { font-size: 0.9rem; color: #7f8c8d; font-weight: 500; }
        .uptime-display i { color: var(--primary); margin-right: 5px; }
        
        .grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(320px, 1fr)); gap: 25px; }
        .card { background: var(--card-bg); padding: 25px; border-radius: var(--border-radius); box-shadow: 0 4px 20px rgba(0,0,0,0.08); position: relative; }
        .card h2 { margin-top: 0; font-size: 1rem; color: #95a5a6; text-transform: uppercase; letter-spacing: 1.2px; display: flex; align-items: center; gap: 10px; }
        .card h2 i { color: var(--primary); font-size: 1.2rem; }
        
        .info-group { margin: 20px 0; }
        .label { display: block; font-size: 0.8rem; color: #bdc3c7; text-transform: uppercase; font-weight: bold; }
        .value { font-size: 1.3rem; font-weight: 600; color: var(--text-main); display: flex; align-items: center; gap: 10px; }
        
        .btn { 
            display: inline-flex; align-items: center; gap: 8px; padding: 12px 20px; 
            background: var(--primary); color: white; text-decoration: none; 
            border-radius: 6px; font-weight: 600; border: none; cursor: pointer; transition: transform 0.1s, background 0.2s;
        }
        .btn:hover { background: #2980b9; transform: translateY(-1px); }
        .btn-danger { background: var(--danger); }
        .btn-danger:hover { background: #c0392b; }

        .form-group { margin-top: 15px; }
        input { 
            width: 100%; padding: 10px; margin: 8px 0 15px 0; 
            border: 1px solid #dcdde1; border-radius: 6px; box-sizing: border-box; background: #f9f9f9;
        }
        input:focus { outline: none; border-color: var(--primary); background: #fff; }
    </style>
</head>
<body>
    <div class="container">
        <header>
            <div class="header-left">
                <h1>Admin Dashboard</h1>
                <div class="uptime-display">
                    <i class="fa-solid fa-clock"></i>Up: <span id="uptime-value">0h 0m 0s</span>
                </div>
            </div>
            <a href="/logout" class="btn btn-danger"><i class="fa-solid fa-right-from-bracket"></i> Logout</a>
        </header>

        <div class="grid">
            <div class="card">
                <h2><i class="fa-solid fa-wifi"></i> Network Connection</h2>
                <div class="info-group">
                    <span class="label">SSID</span>
                    <div class="value" id="ssid-value">---</div>
                </div>
                <div class="info-group">
                    <span class="label">Signal Strength</span>
                    <div class="value"><i class="fa-solid fa-signal"></i> <span id="rssi-value">0</span> dBm</div>
                </div>
                <div class="info-group">
                    <span class="label">IP Address</span>
                    <div class="value" style="font-family: monospace; font-size: 1.1rem;" id="ip-value">0.0.0.0</div>
                </div>
                <a href="/admin/change-wifi" class="btn"><i class="fa-solid fa-gear"></i> Change WiFi</a>
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

    <script>
        async function loadData() {
            try {
                const res = await fetch('/admin/config');
                const json = await res.json();
                
                document.getElementById('ssid-value').innerText = json.WiFi.SSID || "N/A";
                document.getElementById('ip-value').innerText = json.WiFi.IPAddress || "";
                document.getElementById('uptime-value').innerText = "TODO"; // TODO: Uptime in human-readable format
                document.getElementById('rssi-value').innerText = json.WiFi.SignalStrength || "0 dBm";
            } catch (e) {
                console.error("Fetch error", e);
            }
        }
        window.onload = loadData;
        setInterval(loadData, 10000);
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

    inline void get_AdminConfig(Request const &req, Response &res)
    {
        ESP32WebServer::WiFiConfig wifiConfig = ESP32WebServer::getWiFiConfig();

        JsonDocument configDoc;
        configDoc["WiFi"]["SSID"] = wifiConfig.ssid;
        configDoc["WiFi"]["Password"] = wifiConfig.password;
        configDoc["WiFi"]["SignalStrength"] = wifiConfig.signalStrength;
        configDoc["WiFi"]["IPAddress"] = wifiConfig.ipAddress;

        res.json(configDoc);
    }

    class AdminRouter : public ESP32WebServer::Router
    {
    public:
        AdminRouter()
        {
            add("GET", "/admin", get_AdminLogin);
            add("GET", "/admin/config", get_AdminConfig);
            add("POST", "/admin/login", post_AdminLogin);
        }
    };
}