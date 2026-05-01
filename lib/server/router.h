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
    using RequestHandler = std::function<void(const Request &, Response &)>;

    class Router
    {
    public:
        std::map<std::string, std::vector<RequestHandler>> middlewares;

        struct Route
        {
            std::string method;
            std::string path;
            std::vector<RequestHandler> handler;
        };
        std::vector<Route> routes;

        Router() {};

        void route(const std::string &method, const std::string &path, std::vector<RequestHandler> handlers)
        {
            routes.push_back({method, path, handlers});
        }

        void route(const std::string &method, const std::string &path, RequestHandler handler)
        {
            route(method, path, std::vector<RequestHandler>{handler});
        }

        void use(const std::string &prefix, const RequestHandler &handler)
        {
            const auto &entry = middlewares.find(prefix);
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

    private:
    };

}