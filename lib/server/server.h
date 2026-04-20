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
#include <router.h>

namespace ESP32WebServer
{

    const int BUFFER_SIZE = 30720;

    class MiniServer
    {
    public:
        MiniServer(const std::string &ip_addr, int port);
        ~MiniServer();

        // Connect to WiFi network via SSID (Name of WiFi) and password
        WiFiClass connectWiFi(const std::string &ssid, const std::string &password);

        // This is a blocking call that listens for incoming client connections and handles them
        // May be executed on a different Thread or Core to avoid blocking the main loop
        void listenClient();

        // Serve a static file as index.html on the root path
        void index(const std::string &index_path);

        // Add a static file response for a specific path
        void addFile(const std::string &path, const std::string &file_path);

        // Register routes with method, path and handler function
        void add(const std::string &method, const std::string &path, void (*handler)(const Request &req, Response &res));
        void get(const std::string &path, void (*handler)(const Request &req, Response &res));
        void post(const std::string &path, void (*handler)(const Request &req, Response &res));
        void put(const std::string &path, void (*handler)(const Request &req, Response &res));
        void registerRouter(const ESP32WebServer::Router &router);

    private:
        struct sockaddr_in address;
        unsigned int address_len;
        int server_socket;

        void closeServer();
        int startServer();
        int is_running = false;

        void handleClient(int client_socket);

        // Map of path to file path for static file serving
        std::map<std::string, std::string> file_responses;
        void serveFile(int client_socket, Response &res);

        // Map of "METHOD PATH" to handler function for dynamic routes
        std::map<std::string, void (*)(const Request &, Response &)> routes;
        void addRoute(const std::string &method, const std::string &path, void (*handler)(const Request &req, Response &res));
    };
}
#endif