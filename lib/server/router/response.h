// ESP32 MiniWebServer Framework - https://github.com/LemurDaniel/ESP32__MiniWebServer-Framework
// Copyright © 2026, Daniel Landau
// MIT License

// NOTE:
//   HTTP response written directly to the client socket, chainable where methods return `Response &`.
//
//   res.status(200).json(doc);                // sent by the server after the handlers
//   res.beginStream(); res.sendChunk("1"); res.endStream();

//
//   Chapters:
//     BUILD AND SEND RESPONSES   The Response class.
//

#pragma once

#include <Arduino.h>
#include <LittleFS.h>
#include <sys/socket.h>

#include <map>
#include <string>

#include <ArduinoJson.h>

namespace EspWeb
{
    // ===========================================================================
    // ======================   BUILD AND SEND RESPONSES   =======================
    // ===========================================================================
    //
    //   Construction   Binds the response to a socket.
    //   Sending        send writes the whole response at once.
    //   Streaming      beginStream, sendChunk and endStream for chunked encoding.
    //   Finalize       Stops the remaining handlers and middlewares.
    //   Headers        Custom headers and the header block.
    //   Status codes   status and the shortcuts with a default body.
    //   Body           File, text, JSON or HTML body.
    //   Content type   Content-Type setters without a body.
    //

    /**
     * @brief Represents an HTTP response that is written directly to a client socket.
     *
     * Supports single-shot responses (send()), chunked streaming (beginStream / sendChunk / endStream),
     * and serving files from LittleFS. Methods are chainable where they return `Response &`.
     */
    class Response
    {
        // Construction
    public:
        /**
         * @brief Constructs a Response bound to the given socket.
         * @param socket File descriptor of the connected client socket.
         */
        Response(int socket) { this->socket = socket; }

    private:
        /** @brief Underlying TCP socket file descriptor. */
        int socket;

        // Sending
    public:
        /**
         * @brief Finalizes and sends the complete response to the client.
         *
         * Writes headers (including Content-Length) followed by the body or file contents.
         * No-op if already sent or if a streaming session is active.
         */
        void send();

        /**
         * @brief Sets the socket send/receive timeout.
         * @param seconds Timeout in seconds applied to both SO_RCVTIMEO and SO_SNDTIMEO.
         */
        void setTimeout(int seconds);

    private:
        /** @brief True after send() or endStream() has been called; prevents double-sending. */
        int isSent = false;

        // Streaming
    public:
        /**
         * @brief Starts a chunked Transfer-Encoding streaming session.
         *
         * Sends headers with `Transfer-Encoding: chunked`. Must be called before sendChunk().
         * No-op if send() was already called.
         */
        void beginStream();

        /**
         * @brief Sends a string as a single chunk in a chunked streaming session.
         *
         * Requires beginStream() to have been called first.
         * @param data String whose contents are sent as one chunk.
         */
        void sendChunk(const std::string &data);

        /**
         * @brief Sends raw bytes as a single chunk in a chunked streaming session.
         *
         * Requires beginStream() to have been called first.
         * @param data Pointer to the byte buffer to send.
         * @param len  Number of bytes to send. No-op if 0.
         */
        void sendChunk(const uint8_t *data, size_t len);

        /**
         * @brief Terminates the chunked streaming session by sending the final zero-length chunk.
         *
         * No-op if streaming was not started or the response was already sent.
         */
        void endStream();

    private:
        /** @brief True while a chunked streaming session is active. */
        int isStreaming = false;

        // Finalize
    public:
        /**
         * @brief Marks the response as finalized, preventing further handler/middleware processing.
         * @return Reference to this Response for chaining.
         */
        Response &finalize();

        /**
         * @brief Returns whether the response has been finalized.
         * @return Non-zero if finalized, 0 otherwise.
         */
        int isFinalized() { return finalized; }

    private:
        /** @brief Set to true by finalize() or send(); prevents further modifications by middleware. */
        int finalized = false;

        // Headers
    public:
        /**
         * @brief Adds or overwrites an HTTP response header.
         * @param key   Header name (e.g. `"Content-Type"`).
         * @param value Header value (e.g. `"application/json"`).
         * @return Reference to this Response for chaining.
         */
        Response &header(const std::string &key, const std::string &value);

