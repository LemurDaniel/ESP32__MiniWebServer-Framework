// ESP32 MiniWebServer Framework - https://github.com/LemurDaniel/ESP32__MiniWebServer-Framework
// Copyright © 2026, Daniel Landau
// MIT License

#pragma once

#include <ArduinoJson-v7.4.3.h>
#include <string>
#include <vector>
#include <map>

namespace ESP32WebServer
{
    class Request
    {
    private:
        static std::string trim(const std::string &s)
        {
            size_t start = s.find_first_not_of(" \t");
            if (start == std::string::npos)
                return {};
            size_t end = s.find_last_not_of(" \t");
            return s.substr(start, end - start + 1);
        }

        static size_t findBytes(const std::vector<uint8_t> &data, const std::string &pattern, size_t start = 0)
        {
            size_t pat_length = pattern.length();
            if (pat_length == 0 || data.size() < pat_length || start > data.size() - pat_length)
                return std::string::npos;

            for (size_t i = start; i <= data.size() - pat_length; ++i)
            {
                if (memcmp(data.data() + i, pattern.c_str(), pat_length) == 0)
                    return i;
            }

            return std::string::npos;
        }

        static std::string extractString(const std::vector<uint8_t> &data, size_t start, size_t end)
        {
            return std::string(reinterpret_cast<const char *>(data.data() + start), end - start);
        }

        static std::vector<std::string> split(const std::string &text, const std::string &splitter)
        {
            std::vector<std::string> elements;

            size_t pos = 0;
            while (pos < text.size())
            {
                size_t end = text.find(splitter, pos);
                if (end == std::string::npos)
                    end = text.size();

                elements.push_back(trim(text.substr(pos, end - pos)));
                pos = end + splitter.size();
            }

            return elements;
        }

        std::string extractBodyAsText(size_t maxSize = std::string::npos) const
        {
            size_t size = (maxSize == std::string::npos) ? bodyRaw.size() : std::min(maxSize, bodyRaw.size());
            return std::string(reinterpret_cast<const char *>(bodyRaw.data()), size);
        }

        std::string readBodyAsText(int clientSocket, size_t maxSize)
        {
            char chunk[256];
            while (bodyRaw.size() < maxSize)
            {
                size_t toRead = std::min((size_t)sizeof(chunk), maxSize - bodyRaw.size());
                int n = read(clientSocket, chunk, toRead);
                if (n <= 0)
                    break;
                bodyRaw.insert(bodyRaw.end(), chunk, chunk + n);
            }
            return extractBodyAsText(maxSize);
        }

        std::vector<std::string> extractHeader(int clientSocket)
        {
            std::vector<std::string> headerRaw;

            char chunk[256];

            std::string header;
            header.reserve(512);

            size_t headerEnd = std::string::npos;

            // ---------------------------------------------------------------------
            // Phase 1: read until \r\n\r\n (complete headers)
            do
            {
                int n = read(clientSocket, chunk, sizeof(chunk));
                if (n <= 0)
                {
                    Serial.println("Failed to read from socket");
                    return {};
                }
                header.append(chunk, n);
                headerEnd = header.find("\r\n\r\n");
            } while (headerEnd == std::string::npos && header.size() < 8192);

            if (headerEnd == std::string::npos)
                return headerRaw;

            // ---------------------------------------------------------------------
            // Phase 2: bytes in headers string are the start of the body;
            std::vector<uint8_t> body(header.begin() + headerEnd + 4, header.end());
            bodyRaw = body;

            // ---------------------------------------------------------------------
            // Phase 3: split headers into list of lines

            header.resize(headerEnd);
            size_t startOfLine = 0;
            while (startOfLine < header.size())
            {
                size_t endOfLine = header.find("\r\n", startOfLine);
                if (endOfLine == std::string::npos)
                {
                    endOfLine = header.size();
                }

                const std::string line = header.substr(startOfLine, endOfLine - startOfLine);
                headerRaw.push_back(line);
                startOfLine = endOfLine + 2;
            }

            return headerRaw;
        }

    public:
        std::string method;
        std::string path;
        std::map<std::string, std::string> headers;
        std::map<std::string, std::string> cookies;

        std::vector<uint8_t> bodyRaw; // Raw body as bytes (binary-safe)

        JsonDocument body; // Parsed JSON body (if Content-Type: application/json)

        static Request parse(int client_socket)
        {
            Request request;

            // --- Extract header ---
            Serial.println("Extracing Raw Header");
            std::vector<std::string> headerRaw = request.extractHeader(client_socket);
            if (headerRaw.empty())
                return request;

            // Split: "GET /path HTTP/1.1"
            std::vector<std::string> firstLine = Request::split(headerRaw[0], " ");
            request.method = firstLine[0];
            request.path = firstLine[1];

            Serial.println("Extracing Headers");
            for (size_t i = 1; i < headerRaw.size(); i++)
            {
                std::vector<std::string> line = Request::split(headerRaw[i], ":");
                if (line.size() >= 2)
                    request.headers[line[0]] = line[1];
            }

            Serial.println("Extracing Cookies");
            if (request.headers.find("Cookie") != request.headers.end())
            {
                std::vector<std::string> cookieHeader = Request::split(request.headers["Cookie"], ";");
                for (size_t i = 0; i < cookieHeader.size(); i++)
                {
                    std::vector<std::string> line = Request::split(cookieHeader[i], "=");
                    if (line.size() >= 2)
                        request.cookies[line[0]] = line[1];
                }
            }

            // --- Extract body ---
             Serial.println("Extracing Body");
            if (request.headers.find("Content-Type") == request.headers.end())
            {
                return request;
            }

            Serial.println("Extracing Body - Content Length");
            size_t contentLength = 0;


            Serial.println(request.headers["Content-Length"].c_str());
            Serial.println(request.headers["Content-Type"].c_str());

            auto clIt = request.headers.find("Content-Length");
            if (clIt != request.headers.end())
            {
                for (char c : clIt->second)
                {
                    if (c < '0' || c > '9') break;
                    contentLength = contentLength * 10 + (c - '0');
                }
            }

            if (contentLength == 0)
            {
                Serial.println("Extracing Body - Content Length is 0");
                return request;
            }

            if (request.headers["Content-Type"].find("application/json") != std::string::npos)
            {
                Serial.println("Extracing Body - Content Json");
                request.readBodyAsText(client_socket, contentLength);
                deserializeJson(request.body, request.bodyRaw);
            }
            else
            {
                request.readBodyAsText(client_socket, contentLength);
            }

            return request;
        }
    };
}
