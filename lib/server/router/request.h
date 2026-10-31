// ESP32 MiniWebServer Framework - https://github.com/LemurDaniel/ESP32__MiniWebServer-Framework
// Copyright © 2026, Daniel Landau
// MIT License

// NOTE:
//   Parsed HTTP request with a lazily read body, created by MiniServer for every client.
//
//   const std::string &id = req.route["id"];
//   JsonDocument &body = req.body.json();

//
//   Chapters:
//     SETTINGS        Size limits of headers and bodies.
//     READ SOCKET     Buffered socket reader with a lookahead.
//     REQUEST BODY    Lazy body as text, JSON, file or chunks.
//     REQUEST         Status line, headers, cookies, query and token.
//

#pragma once

#include <ArduinoJson.h>
#include <LittleFS.h>

#include <algorithm>
#include <map>
#include <string>
#include <vector>

#include <utils/utility.file.h>
#include <utils/utility.token.h>

namespace EspWeb
{
    // ===========================================================================
    // ==============================   SETTINGS   ===============================
    // ===========================================================================
    //
    //   BODY_SIZE_THRESHOLD   Body size above which the body should be spooled to a file.
    //   BODY_SIZE_TRESHOLD    Deprecated misspelled alias.
    //   MAX_HEADER_SIZE       Header block size above which a request is rejected.
    //

    /** @brief Body size threshold in bytes above which the body should be spooled to a temp file. */
    const size_t BODY_SIZE_THRESHOLD = 8192;

    /** @deprecated Use BODY_SIZE_THRESHOLD instead. */
    const size_t BODY_SIZE_TRESHOLD = BODY_SIZE_THRESHOLD;

    /** @brief Maximum size in bytes of the request line plus headers. */
    const size_t MAX_HEADER_SIZE = 8192;

    // ===========================================================================
    // =============================   READ SOCKET   =============================
    // ===========================================================================
    //
    //   SocketReader   Reads from the socket, returns prepended bytes first.
    //

    /**
     * @brief Buffered socket reader that allows prepending bytes back into the read stream.
     *
     * Used to "unread" bytes that were consumed during header parsing but belong to the body.
     */
    struct SocketReader
    {
        /** @brief File descriptor of the client socket. -1 if uninitialized. */
        int clientSocket = -1;

        /** @brief Internal lookahead buffer for prepended bytes. */
        uint8_t bytes[256];

        /** @brief Number of valid bytes currently in the lookahead buffer. */
        size_t bytesLen = 0;

        /**
         * @brief Replaces the lookahead buffer, so the bytes are returned first by read().
         * @param data Pointer to the bytes to prepend.
         * @param len  Number of bytes to prepend (capped at buffer size).
         */
        void prepend(const uint8_t *data, size_t len)
        {
            bytesLen = std::min(len, sizeof(bytes));
            memcpy(bytes, data, bytesLen);
        }

        /**
         * @brief Reads up to `len` bytes, draining the lookahead buffer first.
         * @param buf Destination buffer.
         * @param len Maximum number of bytes to read.
         * @return Total number of bytes actually read.
         */
        int read(void *buf, size_t len)
        {
            size_t total = 0;

            // Lookahead first
            if (bytesLen > 0)
            {
                const size_t n = std::min(len, bytesLen);
                memcpy(buf, bytes, n);
                memmove(bytes, bytes + n, bytesLen - n);
                bytesLen -= n;
                total += n;

                if (total >= len)
                {
                    return (int)total;
                }
            }

            // Then the socket
            const int n = ::read(clientSocket, (uint8_t *)buf + total, len - total);
            if (n > 0)
            {
                total += n;
            }

            return (int)total;
        }
    };

    // ===========================================================================
    // ============================   REQUEST BODY   =============================
    // ===========================================================================
    //
    //   Metadata   Content length and type from the headers.
    //   Readers    json, text, file and chunks, each reads the body on first call.
    //

    /**
     * @brief Lazy-loaded HTTP request body supporting text, JSON, file, and chunked reads.
     *
     * The body is not read until one of the accessor methods is first called.
     * Use only one reader per request, they all consume the same socket bytes.
     */
    class RequestBody
    {
        friend class Request;

