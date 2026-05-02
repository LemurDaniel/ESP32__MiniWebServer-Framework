// ESP32 MiniWebServer Framework - https://github.com/LemurDaniel/ESP32__MiniWebServer-Framework
// Copyright © 2026, Daniel Landau
// MIT License

// This is work in progress!

#pragma once

#include <Arduino.h>
#include <stdlib.h>
#include "mbedtls/sha256.h"

#include <vector>
#include <string>

#include <router.h>
#include <utility.wifi.h>

#include <utility.admin.h>

namespace ESP32WebServer
{
    /*-------------------------------------------------------------------------------------------------
     *
     * Update website via remote zip package
     *
     **/

    inline void post_WebsiteUpdate(Request const &req, Response &res)
    {

        Serial.print("File located at: ");
        Serial.println(req.filePath.c_str());
    }

    class AdminRouter : public ESP32WebServer::Router
    {
    public:
        AdminRouter()
        {
            // use("/web", auth_handler);

            route("POST", "/web", post_WebsiteUpdate);
        }
    };
}