// ESP32 MiniWebServer Framework - https://github.com/LemurDaniel/ESP32__MiniWebServer-Framework
// Copyright © 2026, Daniel Landau
// MIT License

// NOTE:
//   Implementation of response.h.

//
//   Chapters:
//     HELPERS        Reason phrases, MIME types and content types.
//     HEADERS        Custom headers and the header block.
//     SENDING        send and the socket timeout.
//     STREAMING      Chunked transfer encoding.
//     FINALIZE       Stops the remaining handlers.
//     STATUS CODES   status and the shortcuts with a default body.
//     BODY           File, text, JSON or HTML body.
//     CONTENT TYPE   Content-Type setters without a body.
//

#include <router/response.h>
#include <utils/utility.log.h>

namespace EspWeb
{
    // ===========================================================================
    // ===============================   HELPERS   ===============================
    // ===========================================================================
    //
    //   CONTENT_TYPE_*   Content types set by the response.
    //   FILE_CHUNK_SIZE  Bytes per write when sending a file.
    //   reasonPhrase     Text of the status line for a status code.
    //   mimeTypeFor      Content type of a file path by its extension.
    //

    namespace
    {
        constexpr const char *CONTENT_TYPE_BINARY = "application/octet-stream";
        constexpr const char *CONTENT_TYPE_TEXT = "text/plain; charset=utf-8";
        constexpr const char *CONTENT_TYPE_JSON = "application/json";
        constexpr const char *CONTENT_TYPE_HTML = "text/html; charset=utf-8";

        /** @brief One TCP segment on Ethernet MTU (1500 - 40 bytes IP/TCP headers). */
        constexpr size_t FILE_CHUNK_SIZE = 1460;

        const char *reasonPhrase(int statusCode)
        {
            switch (statusCode)
            {
            case 200:
                return "OK";
            case 201:
                return "Created";
            case 204:
                return "No Content";
            case 301:
                return "Moved Permanently";
            case 302:
                return "Found";
            case 400:
                return "Bad Request";
            case 401:
                return "Unauthorized";
            case 403:
                return "Forbidden";
            case 404:
                return "Not Found";
            case 413:
                return "Payload Too Large";
            case 500:
                return "Internal Server Error";
            case 503:
                return "Service Unavailable";
            default:
                return "Unknown";
            }
        }

        // "/web/index.html" => ".html" => "text/html; charset=utf-8"
        // "/web/data.bin"   => ".bin"  => nullptr
        const char *mimeTypeFor(const std::string &path)
        {
            static const std::map<std::string, const char *> MIME_TYPES = {
                {".css", "text/css"},         {".html", CONTENT_TYPE_HTML},
                {".ico", "image/x-icon"},     {".js", "application/javascript"},
                {".json", CONTENT_TYPE_JSON}, {".jpeg", "image/jpeg"},
                {".jpg", "image/jpeg"},       {".png", "image/png"},
                {".svg", "image/svg+xml"},    {".txt", CONTENT_TYPE_TEXT},
                {".xml", "application/xml"},
            };

            const size_t dot = path.find_last_of('.');
            if (dot == std::string::npos)
            {
                return nullptr;
            }

            const auto entry = MIME_TYPES.find(path.substr(dot));
            return entry != MIME_TYPES.end() ? entry->second : nullptr;
        }
    }

    // ===========================================================================
    // ===============================   HEADERS   ===============================
    // ===========================================================================
    //
    //   header        Adds or overwrites one header.
    //   sendHeaders   Writes the status line and the header block once.
    //

    Response &Response::header(const std::string &key, const std::string &value)
    {
        headers[key] = value;
        return *this;
    }

