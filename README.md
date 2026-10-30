> [!NOTE]
> 🤖 **AI Notice:** Parts of this repository (code, fixes and documentation) were created or revised with the help of AI ([Claude Code](https://claude.com/claude-code)). All changes were reviewed by the maintainer.

# 🚀 (Prototype) ESP32 Mini WebServer Framework

<div align="center">

<img src=".github/social-preview.png" alt="ESP32 Mini WebServer Framework – Express-style routing, admin dashboard, WiFi management and OTA updates" width="800">

<br><br>

![ESP32](https://img.shields.io/badge/ESP32-000000?style=for-the-badge&logo=Espressif&logoColor=white)
![Arduino](https://img.shields.io/badge/Arduino-00979D?style=for-the-badge&logo=Arduino&logoColor=white)
![PlatformIO](https://img.shields.io/badge/PlatformIO-FF6000?style=for-the-badge&logo=PlatformIO&logoColor=white)
![C++](https://img.shields.io/badge/C++-00599C?style=for-the-badge&logo=cplusplus&logoColor=white)

### ⚠️ This is still a Prototype for my Personal Projects

### 🤓 So Please be nice with your Feedback

</div>


## 🎯 Overview

A lightweight **Mini WebServer Framework** for ESP32 microcontrollers, built as a personal project.

Add it to your PlatformIO project via `lib_deps` and include `MiniServer.h`.

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


## 📦 Installation

Add the following to your `platformio.ini`:

```ini
[env:your_board]
platform = espressif32
framework = arduino
board = <your-board>
board_build.filesystem = littlefs

lib_deps =
    https://github.com/LemurDaniel/ESP32__MiniWebServer-Framework.git

; List any static files you want embedded in the LittleFS image
board_build.include_files_txt =
    data/web/index.html
    data/web/main.css
```

---

## 🔐 Admin Dashboard

The built-in Admin Dashboard is available at `/admin` on every device. No extra configuration needed and can also be turned off easily.

![Admin Dashboard](.assets/admin.dashboard.overview.png)

It provides:

- **WiFi** — scan for nearby networks, select one, enter the password and save. Credentials persist across reboots. Multiple networks can be stored; the device always picks the strongest signal.
- **Security** — change the admin username and password.
- **API Tokens** — create and manage permanent API tokens for programmatic access.
- **Restart** — reboot the device directly from the browser.

#### Default credentials

| Field | Default |
|-------|---------|
| Username | `admin` |
| Password | `admin` |

Override in code before calling `start()`:

```cpp
// Disable the admin dashboard UI (keeps the /admin API routes)
Server->disableAdminDashboard();

// Disable the admin dashboard and all /admin routes entirely
Server->disableAdmin();
```

#### Custom Links & Admin Pages

Add shortcut links to the Admin Dashboard nav bar with `setCustomLink()`. The links appear as badge-style buttons below the dashboard header and open in a new tab:

```cpp
Server->setCustomLink("My App", "/");
Server->setCustomLink("Sensor Data", "/api/sensors");
Server->setCustomLink("Grafana", "http://grafana.local:3000");
```

For more complex use cases — device-specific settings, sensor configuration, custom controls — you can register your own pages under `/admin`. They are automatically protected by the admin auth middleware:

```cpp
Server->get("/admin/sensors", [](EspWeb::Request &req, EspWeb::Response &res) {
    res.html("<h1>Sensor Settings</h1>").OK();
});

// Combine with setCustomLink to make them easily reachable from the dashboard
Server->setCustomLink("Sensor Settings", "/admin/sensors");
```

#### Permanent API Tokens

Permanent tokens can be created from the **API Tokens** section of the admin dashboard. Unlike session tokens (which expire after 1 hour), permanent tokens persist across reboots and never expire.

Use them in API requests via the `Authorization` header:

```http
GET /admin/wifi/active HTTP/1.1
Authorization: <your-token>
```

Tokens are stored in `/admin_perm_token.json` on LittleFS.

#### Protecting your own routes

Use the built-in auth and CORS handlers to secure your API:

```cpp
// On the server
Server->use("/api", Server->auth());
Server->use(Server->cors());

// Inside a Router subclass
use("/api", auth());
route("GET", "/myApi", { cors(), get_myApi });
```

Requests without a valid `Authorization` token or `SessionToken` receive a `401 Unauthorized` response. Tokens are created and managed in the Admin Dashboard.

---

## 🎮 Usage

### 🚀 Quick Start

#### Prerequisites

- ✅ [PlatformIO IDE](https://platformio.org/platformio-ide)
- ✅ ESP32 development board
- ✅ USB cable for programming
- ✅ WiFi network (optional — can be configured via AP mode after flashing)

#### Minimal example

```cpp
#include <Arduino.h>
#include <MiniServer.h>

void setup()
{
    Serial.begin(115200);

    EspWeb::MiniServer *Server = new EspWeb::MiniServer();

    // Optional: set a mDNS hostname — device becomes reachable as myESP32.local
    // Server->dns("myESP32");

    // Optional: serve static files from LittleFS (requires filesystem upload).
    // root() sets the base directory for static files; any request that matches
    // a file in that directory is served automatically.
    // index() defines the file returned for GET /.
    // Server->root("/web");
    // Server->index("/web/index.html");

    // Optional: Take care of CORS
    // Server->use(Server->cors());

    Server->get("/hello", [](EspWeb::Request &req, EspWeb::Response &res) {
        res.text("Hello from ESP32!").OK();
    });

    Server->start(80);
}

void loop() { delay(10); }
```

After uploading, open the serial monitor — the device prints its IP address once connected. Navigate to that address in your browser.

Optionally assign a hostname so the device is reachable as `myESP32.local` instead of an IP (same network only, not available in AP mode):

```cpp
Server->dns("myESP32");
```

---

<details>
<summary>🛜 Setup WiFi via Code</summary>

The device supports two WiFi modes that switch automatically:

**Station mode** — connects to a saved WiFi network on boot and prints the assigned IP to the serial monitor.

**AP mode** — fallback when no saved network is in range. The device creates a hotspot called `ESP32_MiniWebServer` at `192.168.4.1`. Open the Admin Dashboard at `/admin` to scan for networks, enter credentials, and save them.

Credentials can also be saved directly in code and are stored permanently to LittleFS. Multiple networks can be stored — on each boot the device picks whichever saved network has the strongest signal:

```cpp
Server->connectWiFi("HOME_SSID", "HOME_PASSWORD");
Server->connectWiFi("OFFICE_SSID", "OFFICE_PASSWORD");

// Remove all saved networks and force AP mode
// Server->clearWiFi();
```

#### Connection routine

On `Server->start()` and every **30 seconds** while offline:

1. **Scan** for nearby networks and match against all saved networks.
2. **Sort** matches by signal strength.
3. **Connect** to each candidate in order (30-second timeout per attempt).
4. **Fallback to AP mode** if no saved network is reachable.

</details>

---


<details>
<summary>🔨 Upload Website</summary>

**Upload firmware and open the serial monitor**

![Upload and Monitor](.assets/pio.upload-monitor.png)

**Upload the filesystem** — only required when serving static files from LittleFS (e.g. a frontend):

![Build Filesystem](.assets/pio.build-filesystem.png)

> ⚠️ This overwrites the entire LittleFS partition — including any saved WiFi credentials, API tokens, and other config files set via the Admin Dashboard. For a running system, prefer uploading files individually through the Admin Dashboard instead.

</details>

---

<details>
<summary>🛤️ Router System</summary>

For larger projects, group related routes into their own `Router` class instead of registering everything inline on the server.

### 1. Create the header file

Declare handler functions and a `Router` class that registers them:

```cpp
#include <Arduino.h>
#include <WiFi.h>

#include <router/router.h>

namespace routes_example
{
    class Router : public EspWeb::Router
    {
    public:
        Router()
        {
            route("GET", "/hello", get_hello);
            route("GET", "/status", get_status);
            route("GET", "/example", get_example);
            route("POST", "/data", post_data);
        }

    private:
        static void get_hello(EspWeb::Request &req, EspWeb::Response &res);
        static void get_status(EspWeb::Request &req, EspWeb::Response &res);
        static void get_example(EspWeb::Request &req, EspWeb::Response &res);
        static void post_data(EspWeb::Request &req, EspWeb::Response &res);
    };
}
```

### 2. Implement route handlers

In the `.cpp`, include the header and implement each function:

```cpp
#include <routes/routes.example.h>

namespace routes_example
{
    void Router::get_hello(EspWeb::Request &req, EspWeb::Response &res)
    {
        res.text("Hello, World!").status(200);
    }

    void Router::get_status(EspWeb::Request &req, EspWeb::Response &res)
    {
        JsonDocument status;

        status["device"] = "ESP32";
        status["firmware"] = "1.0.0";
        status["uptime"] = static_cast<double>(millis());
        status["free_heap"] = static_cast<double>(ESP.getFreeHeap());
        status["wifi_rssi"] = static_cast<double>(WiFi.RSSI());

        res.json(status).status(200);
    }

    void Router::get_example(EspWeb::Request &req, EspWeb::Response &res)
    {
        res.text("This is an example route!").status(200);
    }

    void Router::post_data(EspWeb::Request &req, EspWeb::Response &res)
    {
        JsonDocument response;
        response["message"] = "Data received successfully";
        response["timestamp"] = millis();
        res.json(response).status(201);
    }
}
```

### 3. Register with the server

```cpp
#include <Arduino.h>
#include <MiniServer.h>
#include <routes/routes.example.h>

void setup()
{
    EspWeb::MiniServer *Server = new EspWeb::MiniServer();

    // Server->connectWiFi("YOUR_SSID", "YOUR_PASSWORD");

    // Serve static files from LittleFS
    Server->root("/web");
    Server->index("/web/index.html");

    Server->registerRouter(routes_example::Router());

    Server->start(80);
}

void loop() { delay(10); }
```

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
void get_temperature(Request &req, Response &res) { ... }
void post_led_control(Request &req, Response &res) { ... }
```

</details>

---

<details>
<summary>📤 Response Handling</summary>

A `Response` object is passed to every handler. The response is sent automatically after the last handler finishes, or as soon as `finalize()` is called.

### Response types

| Method | Content-Type | Example |
|--------|-------------|---------|
| `text(str)` | `text/plain; charset=utf-8` | `res.text("Hello").status(200)` |
| `json(doc)` | `application/json` | `res.json(doc).status(200)` |
| `html(str)` | `text/html; charset=utf-8` | `res.html("<h1>Hi</h1>").status(200)` |
| `file(path)` | auto-detected from extension | `res.file("/web/index.html")` |
| `binary(path)` | `application/octet-stream` | `res.binary("/data.bin")` |

`file(path)` sets the `Content-Type` automatically based on the file extension:

| Extension | Content-Type |
|-----------|-------------|
| `.html` | `text/html; charset=utf-8` |
| `.css` | `text/css` |
| `.js` | `application/javascript` |
| `.json` | `application/json` |
| `.txt` | `text/plain; charset=utf-8` |
| `.xml` | `application/xml` |
| `.svg` | `image/svg+xml` |
| `.png` | `image/png` |
| `.jpg` / `.jpeg` | `image/jpeg` |
| `.ico` | `image/x-icon` |
| *(other)* | `application/octet-stream` |

To override the `Content-Type` without setting a body, use the no-arg setters:

```cpp
res.file("/data.bin").text();   // overwrite to text/plain
res.html();                     // Content-Type: text/html; charset=utf-8
res.json();                     // Content-Type: application/json
res.text();                     // Content-Type: text/plain; charset=utf-8
res.binary();                   // Content-Type: application/octet-stream
```

Or call `.header("Content-Type", "<value>")` directly.

### Status shorthands

These set the status code and fill in a default body if none was set yet:

| Method | Status | Default body |
|--------|--------|-------------|
| `OK()` | 200 | `"OK"` |
| `Created()` | 201 | `"Created"` |
| `BadRequest()` | 400 | `"Bad Request"` |
| `Unauthorized()` | 401 | `"Unauthorized"` |
| `Forbidden()` | 403 | `"Forbidden"` |
| `NotFound()` | 404 | `"Not Found"` |
| `InternalServerError()` | 500 | `"Internal Server Error"` |
| `Redirect(location)` | 302 | *(sets `Location` header)* |

```cpp
res.json(doc).OK();                        // 200, JSON body
res.json(doc).Created();                   // 201, JSON body
res.text("Missing").NotFound();            // 404, custom text body
res.text("No permission").Forbidden();     // 403, custom text body
res.Redirect("/login").finalize();         // 302, redirect
res.text("Created").status(201);           // any status code
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
<summary>📥 Request Body</summary>

The request body is accessed via `req.body` and is read lazily — nothing is read from the socket until you call one of the methods below.

| Method | Description |
|--------|-------------|
| `req.body.text()` | Read body as a `std::string` |
| `req.body.json()` | Parse body as JSON, returns `JsonDocument` |
| `req.body.file()` | Write body to a temp file on LittleFS, returns the file path |
| `req.body.chunks(buf, size)` | Read body in chunks into a buffer, returns bytes read |

`req.body.contentLength` and `req.body.contentType` are always available without triggering a read.

### JSON body

```cpp
Server->post("/data", [](EspWeb::Request &req, EspWeb::Response &res) {
    JsonDocument body = req.body.json();
    const char* name = body["name"];
    res.text(name).OK();
});
```

### Binary / streaming (e.g. OTA firmware)

```cpp
Server->post("/ota", [](EspWeb::Request &req, EspWeb::Response &res) {
    if (Update.begin(req.body.contentLength)) {
        uint8_t buf[1024];
        size_t n;
        while ((n = req.body.chunks(buf, sizeof(buf))) > 0)
            Update.write(buf, n);

        if (Update.end() && Update.isFinished())
            res.text("Update OK").OK();
        else
            res.text("Update failed").InternalServerError();
    }
});
```

</details>

---

<details>
<summary>🔍 URL Query Parameters</summary>

Query parameters from the URL (e.g. `/search?term=hello&page=2`) are parsed automatically and stored in `req.query` as a `std::map<std::string, std::string>`.

```cpp
Server->get("/search", [](EspWeb::Request &req, EspWeb::Response &res) {
    if (req.query.find("term") == req.query.end()) {
        res.text("Missing parameter: term").status(400);
        return;
    }
    std::string term = req.query["term"];
    res.text("Searching for: " + term).OK();
});
```

| Access | Description |
|--------|-------------|
| `req.query["key"]` | Value of a query parameter |
| `req.query.find("key") != req.query.end()` | Check if a parameter is present |

> Parameters are not decoded — percent-encoding (e.g. `%20`) is left as-is.

</details>

---

<details>
<summary>🔀 Route Parameters</summary>

Dynamic path segments are declared with a `:` prefix and accessed via `req.route`:

```cpp
Server->get("/users/:id", [](EspWeb::Request &req, EspWeb::Response &res) {
    std::string id = req.route["id"];
    res.text("User: " + id).OK();
});
```

Multiple parameters in a single path are supported:

```cpp
Server->get("/users/:id/posts/:postId", [](EspWeb::Request &req, EspWeb::Response &res) {
    std::string userId = req.route["id"];
    std::string postId = req.route["postId"];

    JsonDocument doc;
    doc["userId"] = userId;
    doc["postId"] = postId;
    res.json(doc).OK();
});
```

Always check that the parameter exists before using it:

```cpp
Server->get("/echo/:message", [](EspWeb::Request &req, EspWeb::Response &res) {
    const auto it = req.route.find("message");
    if (it == req.route.end()) {
        res.text("missing param").status(400);
        return;
    }
    res.text(it->second).OK();
});
```

| Access | Description |
|--------|-------------|
| `req.route["key"]` | Value of a route parameter |
| `req.route.find("key") != req.route.end()` | Check if a parameter is present |

> Exact path segments take priority over route parameters — `/users/profile` matches before `/users/:id`.

</details>

---

<details>
<summary>🔗 Middleware Chain</summary>

Middleware lets you chain multiple handler functions for a single route. Each handler runs in order. Calling `res.finalize()` stops the chain immediately — no further handlers run.

### How it works

Instead of a single handler, pass a list to `route()`:

```cpp
route("GET", "/secret", {
    authMiddleware,   // runs first — calls finalize() with 401 if not authorized
    get_secret        // only runs if authMiddleware did NOT finalize the response
});
```

The server iterates through all handlers and stops as soon as `response.finalized == true` ([server.cpp](lib/server/server.cpp#L96-L107)).

### Implementing middleware

A middleware handler has the same signature as a regular route handler:

```cpp
void authMiddleware(EspWeb::Request &req, EspWeb::Response &res)
{
    if (req.cookies.find("session") == req.cookies.end())
    {
        res.text("Unauthorized").status(401).finalize();
        // Chain stops here — get_secret is never called
        return;
    }
    // No finalize() call → next handler in the chain runs
}

void get_secret(EspWeb::Request &req, EspWeb::Response &res)
{
    res.text("Secret data!").status(200);
}
```

### Registering middleware

**Per-route** — pass a handler list directly to `route()`. Runs only for that specific route:

```cpp
route("GET", "/secret",  { authMiddleware, get_secret  });
route("POST", "/config", { authMiddleware, post_config });
```

**Global / prefix-based** — register via `use()`. Runs before every route whose path starts with the given prefix. Useful for logging, CORS headers, or auth that spans multiple endpoints:

> ⚠️ `use()` matches **full path segments only** — no wildcards, no partial matches.
> `use("/api")` matches `/api`, `/api/data`, `/api/config` but **not** `/apiv2` or `/ap`.

```cpp
class Router : public EspWeb::Router
{
public:
    Router()
    {
        // Runs before every route
        use("/", loggingMiddleware);

        // Runs before every route under /api (full segment match — /apiv2 is NOT matched)
        use("/api", authMiddleware);

        route("GET", "/hello",      get_hello);
        route("GET", "/api/data",   get_data);
        route("POST", "/api/config", post_config);
    }
};
```

### Execution flow

```
Request → use() handlers (in registration order, prefix-matched)
              │
              ├─ finalize() called → Response sent immediately
              │
              └─ not finalized ──→ per-route handler chain
                                        │
                                        ├─ finalize() called → Response sent
                                        │
                                        └─ not finalized ──→ final handler → Response sent
```

### Common middleware patterns

| Pattern | Where | Description |
|---------|-------|-------------|
| Logging | `use("/")` | Log every request without repeating per route |
| CORS headers | `use("/")` | Add headers to every response |
| Auth (global) | `use("/api")` | Protect all `/api/*` routes at once |
| Auth (per-route) | `route()` chain | Fine-grained control per endpoint |
| Rate limiting | `use("/")` | Track request counts, abort with `429` |

➡️ See also: **📤 Response Handling** above for all available response methods.

</details>

---

<details>
<summary>🔒 Built-in Middleware</summary>

Both handlers are available on the server and directly inside any `Router` subclass — no server reference needed:

#### CORS

```cpp
// On the server
Server->use(Server->cors());
Server->use(Server->cors("https://my-frontend.example.com"));

// Inside a Router subclass
use("/", cors());
use("/", cors("https://my-frontend.example.com"));
```

#### Auth

Protects routes using Permanent API Tokens created in the Admin Dashboard. Checks the `Authorization` header — no session or cookie required:

```cpp
// On the server
Server->use("/api", Server->auth());

// Inside a Router subclass
use("/api", auth());
route("GET", "/myApi", { auth(), get_myApi });
```

Requests without a valid `Authorization` token receive a `401 Unauthorized` response.

#### Managing tokens from code

Tokens can also be issued and revoked programmatically from within any Router. The tokens work with the same `auth()` handler:

```cpp
class Router : public EspWeb::Router
{
public:
    Router()
    {
        use("/api", auth());

        route("POST", "/api/login", post_login);
        route("POST", "/api/logout", post_logout);
        route("GET",  "/api/data",  get_data);
    }

private:
    static void post_login(Request &req, Response &res)
    {
        // Issue a session token — expires after 3600 seconds
        // Second argument is the list of allowed actions
        std::string token = Router::getSessionToken(3600, { "admin", "sensors" });
        res.header("Set-Cookie", "adminToken=" + token + "; path=/").OK();
    }

    static void post_logout(Request &req, Response &res)
    {
        res.header("Set-Cookie", "adminToken=; Max-Age=0; path=/").OK();
    }

    static void get_data(Request &req, Response &res)
    {
        // Only reachable with a valid token — auth() already checked
        res.text("secret").OK();
    }
};
```

Permanent tokens (no expiry) can be managed the same way:

```cpp
// Create a named permanent API token with specific actions
std::string token = Router::getApiToken("my-device", { "admin", "sensors" });

// Revoke it
Router::removeApiToken("my-device");
```

#### Token actions

Every token carries a list of **actions** — strings that describe what the token is allowed to do. The default list is `{ "admin" }`.

Define your own actions globally before calling `start()`:

```cpp
Server->setTokenActions({ "sensors", "control", "status" });
// "admin" is always prepended automatically
```

Inside a handler, check the token on the incoming request:

```cpp
void get_data(EspWeb::Request &req, EspWeb::Response &res)
{
    if (!req.token.isValid())
    {
        res.Unauthorized();
        return;
    }

    if (!req.token.isAllowed("sensors"))
    {
        res.Forbidden();
        return;
    }

    res.text("sensor data").OK();
}
```

| Method | Description |
|--------|-------------|
| `req.token.isValid()` | `false` if no token was sent or the token is expired / unknown |
| `req.token.isAllowed("action")` | `true` if the token's action list contains the given string |
| `req.token.isPrivileged()` | Shorthand for `isAllowed("admin")` |

</details>

---

<details>
<summary>📡 Streaming</summary>

Streaming sends data incrementally over a single HTTP connection using **chunked transfer encoding**. The connection stays open until `endStream()` is called.

### Basic streaming

```cpp
Server->get("/stream", [](EspWeb::Request &req, EspWeb::Response &res) {
    res.beginStream();

    for (int i = 0; i < 100; i++) {
        res.sendChunk(std::to_string(i) + "\n");
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    res.endStream();
});
```

`beginStream()` sends the HTTP response headers immediately. Each `sendChunk()` call flushes a chunk to the client right away. `endStream()` sends the terminal chunk and closes the stream.

### Server-Sent Events (SSE)

For real-time updates in a browser without JavaScript polling, use the `text/event-stream` content type. The browser's built-in `EventSource` API handles reconnection automatically.

**Server:**
```cpp
Server->get("/events", [](EspWeb::Request &req, EspWeb::Response &res) {
    res.header("Content-Type", "text/event-stream");
    res.beginStream();

    for (int i = 0; i < 100; i++) {
        std::string msg = "data: " + std::to_string(i) + "\n\n";
        res.sendChunk(msg);
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    res.endStream();
});
```

**Browser:**
```javascript
const es = new EventSource('/events');
es.onmessage = e => console.log(e.data);
es.onerror   = () => es.close();
```

> SSE messages must end with a blank line (`\n\n`). Named events use `event: <name>\n` before the `data:` line.

### Adjusting the timeout

The default socket timeout is 30 seconds. For long-running streams, call `setTimeout()` **before** `beginStream()`:

```cpp
res.setTimeout(300); // 5 minutes
res.beginStream();
```

Pass `0` to disable the timeout entirely.

### API reference

| Method | Description |
|--------|-------------|
| `beginStream()` | Send response headers and open the stream |
| `sendChunk(std::string)` | Send a string chunk |
| `sendChunk(uint8_t*, size_t)` | Send a binary chunk |
| `endStream()` | Send the terminal chunk and close the stream |
| `setTimeout(int seconds)` | Set socket send/receive timeout |

> Testing with `curl -N http://<IP>/stream` is the fastest way to verify streaming works before debugging browser behaviour.

</details>

---

<details>
<summary>📁 File Handling (LittleFS)</summary>

File handling is available through the static `Router::fs` member inside any `Router` subclass (no server reference needed). It wraps the `JsonFileHandler` library on top of LittleFS.

### JSON files

```cpp
// Read from Non-Volatile Storage (NVS)
JsonDocument cfg = Router::fs.readJsonConfig("my_config");
const char* name = cfg["name"];

// Write to Non-Volatile Storage (NVS)
JsonDocument out;
out["status"] = "ok";
Router::fs.writeJsonConfig("my_config", out);

// Read from LittleFS file
JsonDocument cfg = Router::fs.readJsonFile("/config.json");

// Write to LittleFS file
Router::fs.writeJsonFile("/config.json", out);
```

### Filesystem operations

```cpp
// Check existence
bool ok = Router::fs.exists("/config.json");

// List contents of a folder
std::vector<EspWeb::FileInfo> files = Router::fs.listFiles("/data");
for (const auto &f : files)
    Serial.println(f.path.c_str());

// Move a file
Router::fs.moveFile("/tmp/upload", "/data/file.bin");

// Delete a file
Router::fs.removeFile("/data/old.json");

// Clear folder contents (keeps the folder)
Router::fs.clearFolder("/tmp");

// Delete folder and all its contents
Router::fs.removeFolder("/cache");
```

### Temp files

Use temp paths as a staging area before moving files into place:

```cpp
std::string tmpPath = Router::fs.getTempFile().c_str();
// … write to tmpPath …
Router::fs.moveFile(tmpPath, "/data/result.bin");
```

### API reference

| Method | Description |
|--------|-------------|
| `fs.readJsonConfig(key)` | Read a `JsonDocument` from Non-Volatile Storage |
| `fs.writeJsonConfig(key, doc)` | Write a `JsonDocument` to Non-Volatile Storage |
| `fs.readJsonFile(path)` | Read a JSON file from LittleFS → `JsonDocument` |
| `fs.writeJsonFile(path, doc)` | Write a `JsonDocument` to a LittleFS JSON file |
| `fs.readFile(path)` | Read a raw file from LittleFS → `std::string` |
| `fs.exists(path)` | Check whether a file or folder exists |
| `fs.listFiles(path)` | List all entries in a folder → `vector<FileInfo>` |
| `fs.moveFile(src, dst)` | Move or rename a file |
| `fs.removeFile(path)` | Delete a file |
| `fs.clearFolder(path)` | Delete all contents of a folder, keep the folder |
| `fs.removeFolder(path)` | Delete a folder and all its contents |
| `fs.getTempFolder()` | Path of the temp folder (`/temp`) |
| `fs.getTempFile()` | Path of a new uniquely-named temp file |

`FileInfo` fields:

| Field | Type | Description |
|-------|------|-------------|
| `name` | `String` | Filename (e.g. `config.json`) |
| `path` | `String` | Full path (e.g. `/data/config.json`) |
| `baseName` | `String` | Name without extension |
| `extension` | `String` | Extension without dot (e.g. `json`) |
| `isDirectory` | `int` | Non-zero if the entry is a directory |

</details>

---

