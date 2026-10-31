// ESP32 MiniWebServer Framework - https://github.com/LemurDaniel/ESP32__MiniWebServer-Framework
// Copyright © 2026, Daniel Landau
// MIT License

// NOTE:
//   Implementation of MiniServer.h.

//
//   Chapters:
//     HELPERS            Task settings, path normalization and the handler pipeline.
//     LIFECYCLE          Constructor, destructor and the server socket.
//     NETWORK            WiFi credentials and the mDNS name.
//     ADMIN              Credentials, links, token actions and the admin switches.
//     STATIC FILES       Single files, the index page and whole folders.
//     ROUTING            Routers and routes per HTTP method.
//     MIDDLEWARE         Prefix middlewares.
//     REQUEST HANDLING   Accept task, worker tasks and the handler pipeline.
//     START SERVER       Registers the defaults, opens the socket and starts the tasks.
//

#include <cerrno>

#include <MiniServer.h>
#include <utils/utility.log.h>

namespace EspWeb
{
    // ===========================================================================
    // ===============================   HELPERS   ===============================
    // ===========================================================================
    //
    //   TASK_*                FreeRTOS stack sizes and priorities.
    //   RESPONSE_UNAVAILABLE  Raw 503 answer when the worker queue is full.
    //   normalizePath         Prefixes a path with "/" when missing.
    //   runHandlers           Runs a handler chain until one finalizes the response.
    //

    namespace
    {
        constexpr uint32_t TASK_STACK_WORKER = 8192;
        constexpr uint32_t TASK_STACK_ACCEPT = 8192;
        constexpr uint32_t TASK_STACK_WIFI = 4096;

        // The accept task runs above the workers, so new clients are queued while workers are busy.
        constexpr UBaseType_t TASK_PRIORITY_WORKER = 1;
        constexpr UBaseType_t TASK_PRIORITY_ACCEPT = 2;
        constexpr UBaseType_t TASK_PRIORITY_WIFI = 1;

        /** @brief Pause after a failed accept(), e.g. when all sockets are in use. */
        constexpr uint32_t ACCEPT_RETRY_DELAY_MS = 100;

        constexpr char RESPONSE_UNAVAILABLE[] = "HTTP/1.1 503 Service Unavailable\r\n"
                                                "Content-Length: 19\r\n"
                                                "\r\n"
                                                "Service Unavailable";

        // "api"  => "/api"
        // "/api" => "/api"
        // ""     => "/"
        std::string normalizePath(const std::string &path)
        {
            if (!path.empty() && path[0] == '/')
            {
                return path;
            }
            return "/" + path;
        }

        /**
         * @brief Runs a handler chain, stopping at the first handler that finalizes the response.
         * @return `true` if a handler finalized the response.
         */
        bool runHandlers(const std::vector<RequestHandler> &handlers, Request &request, Response &response)
        {
            for (const RequestHandler &handler : handlers)
            {
                handler(request, response);
                if (response.isFinalized())
                {
                    return true;
                }
            }
            return false;
        }
    }

    // ===========================================================================
    // ==============================   LIFECYCLE   ==============================
    // ===========================================================================
    //
    //   MiniServer    Mounts LittleFS, formatting it on first use.
    //   ~MiniServer   Closes the server socket.
    //   closeServer   Closes the server socket.
    //

    MiniServer::MiniServer()
    {
        if (!LittleFS.begin(true))
        {
            ESPWEB_LOG_ERROR("MiniServer", "Failed to mount LittleFS");
        }
    }

    MiniServer::~MiniServer()
    {
        closeServer();
    }

    void MiniServer::closeServer()
    {
        close(_serverSocket);
    }

    // ===========================================================================
    // ===============================   NETWORK   ===============================
    // ===========================================================================
    //
    //   dns           Sets the mDNS name, registered in start.
    //   connectWiFi   Saves a network.
    //   clearWiFi     Removes every saved network.
    //

    void MiniServer::dns(const std::string &dnsName)
    {
        _dnsName = dnsName;
    }

    void MiniServer::connectWiFi(const std::string &ssid, const std::string &password)
    {
        WiFiUtility::instance().addWiFiConfig(ssid, password);
    }

    void MiniServer::clearWiFi()
    {
        WiFiUtility::instance().clearWiFiConfig();
    }

    // ===========================================================================
    // ================================   ADMIN   ================================
    // ===========================================================================
    //
    //   defaultAdminSalt          Delegates to TokenManager::setSalt.
    //   defaultAdminCredentials   Delegates to TokenManager::setCredentials.
    //   setCustomLink             Adds a dashboard link.
    //   setTokenActions           Sets the token actions, "admin" first.
    //   disableAdmin              Disables the admin routes and pages.
    //   disableAdminDashboard     Disables only the admin pages.
    //

