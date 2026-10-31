// ESP32 MiniWebServer Framework - https://github.com/LemurDaniel/ESP32__MiniWebServer-Framework
// Copyright © 2026, Daniel Landau
// MIT License

// NOTE:
//   Entry point of the framework, include only this header.
//   Requires: LittleFS partition (board_build.filesystem = littlefs) and ArduinoJson.
//
//   EspWeb::MiniServer *server = new EspWeb::MiniServer();
//   server->get("/hello", [](EspWeb::Request &req, EspWeb::Response &res) { res.text("Hello"); });
//   server->start(80);

//
//   Chapters:
//     SETTINGS        Connection limits of the server.
//     SERVE HTTP      The MiniServer class.
//

#pragma once

#include <Arduino.h>

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

#include <map>
#include <string>
#include <vector>

#include <ArduinoJson.h>

#include <router/router.h>
#include <utils/utility.admin.h>
#include <utils/utility.file.h>
#include <utils/utility.path.h>
#include <utils/utility.token.h>
#include <utils/utility.wifi.h>

namespace EspWeb
{
    // ===========================================================================
    // ==============================   SETTINGS   ===============================
    // ===========================================================================
    //
    //   CONNECTION_LIMIT         Queued connections, in listen() and the worker queue.
    //   CONNECTION_TIMEOUT_SEC   Send and receive timeout per client.
    //

    /** @brief Maximum number of queued incoming connections. */
    const int CONNECTION_LIMIT = 10;

    /** @brief Socket send/receive timeout in seconds for each client connection. */
    const int CONNECTION_TIMEOUT_SEC = 30;

    // ===========================================================================
    // =============================   SERVE HTTP   ==============================
    // ===========================================================================
    //
    //   Lifecycle          Constructor, start and the socket.
    //   Network            WiFi credentials and the mDNS name.
    //   Admin              Credentials, links, token actions and the admin switches.
    //   Static files       Single files, the index page and whole folders.
    //   Routing            Routers and routes per HTTP method.
    //   Middleware         Prefix middlewares and the built-in factories.
    //   Request handling   Accept task, worker tasks and the handler pipeline.
    //

    /**
     * @brief Main HTTP server class for the ESP32 MiniWebServer Framework.
     *
     * Listens on a TCP socket, dispatches incoming requests through a middleware/route
     * pipeline, and manages worker tasks for concurrent request handling.
     * Includes a built-in admin panel (can be disabled) for WiFi config, OTA updates,
     * and API token management.
     */
    class MiniServer
    {
        // Lifecycle
    public:
        /** @brief Mounts LittleFS, formatting it on first use. */
        MiniServer();
        ~MiniServer();

        /**
         * @brief Starts the HTTP server on the given port and spawns worker tasks.
         * @param port        TCP port to listen on (e.g. 80).
         * @param ipAddr      IP address to bind to (default `"0.0.0.0"` for all interfaces).
         * @param workerCount Number of FreeRTOS worker tasks for concurrent request handling (default 3).
         * @return 0 on success (also when already running), non-zero on failure.
         */
        int start(const int port, const std::string &ipAddr = "0.0.0.0", int workerCount = 3);

    private:
        /** @brief Closes the server socket and cleans up resources. */
        void closeServer();

        /** @brief True while the server is running. */
        bool _isRunning = false;

        /** @brief Bound server socket address. */
        struct sockaddr_in _address;

        /** @brief Length of _address. */
        unsigned int _addressLen;

        /** @brief Listening server socket file descriptor. */
        int _serverSocket;

        // Network
    public:
        /**
         * @brief Registers an mDNS hostname so the device is reachable as `<dnsName>.local`.
         * @param dnsName Hostname without the `.local` suffix.
         */
        void dns(const std::string &dnsName);

        /**
         * @brief Saves WiFi credentials, used by the connection routine on start().
         *
         * If no network is saved, the server starts in AP mode with SSID `"ESP32_MiniWebServer"`
         * and exposes the admin page for WiFi configuration.
         * @param ssid     SSID of the WiFi network to connect to.
         * @param password Password for the WiFi network.
         */
        void connectWiFi(const std::string &ssid, const std::string &password);

        /**
         * @brief Removes all saved WiFi credentials, causing a fallback to AP mode on next boot.
         */
        void clearWiFi();

    private:
        /** @brief mDNS hostname registered for the device. */
        std::string _dnsName;

        // Admin
    public:
        /**
         * @brief Overrides the salt used when hashing admin credentials.
         * @param salt Custom salt string.
         */
        void defaultAdminSalt(const std::string &salt);

        /**
         * @brief Overrides the default admin username and password.
         * @param username New admin username.
         * @param password New admin password (stored as a salted SHA-256 hash).
         */
        void defaultAdminCredentials(const std::string &username, const std::string &password);

        /**
         * @brief Adds a custom link entry to the admin dashboard navigation.
         * @param name Display name of the link.
         * @param href Target URL.
         */
        void setCustomLink(const std::string &name, const std::string &href);

        /**
         * @brief Sets the list of allowed actions that can be assigned to API tokens.
         *
         * `"admin"` is always added as first action.
         * @param actions Vector of action name strings.
         */
        void setTokenActions(const std::vector<std::string> &actions);

        /**
         * @brief Completely disables the built-in admin panel and all its routes.
         */
        void disableAdmin();

