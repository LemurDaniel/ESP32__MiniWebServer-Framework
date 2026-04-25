

#include <Arduino.h>
#include <WiFi.h>

#include <../lib/server/router.h>

namespace routes_example
{
    class Router : public ESP32WebServer::Router
    {
    public:
        Router()
        {
            add("GET", "/hello", get_hello);
            add("GET", "/status", get_status);
            add("GET", "/example", get_example);
            add("POST", "/data", post_data);
        }

    private:
        static void get_hello(const ESP32WebServer::Request &req, ESP32WebServer::Response &res);
        static void get_status(const ESP32WebServer::Request &req, ESP32WebServer::Response &res);
        static void get_example(const ESP32WebServer::Request &req, ESP32WebServer::Response &res);
        static void post_data(const ESP32WebServer::Request &req, ESP32WebServer::Response &res);
    };

}