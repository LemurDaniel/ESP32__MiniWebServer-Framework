// ESP32 MiniWebServer Framework - https://github.com/LemurDaniel/ESP32__MiniWebServer-Framework
// Copyright © 2026, Daniel Landau
// MIT License

#pragma once

#include <ArduinoJson-v7.4.3.h>
#include <LittleFS.h>

#include <string>
#include <vector>
#include <map>

#include <utility.file.h>

namespace ESP32WebServer
{

    const size_t BODY_SIZE_TRESHOLD = 8192;

    class Request
    {
    private:
        /*-------------------------------------------------------------------------------------------------
         *
         * static helper Methods
         *
         *
         **/
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

        /*-------------------------------------------------------------------------------------------------
         *
         * Extract body content
         *
         *
         **/
        void drainSocket()
        {
            char drain[256];
            size_t drained = bodyRaw.size();
            bodyRaw.clear();
            while (drained < contentLength)
            {
                size_t toRead = std::min((size_t)sizeof(drain), contentLength - drained);
                int n = read(clientSocket, drain, toRead);
                if (n <= 0)
                    break;
                drained += n;
            }
        }

        std::string extractBodyAsText(size_t maxSize = std::string::npos) const
        {
            size_t size = (maxSize == std::string::npos) ? bodyRaw.size() : std::min(maxSize, bodyRaw.size());
            return std::string(reinterpret_cast<const char *>(bodyRaw.data()), size);
        }

        std::string readBodyAsText()
        {
            char chunk[256];
            while (bodyRaw.size() < contentLength)
            {
                size_t toRead = std::min((size_t)sizeof(chunk), contentLength - bodyRaw.size());
                int n = read(clientSocket, chunk, toRead);
                if (n <= 0)
                    break;
                bodyRaw.insert(bodyRaw.end(), chunk, chunk + n);
            }
            return extractBodyAsText(contentLength);
        }

        /*-------------------------------------------------------------------------------------------------
         *
         * Extract Header
         *
         *
         **/
        std::vector<std::string> extractHeader()
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
        Request() = default;

        Request(Request &&other) = default;
        Request &operator=(Request &&other) = default;

        Request(const Request &) = delete;
        Request &operator=(const Request &) = delete;

        // Delete temp file on destruktor
        ~Request()
        {
            if (!filePath.empty())
            {
                LittleFS.remove(filePath.c_str());
                Serial.print("Removed file: ");
                Serial.println(filePath.c_str());
            }
        }

    private:
        int clientSocket;
        size_t readSize = 0;
        std::vector<uint8_t> bodyRaw; // Raw body as bytes

    public:
        int rejected = false;
        std::string error;

        std::string method;
        std::string path;
        std::map<std::string, std::string> headers;
        std::map<std::string, std::string> cookies;

        size_t contentLength = 0;
        std::string contentType = "application/text";

        std::string filePath; // set if body was too large for RAM and written to LittleFS
        JsonDocument body;    // Parsed JSON body (if Content-Type: application/json)

        static Request parse(int clientSocket)
        {
            Request request;
            request.clientSocket = clientSocket;

            // --- Extract header ---
            Serial.println("Extracing Raw Header");
            std::vector<std::string> headerRaw = request.extractHeader();
            if (headerRaw.empty())
                return request;

            // Split: "GET /path HTTP/1.1"
            std::vector<std::string> firstLine = Request::split(headerRaw[0], " ");
            request.method = firstLine[0];
            request.path = firstLine[1];

            for (size_t i = 1; i < headerRaw.size(); i++)
            {
                std::vector<std::string> line = Request::split(headerRaw[i], ":");
                if (line.size() >= 2)
                    request.headers[line[0]] = line[1];
            }

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
            if (request.headers.find("Content-Type") != request.headers.end())
            {
                request.contentType = request.headers["Content-Type"];
            }

            auto clIt = request.headers.find("Content-Length");
            if (clIt != request.headers.end())
            {
                for (char c : clIt->second)
                {
                    if (c < '0' || c > '9')
                        break;
                    request.contentLength = request.contentLength * 10 + (c - '0');
                }
            }

            if (request.contentLength == 0)
            {
                return request;
            }

            if (request.contentType.find("application/json") != std::string::npos)
            {
                if (request.contentLength > BODY_SIZE_TRESHOLD)
                {
                    request.rejected = true;
                    request.error = "JSON Request Body too big!";
                    return request;
                }

                request.readBodyAsText();
                deserializeJson(request.body, request.bodyRaw);
            }
            else if (
                request.contentLength > BODY_SIZE_TRESHOLD ||
                request.contentType.find("multipart/form-data") != std::string::npos ||
                request.contentType.find("application/octet-stream") != std::string::npos ||
                request.contentType.find("image/") != std::string::npos)
            {
                return request;
            }
            else
            {
                request.readBodyAsText();
            }

            return request;
        }

        size_t readBodyAsChunks(uint8_t *chunk, size_t chunkSize)
        {
            if (readSize >= contentLength)
                return 0;

            size_t written = 0;

            // Copy bytes already buffered from header read into chunk
            if (!bodyRaw.empty())
            {
                memcpy(chunk, bodyRaw.data(), bodyRaw.size());
                written += bodyRaw.size();
                readSize += bodyRaw.size();
                bodyRaw.clear();
            }

            // Fill rest of chunk from socket
            while (written < chunkSize && readSize < contentLength)
            {
                size_t toRead = std::min(chunkSize - written, contentLength - readSize);
                int n = read(clientSocket, chunk + written, toRead);
                if (n <= 0)
                    break;
                written += n;
                readSize += n;
            }

            return written;
        }
    };
}
