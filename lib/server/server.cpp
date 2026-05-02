// ESP32 MiniWebServer Framework - https://github.com/LemurDaniel/ESP32__MiniWebServer-Framework
// Copyright © 2026, Daniel Landau
// MIT License

#include <server.h>

/********** TCP Server Implementation **********/

namespace ESP32WebServer
{
    /**
     ***********************************************
     ************************************************
     * Setting up the MiniServer with Constructor and Destructor:
     * - Create a socket
     * - Bind it to the specified IP and port
     * - Listen for incoming connections
     *
     **/
    MiniServer::MiniServer()
    {
        LittleFS.begin(true);
        _is_running = false;
    }
    MiniServer::~MiniServer()
    {
        closeServer();
    }

    void MiniServer::closeServer()
    {
        close(_server_socket);
    }

    void MiniServer::connectWiFi(const std::string &ssid, const std::string &password)
    {
        WiFiUtility::instance().addWiFiConfig(ssid, password);
    }

    void MiniServer::clearWiFi()
    {
        WiFiUtility::instance().clearWiFiConfig();
    }

    void MiniServer::disableAdmin()
    {
        _is_admin_enabled = false;
    }

    void MiniServer::defaultAdminSalt(std::string &salt)
    {
        TokenManager::instance().DEFAULT_ADMIN_SALT = salt;
    }

    void MiniServer::defaultAdminCredentials(std::string &username, std::string &password)
    {
        TokenManager::instance().DEFAULT_ADMIN_USER = username;
        TokenManager::instance().DEFAULT_ADMIN_PWD = password;
    }

    /*-------------------------------------------------------------------------------------------------
     *
     * Handle client requests
     *
     *
     **/

    void MiniServer::handleClient(int client_socket)
    {
        Response response = Response();

        // Parse the raw HTTP request into a structured Request object
        const Request &request = Request::parse(client_socket);
        if (request.rejected)
        {
            response.status(413).text(request.error);
            Response::send(client_socket, response);
            struct linger sl = {1, 0};
            setsockopt(client_socket, SOL_SOCKET, SO_LINGER, &sl, sizeof(sl));
            return;
        }

        // Search for matching middlewares
        for (const auto &entry : middlewares)
        {
            if (request.path.find(entry.first) != 0)
            {
                continue;
            }

            for (const RequestHandler &handler : entry.second)
            {
                handler(request, response);
                if (response.finalized)
                {
                    return Response::send(client_socket, response);
                }
            }
        }

        // Search for matching route
        std::string routeKey;
        routeKey.reserve(request.method.size() + 1 + request.path.size());
        routeKey = request.method;
        routeKey += ' ';
        routeKey += request.path;

        auto entry = routes.find(routeKey);
        if (entry == routes.end())
        {
            response.NotFound();
            Response::send(client_socket, response);
            Serial.printf("No handler found for route: %s\n", routeKey.c_str());
            return;
        }

        const std::vector<RequestHandler> &route = entry->second;
        for (const RequestHandler &handler : route)
        {
            handler(request, response);
            if (response.finalized)
                break;
        }

        Response::send(client_socket, response);
    }

    /*-------------------------------------------------------------------------------------------------
     *
     * Handle Middleware
     *
     **/

    void MiniServer::use(const std::string &prefix, const RequestHandler &handler)
    {
        auto entry = middlewares.find(prefix);
        if (entry != middlewares.end())
        {
            std::vector<RequestHandler> &list = entry->second;
            list.push_back(handler);
        }
        else
        {
            std::vector<RequestHandler> list{handler};
            middlewares.insert({prefix, list});
        }
    }
    void MiniServer::use(const RequestHandler &handler)
    {
        use("/", handler);
    }

    /*-------------------------------------------------------------------------------------------------
     *
     * Implement Routes
     *
     **/

    void MiniServer::addRoute(const std::string &method, const std::string &path, const std::vector<RequestHandler> &handlers)
    {
        routes.insert({method + " " + path, handlers});
    }
    void MiniServer::addRoute(const std::string &method, const std::string &path, const RequestHandler &handler)
    {
        const std::vector<RequestHandler> wrapper = {handler};
        addRoute(method, path, wrapper);
    }