        /**
         * @brief Disables only the admin HTML dashboard pages while keeping API routes active.
         */
        void disableAdminDashboard();

    private:
        /** @brief When false, the built-in admin panel is completely disabled. */
        bool _isAdminEnabled = true;

        /** @brief When false, the admin HTML dashboard pages are not served. */
        bool _isDashboardEnabled = true;

        // Static files
    public:
        /**
         * @brief Serves a static LittleFS file for a specific URL path.
         * @param path     URL path (e.g. `"/favicon.ico"`).
         * @param filePath Absolute LittleFS path to the file (e.g. `"/favicon.ico"`).
         */
        void staticFile(const std::string &path, const std::string &filePath);

        /**
         * @brief Serves a LittleFS file as the site root (`/`, `/index` and `/index.html`).
         * @param indexPath Absolute LittleFS path to the HTML file (e.g. `"/index.html"`).
         */
        void index(const std::string &indexPath);

        /**
         * @brief Serves all files in a LittleFS folder under a URL prefix, recursively.
         * @param folderPath Absolute LittleFS folder path (e.g. `"/web"`).
         * @param prefix     URL prefix to mount the folder under (default `"/"`).
         */
        void root(const std::string &folderPath, const std::string &prefix = "/");

    private:
        /** @brief True once a root folder has been configured via root(). */
        bool _isRootSet = false;

        /** @brief True once an index file has been configured via index(). */
        bool _isIndexSet = false;

        // Routing
    public:
        /**
         * @brief Mounts all routes from a Router instance into the server.
         * @param router Router containing the routes and middlewares to register.
         */
        void registerRouter(const EspWeb::Router &router);

        /**
         * @brief Registers a single route with an explicit HTTP method.
         * @param method  HTTP method string (e.g. `"GET"`, `"POST"`).
         * @param path    URL path pattern, supports `:param` segments.
         * @param handler Request handler function.
         */
        void route(const std::string &method, const std::string &path, const RequestHandler &handler);

        /**
         * @brief Registers a GET route.
         * @param path    URL path pattern.
         * @param handler Request handler function.
         */
        void get(const std::string &path, const RequestHandler &handler);

        /**
         * @brief Registers a POST route.
         * @param path    URL path pattern.
         * @param handler Request handler function.
         */
        void post(const std::string &path, const RequestHandler &handler);

        /**
         * @brief Registers a PUT route.
         * @param path    URL path pattern.
         * @param handler Request handler function.
         */
        void put(const std::string &path, const RequestHandler &handler);

        /**
         * @brief Registers a PATCH route.
         * @param path    URL path pattern.
         * @param handler Request handler function.
         */
        void patch(const std::string &path, const RequestHandler &handler);

        /**
         * @brief Registers a DELETE route. (`delete` is a reserved C++ keyword.)
         * @param path    URL path pattern.
         * @param handler Request handler function.
         */
        void del(const std::string &path, const RequestHandler &handler);

    private:
        /**
         * @brief Registers a route with a single handler.
         * @param method  HTTP method string.
         * @param path    URL path pattern.
         * @param handler Handler function.
         */
        void addRoute(const std::string &method, const std::string &path, const RequestHandler &handler);

        /**
         * @brief Registers a route with multiple chained handlers, the one real implementation.
         * @param method   HTTP method string.
         * @param path     URL path pattern.
         * @param handlers Ordered list of handler functions.
         */
        void addRoute(const std::string &method, const std::string &path, const std::vector<RequestHandler> &handlers);

        /** @brief Trie-based route map for `"METHOD/path"` → handler chains. */
        PathResolver _routes;

        // Middleware
    public:
        /**
         * @brief Adds a global middleware applied to every request.
         * @param handler Middleware function.
         */
        void use(const RequestHandler &handler);

        /**
         * @brief Adds a prefix-scoped middleware applied only to matching paths.
         * @param prefix  URL prefix to match (e.g. `"/api"`).
         * @param handler Middleware function.
         */
        void use(const std::string &prefix, const RequestHandler &handler);

        /**
         * @brief Returns a CORS middleware handler.
         * @param origin Allowed origin header value (default `"*"`).
         * @return RequestHandler that sets the CORS header.
         */
        RequestHandler cors(const std::string &origin = "*") { return Router::cors(origin); }

        /**
         * @brief Returns the built-in token authentication middleware handler.
         * @return RequestHandler that rejects requests without a valid session or API token.
         */
        RequestHandler auth() { return Router::auth(); }

    private:
        /** @brief Trie-based middleware map for prefix-matched middleware chains. */
        PathResolver _middlewares;

        // Request handling
    private:
        /**
         * @brief Parses an incoming HTTP request and runs the handler pipeline.
         * @param clientSocket File descriptor of the accepted client socket.
         */
        void handleClient(int clientSocket);

        /**
         * @brief FreeRTOS task that dequeues client sockets and calls handleClient().
         * @param param Pointer to the owning MiniServer instance.
         */
        static void workerTask(void *param);

        /**
         * @brief FreeRTOS task that calls accept() in a loop and enqueues client sockets.
         * @param param Pointer to the owning MiniServer instance.
         */
        static void acceptClientTask(void *param);

        /** @brief FreeRTOS queue carrying accepted client socket descriptors to worker tasks. */
        QueueHandle_t _handleQueue = xQueueCreate(CONNECTION_LIMIT, sizeof(int));
    };
}
