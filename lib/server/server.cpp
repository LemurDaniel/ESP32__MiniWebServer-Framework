
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
    MiniServer::MiniServer(const std::string &ip_addr, int port)
    {
        is_running = false; 
        
        memset(&address, 0, sizeof(address));
        address.sin_family = AF_INET;
        if (inet_pton(AF_INET, ip_addr.c_str(), &address.sin_addr) <= 0)
        {
            Serial.println("Invalid IP address");
            return;
        }
        address.sin_port = htons(port);
        address_len = sizeof(address);
    }
    MiniServer::~MiniServer()
    {
        closeServer();
    }

    void MiniServer::closeServer()
    {
        close(server_socket);
    }

    int MiniServer::startServer()
    {
        server_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (server_socket < 0)
        {
            Serial.println("Failed to create socket");
            return 1;
        }

        int opt = 1;
        if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
        {
            Serial.println("Failed to set socket options");
            return 1;
        }

        if (bind(server_socket, (struct sockaddr *)&address, sizeof(address)) < 0)
        {
            Serial.println("Failed to bind socket");
            return 1;
        }

        if (listen(server_socket, 3) < 0)
        {
            Serial.println("Failed to listen on socket");
            return 1;
        }

        return 0;
    }

    /************************************************
     ************************************************
     * Listening for Clients:
     * - Listen for incoming connections
     * - Accept the connection and get a new socket for communication
     * - Handle the client requests (index serving, GET/POST handling, etc.)
     *
     **/
    void MiniServer::listenClient()
    {
        if(!is_running)
        {
            if (startServer() != 0)
            {
                Serial.println("Failed to start server");
                return;
            }
            is_running = true;
            Serial.println("Server started and listening for clients...");
        }

        struct sockaddr_in client_addr;
        socklen_t len = sizeof(client_addr);

        int client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &len);

        if (client_socket < 0)
        {
            return;
        }

        Serial.printf("Client connected: %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        handleClient(client_socket);
    }

    void MiniServer::handleClient(int client_socket)
    {
        char buffer[1024];

        int bytesRead = read(client_socket, buffer, sizeof(buffer) - 1);

        if (bytesRead <= 0)
        {
            close(client_socket);
            return;
        }

        buffer[bytesRead] = '\0'; // Null-terminate the buffer to make it a valid C-string

        Serial.printf("\n\n*******************************\n");
        Serial.printf("Received request:\n%s", buffer);
        Serial.printf("*******************************\n\n");

        Request request = Request::parse(buffer);
        Response response = Response();

        // Simple serving of a static file if path matches, otherwise look for dynamic route handlers
        if (file_responses.find(request.path) != file_responses.end())
        {
            serveFile(client_socket, request.path);
            close(client_socket);
            return;
        }

        // Search for a matching route handler
        const std::string routeKey = request.method + " " + request.path;
        if (routes.find(routeKey) != routes.end())
        {
            // Retrieve the handler function for the matched route and execute it
            const auto route = routes[routeKey];
            route(request, response);

            std::string header = "HTTP/1.1 " + std::to_string(response.status_code) + " OK\r\n" +
                                 "Content-Type: " + response.contentType + "\r\n" +
                                 "Content-Length: " + std::to_string(response.body.size()) + "\r\n" +
                                 "Connection: close\r\n\r\n";

            for (const auto &pair : response.headers)
            {
                header += pair.first + ": " + pair.second + "\r\n";
            }

            write(client_socket, header.c_str(), header.size());
            write(client_socket, response.body.c_str(), response.body.size());
            close(client_socket);

            return;
        }

        Serial.printf("No route found for path: %s\n", request.path.c_str());
        close(client_socket);
    }

    /************************************************
     ************************************************
     * Serve index or handle GET/POST requests:
     *
     **/
    void MiniServer::addFile(const std::string &path, const std::string &file_path)
    {
        Serial.printf("Adding file response: %s -> %s\n", path.c_str(), file_path.c_str());
        file_responses.insert({path, file_path});
    }

    void MiniServer::index(const std::string &index_path)
    {
        addFile("/", index_path);
        addFile("/index", index_path);
        addFile("/index.html", index_path);
    }

    void MiniServer::serveFile(int client_socket, const std::string &path)
    {

        if (!LittleFS.begin())
        {
            Serial.println("An Error has occurred while mounting LittleFS");
            return;
        }

        Serial.printf("Serving file: %s\n", path.c_str());

        File file = LittleFS.open(path.c_str(), "r");
        if (!file)
        {
            Serial.println("Failed to open file for reading");

            const char *body = "Not Found";
            const char *header =
                "HTTP/1.1 404 Not Found\r\n"
                "Content-Type: text/plain\r\n"
                "Content-Length: 9\r\n"
                "Connection: close\r\n\r\n";

            write(client_socket, header, strlen(header));
            write(client_socket, body, 9);
            return;
        }

        char header[256];
        snprintf(header, sizeof(header),
                 "HTTP/1.1 200 OK\r\n"
                 "Content-Type: text/html\r\n"
                 "Content-Length: %zu\r\n"
                 "Connection: close\r\n\r\n",
                 file.size());

        write(client_socket, header, strlen(header));

        char chunk[512];
        size_t n;

        while ((n = file.readBytes(chunk, sizeof(chunk))) > 0)
        {
            write(client_socket, chunk, n);
        }

        file.close();
    }

    void MiniServer::addRoute(const std::string &method, const std::string &path, void (*handler)(const Request &req, Response &res))
    {
        routes.insert({method + " " + path, handler});
    }

    void MiniServer::add(const std::string &method, const std::string &path, void (*handler)(const Request &req, Response &res))
    {
        addRoute(method, path, handler);
    }

    void MiniServer::get(const std::string &path, void (*handler)(const Request &req, Response &res))
    {
        addRoute("GET", path, handler);
    }

    void MiniServer::post(const std::string &path, void (*handler)(const Request &req, Response &res))
    {
        addRoute("POST", path, handler);
    }

    void MiniServer::put(const std::string &path, void (*handler)(const Request &req, Response &res))
    {
        addRoute("PUT", path, handler);
    }

    void MiniServer::registerRouter(const ESP32WebServer::Router &router)
    {
        for (const auto &route : router.routes)
        {
            Serial.printf("Registering route: %s %s\n", route.method.c_str(), route.path.c_str());
            addRoute(route.method, route.path, route.handler);
        }
    }

}