    void MiniServer::registerRouter(const ESP32WebServer::Router &router)
    {
        for (const auto &route : router.routes)
        {
            Serial.printf("Registering route: %s %s\n", route.method.c_str(), route.path.c_str());
            addRoute(route.method, route.path, route.handler);
        }

        for (const auto &entry : router.middlewares)
        {
            Serial.printf("Registering Middelware for Prefix: %s\n", entry.first.c_str());
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

    /*-------------------------------------------------------------------------------------------------
     *
     * Serve and handle files:
     *
     **/

    void MiniServer::serveFile(int client_socket, Response &res)
    {

        File file = LittleFS.open(res.filePath.c_str(), "r");

        const std::string header = res.getHeaders();
        write(client_socket, header.c_str(), header.size());

        char chunk[1460]; // TCP MTU-friendly chunk size
        size_t totalSent = 0;

        file.seek(0); // Ensure we start from the beginning
        while (file.available() && totalSent < res.fileSize)
        {
            size_t n = file.readBytes(chunk, sizeof(chunk));
            if (n > 0)
            {
                write(client_socket, chunk, n);
                totalSent += n;
            }
            else
            {
                Serial.println("⚠️  WARNING: Read error while serving file!");
                break;
            }
        }

        file.close();
        Serial.printf("✅ File transfer completed: %zu bytes sent\n", totalSent);
    }

    void MiniServer::staticFile(const std::string &path, const std::string &file_path)
    {
        Serial.printf("Adding file response: %s -> %s\n", path.c_str(), file_path.c_str());

        const RequestHandler &handler = [file_path](const ESP32WebServer::Request &req, ESP32WebServer::Response &res)
        {
            std::string ext;
            auto dot = file_path.find_last_of('.');
            if (dot != std::string::npos)
            {
                ext = file_path.substr(dot);
            }

            res.file(file_path);

            if (ext == ".css")
                res.header("Content-Type", "text/css");
            else if (ext == ".html")
                res.header("Content-Type", "text/html; charset=utf-8");
        };
        addRoute("GET", path, handler);
    }

    void MiniServer::root(const std::string &folder_path)
    {
        std::vector<FileInfo> files = listFiles(folder_path);
        for (FileInfo info : files)
        {
            staticFile("/" + info.name, info.path);
        }
    }

    void MiniServer::index(const std::string &index_path)
    {
        staticFile("/", index_path);
        staticFile("/index", index_path);
        staticFile("/index.html", index_path);
    }

    /*-------------------------------------------------------------------------------------------------
     *
     * Handle multiple connections and cleanup:
     *
     **/
    void MiniServer::workerTask(void *param)
    {

        MiniServer *server = static_cast<MiniServer *>(param);
        int client_socket;

        while (true)
        {
            // Hier wartet der Task, verbraucht 0% CPU währenddessen
            if (xQueueReceive(server->_handleQueue, &client_socket, portMAX_DELAY))
            {
                // --- TIMEOUT SETUP START ---
                struct timeval tv;
                tv.tv_sec = 5;
                tv.tv_usec = 0;

                setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
                setsockopt(client_socket, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
                // --- TIMEOUT SETUP END ---
                Serial.println("--------------------------------------------------------------------");
                Serial.printf("Worker handling client on socket %d\n", client_socket);

                server->handleClient(client_socket);

                Serial.printf("Worker finished handling client on socket %d\n", client_socket);
                Serial.println("--------------------------------------------------------------------");
                shutdown(client_socket, SHUT_RDWR);
                close(client_socket);
            }
        }
    }

    void MiniServer::acceptClientTask(void *param)
    {
        MiniServer *server = static_cast<MiniServer *>(param);
        const int server_socket = server->_server_socket;

        while (true)
        {
            struct sockaddr_in client_addr;
            socklen_t len = sizeof(client_addr);

            int client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &len);

            if (client_socket < 0)
            {
                Serial.println("Failed to accept client connection");
                return;
            }

            Serial.printf("Accepted new client on socket %d\n", client_socket);

            if (xQueueSend(server->_handleQueue, &client_socket, 0) != pdTRUE)
            {
                Serial.println("Queue full! Rejecting new client.");
                write(client_socket, "HTTP/1.1 503 Service Unavailable\r\nContent-Length: 19\r\n\r\nService Unavailable", 75);
                close(client_socket);
            }
        }
    }

    int MiniServer::start(int port, const std::string &ip_addr)
    {

        if (_is_running)
        {
            Serial.println("Server is already running");
            return 0;
        }

        clearFolder(TEMP_FOLDER);

        if (_is_admin_enabled)
        {
            this->registerRouter(ESP32WebServer::AdminRouter());
        }

        if (!WiFiUtility::instance().isNetworkReady())
        {
            WiFiUtility::instance().setup();
        }

        memset(&_address, 0, sizeof(_address));
        _address.sin_family = AF_INET;
        _address.sin_port = htons(port);
        _address_len = sizeof(_address);
        if (inet_pton(AF_INET, ip_addr.c_str(), &_address.sin_addr) <= 0)
        {
            Serial.println("Invalid IP address");
            return 1;
        }

        _server_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (_server_socket < 0)
        {
            Serial.println("Failed to create socket");
            return 1;
        }

        int opt = 1;
        if (setsockopt(_server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
        {
            Serial.println("Failed to set socket options");
            return 1;
        }

        if (bind(_server_socket, (struct sockaddr *)&_address, sizeof(_address)) < 0)
        {
            Serial.println("Failed to bind socket");
            return 1;
        }

        if (listen(_server_socket, 3) < 0)
        {
            Serial.println("Failed to listen on socket");
            return 1;
        }

        Serial.println("Starting worker tasks...");
        for (int i = 0; i < ESP32WebServer::WORKER_TASK_COUNT; i++)
        {
            std::string taskName = "worker" + std::to_string(i);
            xTaskCreatePinnedToCore(workerTask, taskName.c_str(), 8192, this, 1, NULL, i % 2);
        }

        Serial.println("Starting accept client task...");
        xTaskCreatePinnedToCore(acceptClientTask, "accept", 8192, this, 2, NULL, 0);

        Serial.println("Starting WiFi manager task");
        xTaskCreate(WiFiUtility::wifiManagerTask, "WiFiManager", 4096, nullptr, 1, nullptr);

        Serial.println("Server started and listening for clients...");
        _is_running = true;
        return 0;
    }

}