        // Metadata
    public:
        RequestBody() = default;

        /** @brief Total body size in bytes as reported by the Content-Length header. */
        size_t contentLength = 0;

        /** @brief MIME type from the Content-Type request header. */
        std::string contentType = "application/text";

        // Readers
    public:
        /**
         * @brief Reads and parses the body as JSON.
         * @return Reference to the parsed JsonDocument. Cached after the first call.
         */
        JsonDocument &json();

        /**
         * @brief Reads the full body into a string.
         * @return Reference to the body string. Cached after the first call.
         */
        const std::string &text();

        /**
         * @brief Writes the body to a LittleFS file, creating its parent folder.
         * @param name Absolute LittleFS path of the target file.
         * @return The given path.
         */
        const std::string &file(const std::string &name);

        /**
         * @brief Reads the next chunk of raw body bytes directly from the socket.
         * @param chunk     Destination buffer.
         * @param chunkSize Maximum number of bytes to read per call.
         * @return Number of bytes actually read; 0 when the body is exhausted.
         */
        size_t chunks(uint8_t *chunk, size_t chunkSize);

    private:
        /** @brief Socket reader used to stream body bytes on demand. */
        SocketReader socket;

        /** @brief Cached text body (populated on first call to text()). */
        std::string bodyText;

        /** @brief LittleFS path of the spooled temp file (populated on first call to file()). */
        std::string filePath;

        /** @brief Cached parsed JSON body (populated on first call to json()). */
        JsonDocument bodyJson;

        /** @brief Number of bytes already consumed from the socket by chunks(). */
        size_t readSize = 0;
    };

    // ===========================================================================
    // ===============================   REQUEST   ===============================
    // ===========================================================================
    //
    //   Lifecycle   Move-only, created by parse.
    //   Parsing     parse and the header extraction.
    //   Rejection   rejected and error, answered with 413 by MiniServer.
    //   Content     Method, path, headers, cookies, query, route, token and body.
    //

    /**
     * @brief Represents a parsed incoming HTTP request.
     *
     * Non-copyable; move-only. Created by the static Request::parse() factory which reads
     * the status line and headers from the socket. The body is read lazily via RequestBody.
     */
    class Request
    {
        // Lifecycle
    public:
        Request() = default;

        Request(Request &&other) = default;
        Request &operator=(Request &&other) = default;

        Request(const Request &) = delete;
        Request &operator=(const Request &) = delete;

        ~Request();

        // Parsing
    public:
        /**
         * @brief Parses an HTTP request from the given client socket.
         * @param clientSocket File descriptor of the accepted client connection.
         * @return Fully populated Request object. Check `rejected` on parse error.
         */
        static Request parse(int clientSocket);

    private:
        /**
         * @brief Reads and splits raw header lines from the socket.
         *
         * Bytes read past the header block are handed back to the socket reader for the body.
         * @return Vector of raw header lines, empty on read failure or oversized headers.
         */
        std::vector<std::string> extractHeader();

        /** @brief Buffered socket reader used during header parsing. */
        SocketReader socket;

        // Rejection
    public:
        /** @brief True if the request was rejected while parsing (e.g. oversized headers). */
        int rejected = false;

        /** @brief Error message describing why the request was rejected. */
        std::string error;

        // Content
    public:
        /** @brief Validated token extracted from the request (session or API). */
        Token token;

        /** @brief HTTP method in uppercase (e.g. `"GET"`, `"POST"`). */
        std::string method;

        /** @brief Request path without query string (e.g. `"/api/data"`). */
        std::string path;

        /** @brief Map of request header names (as sent by the client) to their values. */
        std::map<std::string, std::string> headers;

        /** @brief Map of parsed cookie name → value pairs from the Cookie header. */
        std::map<std::string, std::string> cookies;

        /** @brief Map of URL query parameter names → values (e.g. `?foo=bar`), not URL-decoded. */
        std::map<std::string, std::string> query;

        /** @brief Map of named route parameters extracted from path patterns (e.g. `:id`). */
        std::map<std::string, std::string> route;

        /** @brief Lazy body reader providing access to the request body. */
        RequestBody body;
    };
}
