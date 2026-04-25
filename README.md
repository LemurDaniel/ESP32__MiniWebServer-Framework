# 🚀 (Prototype) ESP32 Mini WebServer Framework

<div align="center">

![ESP32](https://img.shields.io/badge/ESP32-000000?style=for-the-badge&logo=Espressif&logoColor=white)
![Arduino](https://img.shields.io/badge/Arduino-00979D?style=for-the-badge&logo=Arduino&logoColor=white)
![PlatformIO](https://img.shields.io/badge/PlatformIO-FF6000?style=for-the-badge&logo=PlatformIO&logoColor=white)
![C++](https://img.shields.io/badge/C++-00599C?style=for-the-badge&logo=cplusplus&logoColor=white)

### ⚠️ This is still a Prototype for my Personal Projects

### 🤓 So Please be nice with your Feedback

</div>


## 🎯 Overview

A lightweight **Mini WebServer Framework** for ESP32 microcontrollers as a personal project! 🎉 

Just copy the server folder into your project and include server.h for the server and router.h for your routes.

May not work with the RISC V Processor
- ESP32-C3
- ESP32-C6
- ESP32-H2

It Should work with following families:
- ESP32 (WROOM, WROVER, CAM ...)
- ESP32-S2
- ESP32-S3

## 🙏 Acknowledgments

- 🎉 **Arduino Community** for the amazing ecosystem
- 🔧 **PlatformIO** for the excellent development platform
- 🌐 **ESP32** community for inspiration and support
- 💖 **Open Source** contributors worldwide


## 📁 Project Structure

```
📦 ESP32 Mini WebServer Framework
├── 📁 src/
│   ├── 🎯 main.cpp                 # Main application entry
│   └── 📁 routes/
│       └── 🛤️ routes.example.h     # API route definitions
│       └── 🛤️ routes.example.cpp   # API route implementations
├── 📁 data/
│   └── 📁 web/
│       └── 🎨 index.html           # Web interface
├── 📁 lib/
│   └── 📁 server/
│       ├── 🌐 server.h/.cpp        # Core web server
│       ├── 🛤️ router.h             # Routing engine
│       ├── 📥 request.h            # HTTP request handling
│       ├── 📤 response.h           # HTTP response handling
│       ├── 🔐 utility.admin.h      # Admin Dashboard
│       ├── 🛜 utility.wifi.h       # WiFi utility
│       ├── 📂 utility.file.h       # File utility
│       └── 📋 ArduinoJson-v7.4.3.h # JSON library
├── ⚙️ platformio.ini               # Build configuration
└── 📖 README.md                    # This file
```

## 🎮 Usage

### **Accessing the Web Interface**

1. Connect your ESP32 to power
2. Wait for WiFi connection
3. Find the ESP32's IP address in serial monitor
4. Open your browser and navigate to `http://ESP32_IP_ADDRESS`

### **Example API Endpoints**

| Method | Endpoint | Description | Response |
|--------|----------|-------------|----------|
| 🟢 GET | `/hello` | Simple hello world | Text response 📝 |
| 🟢 GET | `/status` | Device status | JSON with device info 📊 |
| 🟢 GET | `/example` | Example Get | Text response 📝 |
| 🟡 POST | `/data` | Simple POST | JSON data response 📊 |


---

<details>
<summary>🚀 Quick Start</summary>

### Prerequisites

- ✅ [PlatformIO IDE](https://platformio.org/platformio-ide)
- ✅ ESP32 development board
- ✅ USB cable for programming 🔌
- ✅ WiFi network 📶

### Installation

1. **Connect your ESP32 via USB Cable** 🔌

2. **Install Plattfrom.IO Extension** 💾

3. **Update WiFi credentials** 🔐
   Edit `src/main.cpp` and update your WiFi settings:
   ```cpp
   Server->connectWiFi("YOUR_SSID", "YOUR_PASSWORD");
   ```

4. **Build and Upload FileSystem (to push /data contents to ESP32)** 🔨

![PlatformIO.IO](.assets/pio.build-filesystem.png)

5. **Use PlatformIO to Upload and Monitor code from main.cpp** 🔨

![PlatformIO.IO](.assets/pio.upload-monitor.png)

</details>

---

<details>
<summary>🛤️ Router System</summary>

### Best Practices

**Organize by functionality:**
```
src/routes/
├── routes.sensors.h/.cpp   # 🌡️ Temperature, humidity, pressure
├── routes.control.h/.cpp   # 💡 LED, relay, motor control
├── routes.system.h/.cpp    # 📊 System info, diagnostics
└── routes.auth.h/.cpp      # 🔐 Authentication & user management
```

**Naming convention:**
```cpp
// Namespace: routes_{functionality}
namespace routes_sensors { ... }

// Functions: {method}_{endpoint}
void get_temperature(const Request &req, Response &res) { ... }
void post_led_control(const Request &req, Response &res) { ... }
```

### 1. Create the header file

Declare handler functions and a `Router` class that registers them:

```cpp
#include <../lib/server/router.h>

namespace routes_example
{
    void get_hello(const ESP32WebServer::Request &req, ESP32WebServer::Response &res);
    void get_status(const ESP32WebServer::Request &req, ESP32WebServer::Response &res);
    void post_data(const ESP32WebServer::Request &req, ESP32WebServer::Response &res);

    class Router : public ESP32WebServer::Router
    {
    public:
        Router()
        {
            add("GET",  "/hello",  get_hello);
            add("GET",  "/status", get_status);
            add("POST", "/data",   post_data);
        }
    };
}
```

### 2. Implement route handlers

In the `.cpp`, include the header and implement each function:

```cpp
#include <routes/routes.example.h>

namespace routes_example
{
    void get_hello(const ESP32WebServer::Request &req, ESP32WebServer::Response &res)
    {
        res.text("Hello from ESP32!").OK();
    }

    void get_status(const ESP32WebServer::Request &req, ESP32WebServer::Response &res)
    {
        JsonDocument doc;
        doc["device"]    = "ESP32";
        doc["uptime"]    = static_cast<double>(millis());
        doc["free_heap"] = static_cast<double>(ESP.getFreeHeap());
        doc["wifi_rssi"] = static_cast<double>(WiFi.RSSI());
        res.json(doc).OK();
    }

    void post_data(const ESP32WebServer::Request &req, ESP32WebServer::Response &res)
    {
        JsonDocument doc;
        doc["message"]   = "Data received";
        doc["timestamp"] = static_cast<double>(millis());
        res.json(doc).status(201);
    }
}
```

### 3. Register with the server

```cpp
#include <Arduino.h>
#include <../lib/server/server.h>
#include <routes/routes.example.h>

void setup()
{
    Serial.begin(115200);

    ESP32WebServer::MiniServer* Server = ESP32WebServer::MiniServer::instance();

    // Optional: connect to a WiFi network
    // If omitted, the server starts in AP mode for WiFi configuration via the admin dashboard
    Server->connectWiFi("YOUR_SSID", "YOUR_PASSWORD");

    // Serve index.html at / (requires LittleFS filesystem upload)
    Server->index("/web/index.html");

    // Register all routes from a Router class
    Server->registerRouter(routes_example::Router());

    // Or add individual routes directly:
    // Server->get("/sensors", get_sensor_data);
    // Server->post("/led",    post_led_control);
    // Server->put("/config",  put_update_settings);

    // Start the server — spawns FreeRTOS tasks internally
    Server->start("0.0.0.0", 80);
}

void loop() { delay(10); }
```

</details>

---

<details>
<summary>📤 Response Handling</summary>

A Response object is provided and passed to all handler functions

The response is sent automatically 
- after the last handler finishes
- or a response in the chain is finalized

### Response types

| Method | Content-Type | Example |
|--------|-------------|---------|
| `text(str)` | `text/plain` | `res.text("Hello").status(200)` |
| `json(doc)` | `application/json` | `res.json(doc).status(200)` |
| `html(str)` | `text/html` | `res.html("<h1>Hi</h1>").status(200)` |
| `file(path)` | `text/html` | `res.file("/web/index.html")` |
| `binaryFile(path)` | `application/octet-stream` | `res.binaryFile("/data.bin")` |

### Status shorthands

These set the status code and fill in a default body if none was set yet:

| Method | Status | Default body |
|--------|--------|-------------|
| `OK()` | 200 | `"OK"` |
| `NotFound()` | 404 | `"Not Found"` |
| `InternalServerError()` | 500 | `"Internal Server Error"` |

```cpp
res.json(doc).OK();              // 200, JSON body
res.text("Missing").NotFound();  // 404, custom text body
res.text("Created").status(201); // any status code
```

### Stopping the middleware chain

Call `finalize()` to prevent any further handlers from running — typically used in middleware:

```cpp
res.text("Unauthorized").status(401).finalize();
```

➡️ See **🔗 Middleware** below for full details.

</details>

---

<details>
<summary>🔗 Middleware Chain</summary>

Middleware allows you to chain multiple handler functions for a single route. Each handler runs in order. Calling `res.finalize()`, stops processing further handlers in the chain

### **How it works**

Instead of a single `RequestHandler`, pass a `std::vector<RequestHandler>` to `add()`:

```cpp
add("GET", "/secret", {
    authMiddleware,   // runs first — aborts with 401 if not authorized via finalize()
    get_secret        // only runs if authMiddleware did NOT finalize the response
});
```

The server iterates through all handlers and breaks as soon as `response.finalized == true` ([server.cpp](lib/server/server.cpp#L96-L107)).

### **Implementing Middleware**

A middleware handler has the same signature as a regular route handler. Call `res.finalize()` to short-circuit the chain:

```cpp
void authMiddleware(const ESP32WebServer::Request &req, ESP32WebServer::Response &res)
{
    // Check for a valid session cookie or token
    if (req.cookies.find("session") == req.cookies.end())
    {
        res.text("Unauthorized").status(401).finalize();
        // Chain stops here — get_secret is never called
        return;
    }
    // No finalize() call → next handler in the chain runs
}

void get_secret(const ESP32WebServer::Request &req, ESP32WebServer::Response &res)
{
    res.text("Secret data!").status(200);
}
```

### **Registering Middleware in a Router**

```cpp
class Router : public ESP32WebServer::Router
{
public:
    Router()
    {
        // Public routes — single handler
        add("GET", "/hello", get_hello);

        // Protected routes — middleware chain
        add("GET", "/secret",  { authMiddleware, get_secret  });
        add("POST", "/config", { authMiddleware, post_config });
    }
};
```

### **Execution Flow**

```
Request → authMiddleware
              │
              ├─ not authorized → res.finalize() → Response (401) sent
              │
              └─ authorized ────────────────────→ get_secret → Response (200) sent
```

### **Common Middleware Patterns**

| Pattern | Description |
|---------|-------------|
| Authentication | Check session cookie / token, abort with `401` if missing |
| Authorization | Verify user role/permissions, abort with `403` |
| Logging | Log request details, then call next handler without finalizing |
| Rate Limiting | Track request counts, abort with `429` if threshold exceeded |

➡️ See also: **📤 Response Handling** section above for all available response methods.

</details>
