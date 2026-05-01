// ESP32 MiniWebServer Framework - https://github.com/LemurDaniel/ESP32__MiniWebServer-Framework
// Copyright © 2026, Daniel Landau
// MIT License

#pragma once

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

#include <utility.file.h>
#include <utility.wifi.h>
#include <utility.admin.h>
#include <router.h>

namespace ESP32WebServer
{

    const int WORKER_TASK_COUNT = 4;
    const int CONNECTION_LIMIT = 10;
    const int CONNECTION_TIMEOUT_SEC = 5;

    class MiniServer
    {
    public:
        MiniServer();
        ~MiniServer();

        int start(const int port, const std::string &ip_addr = "0.0.0.0");

        // Connect to WiFi network via SSID (Name of WiFi) and password
        // If not used, the server will start in AP mode with SSID "ESP32_MiniWebServer" and a default admin page for WiFi configuration
        void connectWiFi(const std::string &ssid, const std::string &password);
        void clearWiFi();

        // Serve a static file as index.html on the root path
        void index(const std::string &index_path);

        // Serve all files in there as root;
        void root(const std::string &folder_path);

        // Add a static file response for a specific path
        void staticFile(const std::string &path, const std::string &file_path);

        // Register routes with method, path and handler function
        void registerRouter(const ESP32WebServer::Router &router);
        void route(const std::string &method, const std::string &path, const RequestHandler &handler);

        // Quick functions
        void get(const std::string &path, const RequestHandler &handler);
        void post(const std::string &path, const RequestHandler &handler);
        void put(const std::string &path, const RequestHandler &handler);
        void patch(const std::string &path, const RequestHandler &handler);
        // delete is a keyword in c++, hence using del
        void del(const std::string &path, const RequestHandler &handler);

        // Add default middleware handler
        void use(const RequestHandler &handler);
        void use(const std::string &prefix, const RequestHandler &handler);

    private:
        struct sockaddr_in _address;
        unsigned int _address_len;
        int _server_socket;
        void closeServer();

        int _is_running = false;
        int _is_admin_enabled = true;
        // Disables the admin dashboard entirly
        void disableAdmin();
        // Overrides the default admin credentials
        void defaultAdminSalt(std::string &salt);
        void defaultAdminCredentials(std::string &username, std::string &password);

        void processHandlers(const Request &req, Response &res);
        void handleClient(int client_socket);

        // Map of path to file path for static file serving
        void serveFile(int client_socket, Response &res);

        // Map of "METHOD PATH" to handler function for dynamic routes
        std::map<std::string, std::vector<RequestHandler>> routes;
        std::map<std::string, std::vector<RequestHandler>> middlewares;
        void addRoute(const std::string &method, const std::string &path, const RequestHandler &handler);
        void addRoute(const std::string &method, const std::string &path, const std::vector<RequestHandler> &handlers);

        // Queue for incoming connections to be processed by worker threads
        QueueHandle_t _handleQueue = xQueueCreate(CONNECTION_LIMIT, sizeof(int));
        static void workerTask(void *param);
        static void acceptClientTask(void *param);
    };
}