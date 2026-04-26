

#include <Arduino.h>
#include <WiFi.h>

#include <../lib/server/router.h>

namespace routes_server_manager
{

    class Router : public ESP32WebServer::Router
    {
    public:
        Router()
        {
            route("GET", "/server/shutdown", get_shutdown);
            route("GET", "/server/startup", get_startup);
            route("GET", "/server/status", get_status);
        }

    private:
        static int isShuttingDown;
        static int isPoweringUp;

        static void get_shutdown(const ESP32WebServer::Request &req, ESP32WebServer::Response &res);
        static void get_startup(const ESP32WebServer::Request &req, ESP32WebServer::Response &res);
        static void get_status(const ESP32WebServer::Request &req, ESP32WebServer::Response &res);
    };

}