    // HTTP/1.1 200 OK\r\n
    // Content-Type: text/plain\r\n
    // Connection: close\r\n
    // \r\n
    void Response::sendHeaders()
    {
        if (isHeaderSent)
        {
            return;
        }
        finalized = true;
        isHeaderSent = true;

        std::string header;
        header.reserve(512);
        header = "HTTP/1.1 ";
        header += std::to_string(statusCode);
        header += " ";
        header += reasonPhrase(statusCode);
        header += "\r\n";
        for (const auto &entry : headers)
        {
            header += entry.first;
            header += ": ";
            header += entry.second;
            header += "\r\n";
        }
        header += "Connection: close\r\n\r\n";
        write(socket, header.c_str(), header.size());
    }

    // ===========================================================================
    // ===============================   SENDING   ===============================
    // ===========================================================================
    //
    //   send         Headers with Content-Length, then the body or the file.
    //   setTimeout   Send and receive timeout of the socket.
    //

    void Response::send()
    {
        if (isSent || isStreaming)
        {
            return;
        }
        isSent = true;
        finalized = true;

        header("Content-Length", std::to_string(isFileMode ? fileSize : body.size())).sendHeaders();

        // In-memory body
        if (!isFileMode)
        {
            write(socket, body.c_str(), body.size());
            return;
        }

        // File body, the size was announced in Content-Length, so never more is sent.
        File file = LittleFS.open(filePath.c_str(), "r");
        if (!file)
        {
            ESPWEB_LOG_ERROR("Response", "File %s disappeared before send()", filePath.c_str());
            return;
        }

        char chunk[FILE_CHUNK_SIZE];
        size_t totalSent = 0;
        while (file.available() && totalSent < fileSize)
        {
            const size_t n = file.readBytes(chunk, sizeof(chunk));
            if (n == 0)
            {
                break;
            }

            write(socket, chunk, n);
            totalSent += n;
        }
        file.close();

        ESPWEB_LOG_DEBUG("Response", "File transfer of %s completed: %u bytes sent", filePath.c_str(),
                         (unsigned)totalSent);
    }

    void Response::setTimeout(int seconds)
    {
        struct timeval tv;
        tv.tv_sec = seconds;
        tv.tv_usec = 0;

        setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        setsockopt(socket, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
    }

    // ===========================================================================
    // ==============================   STREAMING   ==============================
    // ===========================================================================
    //
    //   beginStream   Sends the headers with Transfer-Encoding: chunked.
    //   sendChunk     Sends one chunk, from a string or raw bytes.
    //   endStream     Sends the final zero-length chunk.
    //

    void Response::beginStream()
    {
        if (isSent)
        {
            return;
        }

        finalized = true;
        isStreaming = true;

        // Small chunks go out immediately instead of waiting for Nagle's algorithm.
        int flag = 1;
        setsockopt(socket, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));

        if (headers.find("Content-Type") == headers.end())
        {
            text();
        }

        header("Cache-Control", "no-cache");
        header("Transfer-Encoding", "chunked").sendHeaders();
    }

    // One chunk: "<size in hex>\r\n<data>\r\n"
    // "hello" => "5\r\nhello\r\n"
    void Response::sendChunk(const uint8_t *data, size_t len)
    {
        if (isSent || !isStreaming || len == 0)
        {
            return;
        }

        char sizeHex[16];
        const int hexLen = snprintf(sizeHex, sizeof(sizeHex), "%X\r\n", (unsigned)len);

        std::string frame;
        frame.reserve(hexLen + len + 2);
        frame.append(sizeHex, hexLen);
        frame.append(reinterpret_cast<const char *>(data), len);
        frame.append("\r\n", 2);
        write(socket, frame.c_str(), frame.size());
    }

    void Response::sendChunk(const std::string &data)
    {
        sendChunk(reinterpret_cast<const uint8_t *>(data.data()), data.size());
    }

    void Response::endStream()
    {
        if (!isStreaming || isSent)
        {
            return;
        }

        isSent = true;
        write(socket, "0\r\n\r\n", 5);
    }

