// ESP32 MiniWebServer Framework - https://github.com/LemurDaniel/ESP32__MiniWebServer-Framework
// Copyright © 2026, Daniel Landau
// MIT License

#pragma once

#include <vector>
#include <string>

#include <request.h>
#include <response.h>

namespace ESP32WebServer
{
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

}