    void MiniServer::defaultAdminSalt(const std::string &salt)
    {
        TokenManager::instance().setSalt(salt);
    }

    void MiniServer::defaultAdminCredentials(const std::string &username, const std::string &password)
    {
        TokenManager::instance().setCredentials(username, password);
    }

    void MiniServer::setCustomLink(const std::string &name, const std::string &href)
    {
        CUSTOM_LINKS.insert({name, href});
    }

    void MiniServer::setTokenActions(const std::vector<std::string> &actions)
    {
        TOKEN_ACTIONS = actions;
        TOKEN_ACTIONS.insert(TOKEN_ACTIONS.begin(), "admin");
    }

    void MiniServer::disableAdmin()
    {
        _isAdminEnabled = false;
        disableAdminDashboard();
    }

    void MiniServer::disableAdminDashboard()
    {
        _isDashboardEnabled = false;
    }

    // ===========================================================================
    // ============================   STATIC FILES   =============================
    // ===========================================================================
    //
    //   staticFile   One GET route that serves one file.
    //   root         One static file route per file of a folder, recursively.
    //   index        The index file under "/", "/index" and "/index.html".
    //

    void MiniServer::staticFile(const std::string &path, const std::string &filePath)
    {
        ESPWEB_LOG_DEBUG("MiniServer", "Adding file response: %s -> %s", path.c_str(), filePath.c_str());

        const std::string normalizedPath = normalizePath(filePath);
        addRoute("GET", path,
                 [normalizedPath](Request &req, Response &res)
                 {
                     res.file(normalizedPath);
                 });
    }

    // "/web" with "/web/css/main.css" => GET "/css/main.css"
    void MiniServer::root(const std::string &folderPath, const std::string &prefix)
    {
        _isRootSet = true;
        FOLDER_WEB = folderPath;

        for (const FileInfo &info : fileHandler.listFiles(folderPath))
        {
            if (info.isDirectory)
            {
                root(info.path, prefix + info.name + "/");
            }
            else
            {
                staticFile(prefix + info.name, info.path);
            }
        }
    }

    void MiniServer::index(const std::string &indexPath)
    {
        _isIndexSet = true;
        staticFile("/", indexPath);
        staticFile("/index", indexPath);
        staticFile("/index.html", indexPath);
    }

    // ===========================================================================
    // ===============================   ROUTING   ===============================
    // ===========================================================================
    //
    //   addRoute         Stores a handler chain under "METHOD/path".
    //   registerRouter   Adds every route and middleware of a router.
    //   route            Shortcut with an explicit method.
    //   get ... del      Shortcuts per HTTP method.
    //

    // "GET" + "users/:id" => "GET/users/:id"
    void MiniServer::addRoute(const std::string &method, const std::string &path,
                              const std::vector<RequestHandler> &handlers)
    {
        _routes.add(method + normalizePath(path), handlers);
    }

    void MiniServer::addRoute(const std::string &method, const std::string &path, const RequestHandler &handler)
    {
        addRoute(method, path, std::vector<RequestHandler>{handler});
    }

    void MiniServer::registerRouter(const EspWeb::Router &router)
    {
        for (const auto &route : router.routes)
        {
            ESPWEB_LOG_DEBUG("MiniServer", "Registering route: %s %s", route.method.c_str(), route.path.c_str());
            addRoute(route.method, route.path, route.handler);
        }

        for (const auto &entry : router.middlewares)
        {
            ESPWEB_LOG_DEBUG("MiniServer", "Registering middleware for prefix: %s", entry.first.c_str());
            for (const RequestHandler &handler : entry.second)
            {
                use(entry.first, handler);
            }
        }
    }

    void MiniServer::route(const std::string &method, const std::string &path, const RequestHandler &handler)
    {
        addRoute(method, path, handler);
    }

    void MiniServer::get(const std::string &path, const RequestHandler &handler)
    {
        addRoute("GET", path, handler);
    }
    void MiniServer::post(const std::string &path, const RequestHandler &handler)
    {
        addRoute("POST", path, handler);
    }
    void MiniServer::put(const std::string &path, const RequestHandler &handler)
    {
        addRoute("PUT", path, handler);
    }
    void MiniServer::patch(const std::string &path, const RequestHandler &handler)
    {
        addRoute("PATCH", path, handler);
    }
    void MiniServer::del(const std::string &path, const RequestHandler &handler)
    {
        addRoute("DELETE", path, handler);
    }

