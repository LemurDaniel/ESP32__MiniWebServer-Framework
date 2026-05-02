// ESP32 MiniWebServer Framework - https://github.com/LemurDaniel/ESP32__MiniWebServer-Framework
// Copyright © 2026, Daniel Landau
// MIT License

#pragma once

#include <Arduino.h>
#include <stdlib.h>
#include <Update.h>

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

    inline void post_UpdateWebsite(Request &req, Response &res)
    {
        Serial.print("File located at: ");
        Serial.println(req.filePath.c_str());
    }

    inline void post_UpdaterCode(Request &req, Response &res)
    {
        size_t updateSize = req.contentLength;

        if (Update.begin(req.contentLength))
        {
            uint8_t buff[1024];
            int available = 0;
            do
            {
                available = req.readBodyAsChunks(buff, sizeof(buff));
                if (available > 0)
                    Update.write(buff, available);

            } while (available > 0);

            if (Update.end() && Update.isFinished())
            {
                post_AdminRestart(req, res);
            }
            else
            {
                Serial.println(Update.getError());
            }
        }
    }

    class UpdateRouter : public ESP32WebServer::Router
    {
    public:
        UpdateRouter()
        {
            // use("/web", auth_handler);

            route("POST", "/code", post_UpdaterCode);
        }
    };
}