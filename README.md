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

A lightweight **Mini WebServer Framework** for ESP32 microcontrollers! 🎉 

This framework is a personal Project providing a MiniWebserver with dynmiacally managable Endpoints and easier request and reponse handleing! 📊✨

## 📁 Project Structure

```
📦 ESP32 Mini WebServer Framework
├── 📁 src/
│   ├── 🎯 main.cpp                 # Main application entry
│   └── 📁 routes/
│       └── 🛤️ routes.test.h        # API route definitions
├── 📁 lib/
│   ├── 🔧 utilities.h             # Helper utilities
│   └── 📁 server/
│       ├── 🌐 server.h/.cpp        # Core web server
│       ├── 🛤️ router.h             # Routing engine
│       ├── 📥 request.h            # HTTP request handling
│       ├── 📤 response.h           # HTTP response handling
│       └── 📋 ArduinoJson-v7.4.3.h # JSON library
├── 📁 data/
│   └── 📁 web/
│       └── 🎨 index.html           # Web interface
├── ⚙️ platformio.ini               # Build configuration
└── 📖 README.md                    # This file
```

## 🚀 Quick Start

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
   custom_utils::connectWiFi("YOUR_WIFI_NAME", "YOUR_PASSWORD");
   ```
   
4. **Build and Upload FileSystem (to push /data contents to ESP32)** 🔨

![PlatformIO.IO](.assets/pio.build-filesystem.png)

5. **Use Plattform.IO to Upload and Monitor code from main.cpp** 🔨

![PlatformIO.IO](.assets/pio.upload-monitor.png)

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

## 🛤️ Router System & Route Management

### 📁 **Router File Structure**

Routes are organized in separate header files within the `src/routes/` directory, each following a clean, consistent pattern:

```cpp
// src/routes/routes.example.h
#include <../lib/server/router.h>

namespace routes_example {
    
    // Route Handler Functions
    void get_example(const ESP32WebServer::Request &req, ESP32WebServer::Response &res) {
        res.text("This is an example route!").status(200);
    }
    
    void post_data(const ESP32WebServer::Request &req, ESP32WebServer::Response &res) {
        JsonDocument response;
        response["message"] = "Data received successfully";
        response["timestamp"] = millis();
        res.json(response).status(201);
    }
    
    // Router Configuration Function
    ESP32WebServer::Router Router() {
        ESP32WebServer::Router router;
        router.add("GET", "/example", get_example);
        router.add("POST", "/data", post_data);
        return router;
    }
}
```

### ⚡ **Adding New Routes to the Server**

#### **Directly Configure Routes on the Server**
```cpp
#include <Arduino.h>

#include <../lib/utilities.h>
#include <../lib/server/server.h>

// Include the Router File
#include <routes/routes.example.h>

ESP32WebServer::MiniServer Server = ESP32WebServer::MiniServer("0.0.0.0", 80);

void setup()
{
  Serial.begin(115200);

  custom_utils::connectWiFi("FRITZ!Box 6591 TPLink 2,4_EXT2", "**secret-pwd**");

  // Set a path as index (Needs LittleFS as FileSystem)
  Server.index("/web/index.html");

  // Register Routes from the seperate Files
  Server.registerRouter(routes_example::Router());

  // Directly Configure Routes
  // 🟢 GET routes
  // router.add("GET", "/sensors", get_sensor_data);
  // router.add("GET", "/system/info", get_system_info);
    
  // 🟡 POST routes
  // router.add("POST", "/led", post_led_control);
  // router.add("POST", "/config", post_configuration);
    
  // 🔵 PUT routes
  // router.add("PUT", "/settings", put_update_settings);
}
```

### 🎨 **Route Organization Best Practices**

#### **📂 Organize by Functionality**
```
src/routes/
├── 🌡️ routes.sensors.h      # Temperature, humidity, pressure sensors
├── 💡 routes.control.h      # LED, relay, motor control
├── ⚙️ routes.system.h       # System info, diagnostics, configuration
├── 🔐 routes.auth.h         # Authentication & user management
├── 📊 routes.api.h          # General API endpoints
└── 🧪 routes.test.h         # Testing & development routes
```

#### **🏷️ Consistent Naming Convention**
```cpp
// Namespace naming: routes_{functionality}
namespace routes_sensors { ... }
namespace routes_control { ... }
namespace routes_system { ... }

// Function naming: {method}_{endpoint_name}
void get_temperature() { ... }
void post_led_control() { ... }
void put_system_config() { ... }
```

#### **📤 Sending Responses**
```cpp
void get_example(const ESP32WebServer::Request &req, ESP32WebServer::Response &res) {
    // 📝 Text response
    res.text("Simple text response").status(200);
    
    // 📋 JSON response
    JsonDocument jsonData;
    jsonData["message"] = "Success";
    jsonData["timestamp"] = millis();
    res.json(jsonData).status(200);
    
    // 📁 File response
    res.file("/web/data.json").status(200);
    
    // ❌ Error responses
    res.text("Not found").status(404);
    res.text("Internal error").status(500);
}
```

## 📄 License

This project is licensed under the **MIT License** 📜 - see the LICENSE file for details.

## 🙏 Acknowledgments

- 🎉 **Arduino Community** for the amazing ecosystem
- 🔧 **PlatformIO** for the excellent development platform
- 🌐 **ESP32** community for inspiration and support
- 💖 **Open Source** contributors worldwide
