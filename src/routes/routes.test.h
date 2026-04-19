
#include <Arduino.h>
#include <WiFi.h>

#pragma once
#include <../lib/server/router.h>

namespace routes_test
{
    void get_hello(const ESP32WebServer::Request &req, ESP32WebServer::Response &res)
    {
        res.text("Hello, World! This is a simple response from the ESP32.").status(200);
    }

    void get_status(const ESP32WebServer::Request &req, ESP32WebServer::Response &res)
    {
        JsonDocument status;

        status["device"] = "ESP32";
        status["firmware"] = "1.0.0";
        status["uptime"] = static_cast<double>(millis());
        status["free_heap"] = static_cast<double>(ESP.getFreeHeap());
        status["wifi_rssi"] = static_cast<double>(WiFi.RSSI());

        res.json(status).status(200);
    }

    /**
     ***********************************************
     ************************************************
     * Defining routes for overview and testing
     *
     *
     **/

    ESP32WebServer::Router Router()
    {
        ESP32WebServer::Router router;
        router.add("GET", "/hello", get_hello);
        router.add("GET", "/status", get_status);
        // Weitere Routen hier hinzufügen
        return router;
    }
}