    // ===========================================================================
    // ==============================   FINALIZE   ===============================
    // ===========================================================================
    //
    //   finalize   Marks the response as done for the handler pipeline.
    //

    Response &Response::finalize()
    {
        this->finalized = true;
        return *this;
    }

    // ===========================================================================
    // ============================   STATUS CODES   =============================
    // ===========================================================================
    //
    //   status                  Sets any status code.
    //   statusWithDefaultBody   Sets a status and a default body, the shortcuts delegate to it.
    //   Redirect                302 with a Location header.
    //

    Response &Response::status(int status)
    {
        this->statusCode = status;
        return *this;
    }

    Response &Response::statusWithDefaultBody(int status, const char *defaultBody)
    {
        if (this->body.empty())
        {
            this->text(defaultBody);
        }
        this->statusCode = status;
        return *this;
    }

    Response &Response::OK()
    {
        return statusWithDefaultBody(200, "OK");
    }
    Response &Response::Created()
    {
        return statusWithDefaultBody(201, "Created");
    }
    Response &Response::BadRequest()
    {
        return statusWithDefaultBody(400, "Bad Request");
    }
    Response &Response::Unauthorized()
    {
        return statusWithDefaultBody(401, "Unauthorized");
    }
    Response &Response::Forbidden()
    {
        return statusWithDefaultBody(403, "Forbidden");
    }
    Response &Response::NotFound()
    {
        return statusWithDefaultBody(404, "Not Found");
    }
    Response &Response::InternalServerError()
    {
        return statusWithDefaultBody(500, "Internal Server Error");
    }

    Response &Response::Redirect(const std::string &location)
    {
        this->header("Location", location);
        this->statusCode = 302;
        return *this;
    }

    // ===========================================================================
    // ================================   BODY   =================================
    // ===========================================================================
    //
    //   file   Queues a LittleFS file, its content is read in send.
    //   text   Plain-text body.
    //   json   Serialized JSON body.
    //   html   HTML body.
    //

    Response &Response::file(const std::string &path)
    {
        File file = LittleFS.open(path.c_str(), "r");
        if (!file)
        {
            ESPWEB_LOG_WARN("Response", "File %s not found, sending 404", path.c_str());
            this->NotFound().text("File Not Found");
            return *this;
        }

        this->fileSize = file.size();
        this->filePath = path;
        this->isFileMode = true;
        file.close();

        if (this->fileSize == 0)
        {
            ESPWEB_LOG_WARN("Response", "File %s is empty", path.c_str());
        }
        ESPWEB_LOG_DEBUG("Response", "File %s is ready to be served (%u bytes)", path.c_str(),
                         (unsigned)this->fileSize);

        this->status(200);

        const char *mimeType = mimeTypeFor(path);
        if (nullptr == mimeType)
        {
            return this->binary();
        }

        return this->header("Content-Type", mimeType);
    }

    Response &Response::text(const std::string &text)
    {
        this->body = text;
        this->isFileMode = false;
        return this->text();
    }

    Response &Response::json(const JsonDocument &bodyJson)
    {
        this->body.clear();
        serializeJson(bodyJson, this->body);
        this->isFileMode = false;
        return this->json();
    }

    Response &Response::html(const std::string &htmlBody)
    {
        this->text(htmlBody);
        return this->html();
    }

    // ===========================================================================
    // ============================   CONTENT TYPE   =============================
    // ===========================================================================
    //
    //   binary   application/octet-stream
    //   text     text/plain
    //   json     application/json
    //   html     text/html
    //

    Response &Response::binary()
    {
        return this->header("Content-Type", CONTENT_TYPE_BINARY);
    }
    Response &Response::text()
    {
        return this->header("Content-Type", CONTENT_TYPE_TEXT);
    }
    Response &Response::json()
    {
        return this->header("Content-Type", CONTENT_TYPE_JSON);
    }
    Response &Response::html()
    {
        return this->header("Content-Type", CONTENT_TYPE_HTML);
    }
}