    // ===========================================================================
    // =============================   MIDDLEWARE   ==============================
    // ===========================================================================
    //
    //   use   Adds a middleware for a prefix, or for every request.
    //

    void MiniServer::use(const std::string &prefix, const RequestHandler &handler)
    {
        _middlewares.add(normalizePath(prefix), {handler});
    }

    void MiniServer::use(const RequestHandler &handler)
    {
        use("/", handler);
    }

    // ===========================================================================
    // ==========================   REQUEST HANDLING   ===========================
    // ===========================================================================
    //
    //   acceptClientTask   Accepts clients and queues them, 503 when the queue is full.
    //   workerTask         Takes a client from the queue, handles and closes it.
    //   handleClient       Parse, middlewares, route, send.
    //

    void MiniServer::acceptClientTask(void *param)
    {
        MiniServer *server = static_cast<MiniServer *>(param);
        const int serverSocket = server->_serverSocket;

        while (true)
        {
            struct sockaddr_in clientAddr;
            socklen_t len = sizeof(clientAddr);

            const int clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddr, &len);
            if (clientSocket < 0)
            {
                // A FreeRTOS task must never return, so a failed accept() is retried.
                ESPWEB_LOG_WARN("MiniServer", "Failed to accept client connection (errno %d)", errno);
                vTaskDelay(pdMS_TO_TICKS(ACCEPT_RETRY_DELAY_MS));
                continue;
            }

            ESPWEB_LOG_DEBUG("MiniServer", "Accepted new client on socket %d", clientSocket);

            if (xQueueSend(server->_handleQueue, &clientSocket, 0) != pdTRUE)
            {
                ESPWEB_LOG_WARN("MiniServer", "Queue full, rejecting client on socket %d", clientSocket);
                write(clientSocket, RESPONSE_UNAVAILABLE, sizeof(RESPONSE_UNAVAILABLE) - 1);
                close(clientSocket);
            }
        }
    }

    void MiniServer::workerTask(void *param)
    {
        MiniServer *server = static_cast<MiniServer *>(param);
        int clientSocket;

        while (true)
        {
            // Blocks without CPU usage until a client is queued.
            if (!xQueueReceive(server->_handleQueue, &clientSocket, portMAX_DELAY))
            {
                continue;
            }

            struct timeval tv;
            tv.tv_sec = CONNECTION_TIMEOUT_SEC;
            tv.tv_usec = 0;
            setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
            setsockopt(clientSocket, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

            ESPWEB_LOG_DEBUG("MiniServer", "Worker handling client on socket %d", clientSocket);
            server->handleClient(clientSocket);
            ESPWEB_LOG_DEBUG("MiniServer", "Worker finished client on socket %d", clientSocket);

            shutdown(clientSocket, SHUT_RDWR);
            close(clientSocket);
        }
    }

    /*
        > When the request was rejected while parsing
            - Answer 413 and reset the connection, the unread body is dropped.
        > Otherwise
            - Run the middlewares of every matching prefix, stop if one finalizes.
            - Answer 404 when no route matches "METHOD/path".
            - Run the route handlers, stop if one finalizes.
            - Send the response.
    */
    void MiniServer::handleClient(int clientSocket)
    {
        Response response = Response(clientSocket);
        Request request = Request::parse(clientSocket);

        // When the request was rejected while parsing
        if (request.rejected)
        {
            response.status(413).text(request.error).send();
            struct linger sl = {1, 0};
            setsockopt(clientSocket, SOL_SOCKET, SO_LINGER, &sl, sizeof(sl));
            return;
        }

        // Middlewares
        const std::vector<RequestHandler> middlewares = _middlewares.resolve(request.path, request, true);
        ESPWEB_LOG_DEBUG("MiniServer", "Middlewares resolved for %s: %u", request.path.c_str(),
                         (unsigned)middlewares.size());
        if (runHandlers(middlewares, request, response))
        {
            ESPWEB_LOG_DEBUG("MiniServer", "Middleware finalized response for %s", request.path.c_str());
            response.send();
            return;
        }

        // Route
        const std::string routeKey = request.method + request.path;
        const std::vector<RequestHandler> routes = _routes.resolve(routeKey, request, false);
        if (routes.empty())
        {
            ESPWEB_LOG_DEBUG("MiniServer", "No handler found for route: %s", routeKey.c_str());
            response.NotFound().send();
            return;
        }

        runHandlers(routes, request, response);
        response.send();
    }

    // ===========================================================================
    // ============================   START SERVER   =============================
    // ===========================================================================
    //
    //   1. Defaults   Admin routes, the default website and WiFi.
    //   2. Socket     Creates, binds and listens on the server socket.
    //   3. mDNS       Registers the name, again after every reconnect.
    //   4. Tasks      Starts the workers, the accept task and the WiFi manager.
    //

    int MiniServer::start(int port, const std::string &ipAddr, int workerCount)
    {
        if (_isRunning)
        {
            ESPWEB_LOG_WARN("MiniServer", "Server is already running");
            return 0;
        }

        // ---------------------------------------------------------------------------
        // Defaults
        //   Admin routes, the default website and WiFi.
        // ---------------------------------------------------------------------------

        if (_isAdminEnabled)
        {
            registerRouter(AdminRouter(_isDashboardEnabled));
        }

        if (!_isRootSet && !_isIndexSet)
        {
            root(FOLDER_WEB);
            index(FOLDER_WEB + "/index.html");
        }

        if (!WiFiUtility::instance().isNetworkReady())
        {
            WiFiUtility::instance().setup();
        }

        // ---------------------------------------------------------------------------
        // Socket
        //   Creates, binds and listens on the server socket.
        // ---------------------------------------------------------------------------

        memset(&_address, 0, sizeof(_address));
        _address.sin_family = AF_INET;
        _address.sin_port = htons(port);
        _addressLen = sizeof(_address);
        if (inet_pton(AF_INET, ipAddr.c_str(), &_address.sin_addr) <= 0)
        {
            ESPWEB_LOG_ERROR("MiniServer", "Invalid IP address %s", ipAddr.c_str());
            return 1;
        }

        _serverSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (_serverSocket < 0)
        {
            ESPWEB_LOG_ERROR("MiniServer", "Failed to create socket");
            return 1;
        }

        int opt = 1;
        if (setsockopt(_serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
        {
            ESPWEB_LOG_ERROR("MiniServer", "Failed to set socket options");
            return 1;
        }

        if (bind(_serverSocket, (struct sockaddr *)&_address, sizeof(_address)) < 0)
        {
            ESPWEB_LOG_ERROR("MiniServer", "Failed to bind socket to %s:%d", ipAddr.c_str(), port);
            return 1;
        }

        if (listen(_serverSocket, CONNECTION_LIMIT) < 0)
        {
            ESPWEB_LOG_ERROR("MiniServer", "Failed to listen on socket");
            return 1;
        }

        // ---------------------------------------------------------------------------
        // mDNS
        //   Registers the name, again after every reconnect.
        // ---------------------------------------------------------------------------

        if (!_dnsName.empty())
        {
            if (MDNS.begin(_dnsName.c_str()))
            {
                MDNS.addService("http", "tcp", port);
                ESPWEB_LOG_INFO("MiniServer", "mDNS started: http://%s.local", _dnsName.c_str());
            }
            else
            {
                ESPWEB_LOG_WARN("MiniServer", "mDNS failed to start");
            }

            // A new IP needs a new mDNS announcement.
            WiFi.onEvent(
                [this, port](WiFiEvent_t event, WiFiEventInfo_t info)
                {
                    MDNS.end();
                    if (MDNS.begin(_dnsName.c_str()))
                    {
                        MDNS.addService("http", "tcp", port);
                    }
                },
                ARDUINO_EVENT_WIFI_STA_GOT_IP);
        }

        // ---------------------------------------------------------------------------
        // Tasks
        //   Starts the workers, the accept task and the WiFi manager.
        // ---------------------------------------------------------------------------

        // Workers alternate between both cores, the name is copied by FreeRTOS.
        for (int i = 0; i < workerCount; i++)
        {
            const std::string taskName = "worker" + std::to_string(i);
            xTaskCreatePinnedToCore(workerTask, taskName.c_str(), TASK_STACK_WORKER, this, TASK_PRIORITY_WORKER,
                                    nullptr, i % 2);
        }

        xTaskCreatePinnedToCore(acceptClientTask, "accept", TASK_STACK_ACCEPT, this, TASK_PRIORITY_ACCEPT, nullptr, 0);
        xTaskCreate(WiFiUtility::wifiManagerTask, "WiFiManager", TASK_STACK_WIFI, nullptr, TASK_PRIORITY_WIFI, nullptr);

        ESPWEB_LOG_INFO("MiniServer", "Listening on %s:%d with %d workers", ipAddr.c_str(), port, workerCount);
        _isRunning = true;
        return 0;
    }
}
