// ESP32 MiniWebServer Framework - https://github.com/LemurDaniel/ESP32__MiniWebServer-Framework
// Copyright © 2026, Daniel Landau
// MIT License

#pragma once

#ifndef INCLUDED_HTTP_TCPSERVER_LINUX
#define INCLUDED_HTTP_TCPSERVER_LINUX

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <WiFi.h>
#include <Arduino.h>
#include <LittleFS.h>

#include <arpa/inet.h>
#include <sys/socket.h>

#include <iostream>

#include <vector>
#include <string>
#include <map>

#include <ArduinoJson-v7.4.3.h>

namespace ESP32WebServer
{
    class Request
    {
    public:
        std::string method;
        std::string path;
        std::map<std::string, std::string> headers;
        std::string body;
        JsonDocument jsonBody; // Parsed JSON body (if applicable)

        static Request parse(std::string requestRaw)
        {
            // fetch first line of request: "GET /path HTTP/1.1"
            std::string requestLine = requestRaw.substr(0, requestRaw.find("\r\n"));

            // split request line into method and path
            size_t firstSpace = requestLine.find(' ');
            size_t secondSpace = requestLine.find(' ', firstSpace + 1);

            std::string method = requestLine.substr(0, firstSpace);
            std::string path = requestLine.substr(firstSpace + 1, secondSpace - firstSpace - 1);

            // --- Extract headers ---
            std::map<std::string, std::string> headers = {};
            size_t pos = requestRaw.find("\r\n") + 2; // Start after the request line
            while (true)
            {
                size_t endOfLine = requestRaw.find("\r\n", pos);
                if (endOfLine == std::string::npos || endOfLine == pos)
                    break; // End of headers

                std::string headerLine = requestRaw.substr(pos, endOfLine - pos);
                size_t colonPos = headerLine.find(':');
                if (colonPos != std::string::npos)
                {
                    std::string key = headerLine.substr(0, colonPos);
                    std::string value = headerLine.substr(colonPos + 1);
                    // Trim whitespace
                    key.erase(0, key.find_first_not_of(" \t"));
                    key.erase(key.find_last_not_of(" \t") + 1);
                    value.erase(0, value.find_first_not_of(" \t"));
                    value.erase(value.find_last_not_of(" \t") + 1);

                    headers[key] = value;
                }
                pos = endOfLine + 2;
            }

            // --- Process Request Headers and Body ---
            Request request;
            request.method = method;
            request.path = path;
            request.body = "";
            request.headers = headers;

            // Body extrahieren (falls vorhanden)
            size_t headerEnd = requestRaw.find("\r\n\r\n");
            if (headerEnd != std::string::npos)
            {
                request.body = requestRaw.substr(headerEnd + 4);
                if (
                    request.headers.find("Content-Type") != request.headers.end() &&
                    request.headers["Content-Type"] == "application/json")
                {
                    request.body = requestRaw.substr(headerEnd + 4);
                    deserializeJson(request.jsonBody, request.body);
                }
            }

            return request;
        }

    private:
    };

    class Response
    {
    public:
        std::string responseMode = "body"; // "body" or "file"

        // HTTP status code for the response (e.g., 200 for OK, 404 for Not Found)
        int status_code = 200;

        // Custom headers to include in the response
        std::map<std::string, std::string> headers;

        // Default content type is text/plain, but can be set to application/json or others as needed
        std::string body;

        // For static file responses, this will be set to the file path to serve
        size_t fileSize;
        std::string filePath;

        std::string header(const std::string &key, const std::string &value)
        {
            headers[key] = value;
            return value;
        }

        std::string getHeaders()
        {

            if (responseMode == "file")
            {
                this->header("Content-Length", std::to_string(fileSize));
            }
            else
            {
                this->header("Content-Length", std::to_string(body.size()));
            }

            std::string header = "HTTP/1.1 " + std::to_string(status_code) + "\r\n";

            for (const auto &h : headers)
            {
                header += h.first + ": " + h.second + "\r\n";
            }

            header += "Connection: close\r\n\r\n";

            return header;
        }

        /**
         **************************************************
         **************************************************
         * Common status codes:
         * 200 - OK
         * 404 - Not Found
         * 500 - Internal Server Error
         */

        Response OK()
        {
            if (this->body.empty())
            {
                this->body = "OK";
            }
            this->status_code = 200;
            return *this;
        }

        Response NotFound()
        {
            if (this->body.empty())
            {
                this->body = "Not Found";
            }
            this->status_code = 404;
            return *this;
        }

        Response InternalServerError()
        {
            if (this->body.empty())
            {
                this->body = "Internal Server Error";
            }
            this->status_code = 500;
            return *this;
        }

        Response status(int status)
        {
            this->status_code = status;
            return *this;
        }

        /**
         **************************************************
         **************************************************
         * Response body helpers for different content types
         *
         */
        Response file(const std::string &path)
        {
            this->binaryFile(path);

            // Set content to text, so browser displays instead of downloading.
            this->header("Content-Type", "text/html; charset=utf-8;");

            return *this;
        }

        Response binaryFile(const std::string &path)
        {
            // Check if LittleFS is already mounted, if not, mount it
            if (!LittleFS.begin(true)) // Format if mount fails
            {
                Serial.println("❌ CRITICAL: LittleFS mount failed completely!");
                this->InternalServerError().text("Internal Server Error: Filesystem Unavailable");
                return *this;
            }

            // Try to open the requested file
            File file = LittleFS.open(path.c_str(), "r");
            if (!file)
            {
                Serial.println("❌ File not found - sending 404");
                this->NotFound().text("File Not Found");
                file.close();
                return *this;
            }

            // Check file size and existence
            if (file.size() == 0)
            {
                Serial.println("⚠️  WARNING: File size is 0 bytes!");
            }

            this->header("Content-Type", "application/octet-stream"); // Set content type for binary file
            this->fileSize = file.size();
            this->filePath = path;
            this->responseMode = "file";

            Serial.printf("✅ File '%s' is ready to be served (size: %d bytes)\n", path.c_str(), this->fileSize);

            file.close();
            return *this;
        }