        /**
         * @brief Writes the status line and all queued headers to the socket.
         *
         * Called automatically by send() and beginStream(). Subsequent calls are no-ops.
         */
        void sendHeaders();

    private:
        /** @brief Custom headers added via header(). Sent once by sendHeaders(). */
        std::map<std::string, std::string> headers;

        /** @brief True after sendHeaders() has been called; prevents sending headers twice. */
        int isHeaderSent = false;

        // Status codes
    public:
        /**
         * @brief Sets an arbitrary HTTP status code.
         * @param status HTTP status code (e.g. 200, 404, 500).
         * @return Reference to this Response for chaining.
         */
        Response &status(int status);

        /** @brief Sets status 200 OK. Defaults body to `"OK"` if no body was set. @return `*this` */
        Response &OK();

        /** @brief Sets status 201 Created. Defaults body to `"Created"` if empty. @return `*this` */
        Response &Created();

        /** @brief Sets status 400 Bad Request. Defaults body to `"Bad Request"` if empty. @return `*this` */
        Response &BadRequest();

        /** @brief Sets status 401 Unauthorized. Defaults body to `"Unauthorized"` if empty. @return `*this` */
        Response &Unauthorized();

        /** @brief Sets status 403 Forbidden. Defaults body to `"Forbidden"` if empty. @return `*this` */
        Response &Forbidden();

        /** @brief Sets status 404 Not Found. Defaults body to `"Not Found"` if empty. @return `*this` */
        Response &NotFound();

        /** @brief Sets status 500 Internal Server Error. Defaults body accordingly if empty. @return `*this` */
        Response &InternalServerError();

        /**
         * @brief Sets status 302 Found and adds a `Location` header.
         * @param location Target URL for the redirect.
         * @return Reference to this Response for chaining.
         */
        Response &Redirect(const std::string &location);

    private:
        /**
         * @brief Sets a status and, if no body was set yet, a plain-text default body.
         * @param status      HTTP status code.
         * @param defaultBody Body used when the body is still empty.
         * @return Reference to this Response for chaining.
         */
        Response &statusWithDefaultBody(int status, const char *defaultBody);

        /** @brief HTTP status code sent in the response line (default 200). */
        int statusCode = 200;

        // Body
    public:
        /**
         * @brief Queues a LittleFS file to be served and sets Content-Type from its extension.
         *
         * Falls back to `application/octet-stream` for unknown extensions.
         * Sets status 404 if the file does not exist.
         * @param path Absolute LittleFS path (e.g. `"/index.html"`).
         * @return Reference to this Response for chaining.
         */
        Response &file(const std::string &path);

        /**
         * @brief Sets the response body to a plain-text string and Content-Type to `text/plain`.
         * @param text Body content.
         * @return Reference to this Response for chaining.
         */
        Response &text(const std::string &text);

        /**
         * @brief Serializes a JsonDocument as the response body and sets Content-Type to `application/json`.
         * @param bodyJson ArduinoJson document to serialize.
         * @return Reference to this Response for chaining.
         */
        Response &json(const JsonDocument &bodyJson);

        /**
         * @brief Sets the response body to an HTML string and Content-Type to `text/html`.
         * @param htmlBody HTML content.
         * @return Reference to this Response for chaining.
         */
        Response &html(const std::string &htmlBody);

    private:
        /** @brief In-memory response body used when isFileMode is false. */
        std::string body;

        /** @brief When true, send() reads from filePath instead of body. */
        bool isFileMode = false;

        /** @brief LittleFS path of the file to serve when isFileMode is true. */
        std::string filePath;

        /** @brief Size of the file to serve when isFileMode is true. */
        size_t fileSize = 0;

        // Content type
    public:
        /** @brief Sets Content-Type to `application/octet-stream`. @return `*this` */
        Response &binary();

        /** @brief Sets Content-Type to `text/plain; charset=utf-8`. @return `*this` */
        Response &text();

        /** @brief Sets Content-Type to `application/json`. @return `*this` */
        Response &json();

        /** @brief Sets Content-Type to `text/html; charset=utf-8`. @return `*this` */
        Response &html();
    };
}
