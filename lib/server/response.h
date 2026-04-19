#pragma once

#include <ArduinoJson-v7.4.3.h>
#include <string>
#include <map>

namespace ESP32WebServer
{
    class Response
    {
    public:
        int status_code = 200;
        std::map<std::string, std::string> headers;
        std::string contentType = "text/plain";
        std::string body;

        Response status(int status)
        {
            this->status_code = status;
            return *this;
        }

        Response text(const std::string &text)
        {
            this->body = text;
            this->contentType = "text/plain";
            return *this;
        }

        Response json(JsonDocument jsonBody)
        {
            char body[512];
            serializeJson(jsonBody, body, sizeof(body));

            this->body = body;
            this->contentType = "application/json";
            return *this;
        }

    private:
    };
}