        Response text(const std::string &text)
        {
            this->body = text;
            this->responseMode = "body";
            this->header("Content-Type", "text/plain");
            return *this;
        }

        Response json(JsonDocument jsonBody)
        {
            char body[512];
            serializeJson(jsonBody, body, sizeof(body));

            this->body = body;
            this->responseMode = "body";
            this->header("Content-Type", "application/json");
            return *this;
        }

    private:
    };

    class Router
    {
    public:
        struct Route
        {
            std::string method;
            std::string path;
            void (*handler)(const ESP32WebServer::Request &, ESP32WebServer::Response &);
        };
        std::vector<Route> routes;

        Router() {};

        void add(const std::string &method, const std::string &path, void (*handler)(const ESP32WebServer::Request &, ESP32WebServer::Response &))
        {
            routes.push_back({method, path, handler});
        }

    private:
    };

    const int BUFFER_SIZE = 30720;

    class MiniServer
    {
    public:
        MiniServer(const std::string &ip_addr, int port)
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
        ~MiniServer()
        {
            closeServer();
        }

        // Connect to WiFi network via SSID (Name of WiFi) and password
        WiFiClass connectWiFi(const std::string &ssid, const std::string &password)
        {
            WiFi.mode(WIFI_STA);
            WiFi.begin(ssid.c_str(), password.c_str());

            Serial.println();
            Serial.print("Connecting to WiFi...");
            while (WiFi.status() != WL_CONNECTED)
            {
                delay(500);
                Serial.print(".");
            }

            Serial.println("Connected!");
            Serial.printf("IP Address: %s\n", WiFi.localIP().toString().c_str());
            Serial.printf("Signal Strength: %d dBm\n", WiFi.RSSI());
            Serial.println();

            return WiFi;
        }

        // This is a blocking call that listens for incoming client connections and handles them
        // May be executed on a different Thread or Core to avoid blocking the main loop
        void listenClient()
        {
            if (!is_running)
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

        // Serve a static file as index.html on the root path
        void index(const std::string &index_path)
        {
            addFile("/", index_path);
            addFile("/index", index_path);
            addFile("/index.html", index_path);
        }

        // Add a static file response for a specific path
        void addFile(const std::string &path, const std::string &file_path)
        {
            Serial.printf("Adding file response: %s -> %s\n", path.c_str(), file_path.c_str());
            file_responses.insert({path, file_path});
        }

        // Register routes with method, path and handler function
        void add(const std::string &method, const std::string &path, void (*handler)(const Request &req, Response &res))
        {
            addRoute(method, path, handler);
        }

        void get(const std::string &path, void (*handler)(const Request &req, Response &res))
        {
            addRoute("GET", path, handler);
        }

        void post(const std::string &path, void (*handler)(const Request &req, Response &res))
        {
            addRoute("POST", path, handler);
        }

        void put(const std::string &path, void (*handler)(const Request &req, Response &res))
        {
            addRoute("PUT", path, handler);
        }

        void registerRouter(const ESP32WebServer::Router &router)
        {
            for (const auto &route : router.routes)
            {
                Serial.printf("Registering route: %s %s\n", route.method.c_str(), route.path.c_str());
                addRoute(route.method, route.path, route.handler);
            }
        }

    private:
        struct sockaddr_in address;
        unsigned int address_len;
        int server_socket;

        void closeServer()
        {
            close(server_socket);
        }
        int startServer()
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
        };
        int is_running = false;

        void handleClient(int client_socket)
        {
            Serial.println("\n\n\nHandling client request...");

            char buffer[1024];

            int bytesRead = read(client_socket, buffer, sizeof(buffer) - 1);

            if (bytesRead <= 0)
            {
                close(client_socket);
                return;
            }

            buffer[bytesRead] = '\0'; // Null-terminate the buffer to make it a valid C-string

            // Parse the raw HTTP request into a structured Request object
            Request request = Request::parse(buffer);
            Response response = Response();

            // Simple serving of a static file if path matches, otherwise look for dynamic route handlers
            if (file_responses.find(request.path) != file_responses.end())
            {
                response.file(file_responses[request.path]);
                serveFile(client_socket, response);
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

                if (response.responseMode == "file")
                {
                    serveFile(client_socket, response);
                }
                else
                {
                    std::string header = response.getHeaders();
                    write(client_socket, header.c_str(), header.size());
                    write(client_socket, response.body.c_str(), response.body.size());
                }

                close(client_socket);

                return;
            }

            Serial.printf("No route found for path: %s\n", request.path.c_str());
            close(client_socket);
        }

        // Map of path to file path for static file serving
        std::map<std::string, std::string> file_responses;
        void serveFile(int client_socket, Response &res)
        {

            File file = LittleFS.open(res.filePath.c_str(), "r");

            const std::string header = res.getHeaders();
            write(client_socket, header.c_str(), header.size());

            char chunk[512];
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

        // Map of "METHOD PATH" to handler function for dynamic routes
        std::map<std::string, void (*)(const Request &, Response &)> routes;
        void addRoute(const std::string &method, const std::string &path, void (*handler)(const Request &req, Response &res))
    {
        routes.insert({method + " " + path, handler});
    }
    };
}
#endif