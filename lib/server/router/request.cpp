// ESP32 MiniWebServer Framework - https://github.com/LemurDaniel/ESP32__MiniWebServer-Framework
// Copyright © 2026, Daniel Landau
// MIT License

// NOTE:
//   Implementation of request.h.

//
//   Chapters:
//     HELPERS          Key-value splitting and header lookup.
//     REQUEST BODY     Lazy body readers.
//     PARSE REQUEST    Header extraction and the parse flow.
//

#include <cstdlib>

#include <router/request.h>
#include <utils/utility.log.h>

namespace EspWeb
{
    // ===========================================================================
    // ===============================   HELPERS   ===============================
    // ===========================================================================
    //
    //   splitPair   Splits at the first separator into a trimmed key and value.
    //   findValue   Returns a header value or nullptr.
    //

    namespace
    {
        // Only the first separator splits, the value keeps the rest:
        // "Host: 192.168.0.10:80" by ':' => {"Host", "192.168.0.10:80"}
        // "token=abc=="           by '=' => {"token", "abc=="}
        // "flag"                  by '=' => {"flag", ""}
        std::pair<std::string, std::string> splitPair(const std::string &text, char separator)
        {
            const size_t pos = text.find(separator);
            if (pos == std::string::npos)
            {
                return {fileHandler.trim(text), ""};
            }

            return {fileHandler.trim(text.substr(0, pos)), fileHandler.trim(text.substr(pos + 1))};
        }

        const std::string *findValue(const std::map<std::string, std::string> &map, const std::string &key)
        {
            const auto entry = map.find(key);
            return entry != map.end() ? &entry->second : nullptr;
        }
    }

    // ===========================================================================
    // ============================   REQUEST BODY   =============================
    // ===========================================================================
    //
    //   json     Parses the text body once.
    //   text     Reads the whole body into memory once.
    //   file     Writes the body into a LittleFS file.
    //   chunks   Reads the next part of the body.
    //

    JsonDocument &RequestBody::json()
    {
        if (bodyJson.size() == 0)
        {
            deserializeJson(bodyJson, text().c_str());
        }

        return bodyJson;
    }

    const std::string &RequestBody::text()
    {
        if (!bodyText.empty())
        {
            return bodyText;
        }

        char chunk[256];
        while (bodyText.size() < contentLength)
        {
            const size_t toRead = std::min(sizeof(chunk), contentLength - bodyText.size());
            const int n = socket.read(chunk, toRead);
            if (n <= 0)
            {
                break;
            }
            bodyText.append(chunk, n);
        }

        return bodyText;
    }

    const std::string &RequestBody::file(const std::string &filePath)
    {
        if (filePath.empty())
        {
            return filePath;
        }

        const size_t slash = filePath.find_last_of('/');
        if (slash != std::string::npos)
        {
            LittleFS.mkdir(filePath.substr(0, slash).c_str());
        }

        File tmpFile = LittleFS.open(filePath.c_str(), "w", true);
        if (!tmpFile)
        {
            ESPWEB_LOG_ERROR("Request", "Failed to open file for writing: %s", filePath.c_str());
            return filePath;
        }

        uint8_t chunk[512];
        size_t written = 0;
        while (written < contentLength)
        {
            const size_t toRead = std::min(sizeof(chunk), contentLength - written);
            const int n = socket.read(chunk, toRead);
            if (n <= 0)
            {
                break;
            }
            tmpFile.write(chunk, n);
            written += n;
        }
        tmpFile.close();

        ESPWEB_LOG_DEBUG("Request", "Written: %s (%u bytes)", filePath.c_str(), (unsigned)written);
        return filePath;
    }

    size_t RequestBody::chunks(uint8_t *chunk, size_t chunkSize)
    {
        if (readSize >= contentLength)
        {
            return 0;
        }

        const size_t toRead = std::min(chunkSize, contentLength - readSize);
        const int n = socket.read(chunk, toRead);
        if (n <= 0)
        {
            return 0;
        }

        readSize += n;
        return n;
    }

    // ===========================================================================
    // ============================   PARSE REQUEST   ============================
    // ===========================================================================
    //
    //   extractHeader   Reads until the empty line, keeps the body bytes for later.
    //   parse           Request line, headers, cookies, token and body metadata.
    //

    Request::~Request() {}

    // "GET / HTTP/1.1\r\nHost: esp\r\n\r\n<body...>" => ["GET / HTTP/1.1", "Host: esp"], "<body...>" prepended
    std::vector<std::string> Request::extractHeader()
    {
        std::vector<std::string> headerRaw;

        char chunk[256];
        std::string header;
        header.reserve(512);

        // Read until \r\n\r\n (complete headers)
        size_t headerEnd = std::string::npos;
        do
        {
            const int n = socket.read(chunk, sizeof(chunk));
            if (n <= 0)
            {
                ESPWEB_LOG_WARN("Request", "Failed to read from socket");
                return {};
            }
            header.append(chunk, n);
            headerEnd = header.find("\r\n\r\n");
        } while (headerEnd == std::string::npos && header.size() < MAX_HEADER_SIZE);

        if (headerEnd == std::string::npos)
        {
            rejected = true;
            error = "Request header too large";
            return headerRaw;
        }

        // Bytes after the headers belong to the body, they are returned first on the next read.
        const size_t bodyStart = headerEnd + 4;
        socket.prepend(reinterpret_cast<const uint8_t *>(header.data()) + bodyStart, header.size() - bodyStart);

        // Split the header block into lines
        header.resize(headerEnd);
        size_t startOfLine = 0;
        while (startOfLine < header.size())
        {
            size_t endOfLine = header.find("\r\n", startOfLine);
            if (endOfLine == std::string::npos)
            {
                endOfLine = header.size();
            }

            headerRaw.push_back(header.substr(startOfLine, endOfLine - startOfLine));
            startOfLine = endOfLine + 2;
        }

        return headerRaw;
    }

    Request Request::parse(int clientSocket)
    {
        Request request;
        request.socket.clientSocket = clientSocket;

        // A default Token is valid, so without the NullToken auth() would accept requests without any token.
        request.token = Token::NullToken();

        const std::vector<std::string> headerRaw = request.extractHeader();

        // The socket (with any buffered body bytes) moves into the body.
        request.body.socket = std::move(request.socket);
        if (headerRaw.empty())
        {
            return request;
        }

        // Request line
        // "GET /path?a=1&b HTTP/1.1" => method "GET", path "/path", query {a: "1", b: ""}
        const std::vector<std::string> requestLine = fileHandler.split(headerRaw[0], " ");
        if (requestLine.size() < 2)
        {
            ESPWEB_LOG_WARN("Request", "Malformed request line: %s", headerRaw[0].c_str());
            return request;
        }

        request.method = requestLine[0];
        request.path = requestLine[1];

        const size_t queryStart = request.path.find('?');
        if (queryStart != std::string::npos)
        {
            const std::string queryString = request.path.substr(queryStart + 1);
            request.path.resize(queryStart);

            for (const std::string &param : fileHandler.split(queryString, "&"))
            {
                request.query.insert(splitPair(param, '='));
            }
        }

        // Header fields
        for (size_t i = 1; i < headerRaw.size(); i++)
        {
            const auto field = splitPair(headerRaw[i], ':');
            if (!field.first.empty())
            {
                request.headers[field.first] = field.second;
            }
        }

        // Cookies
        // "Cookie: a=1; b=2" => {a: "1", b: "2"}
        if (const std::string *cookieHeader = findValue(request.headers, "Cookie"))
        {
            for (const std::string &cookie : fileHandler.split(*cookieHeader, ";"))
            {
                const auto pair = splitPair(cookie, '=');
                if (!pair.first.empty())
                {
                    request.cookies[pair.first] = pair.second;
                }
            }
        }

        // Token
        // Order of precedence: Authorization header > admin cookie > NullToken (set above).
        if (const std::string *authorization = findValue(request.headers, "Authorization"))
        {
            request.token = TokenManager::instance().checkToken(*authorization);
        }
        else if (const std::string *cookie = findValue(request.cookies, DEFAULT_ADMIN_COOKIE))
        {
            request.token = TokenManager::instance().checkToken(*cookie);
        }

        // Body metadata
        if (const std::string *contentType = findValue(request.headers, "Content-Type"))
        {
            request.body.contentType = *contentType;
        }

        // strtoul instead of stoul: an invalid value results in 0 instead of an exception.
        if (const std::string *contentLength = findValue(request.headers, "Content-Length"))
        {
            request.body.contentLength = std::strtoul(contentLength->c_str(), nullptr, 10);
        }

        ESPWEB_LOG_DEBUG("Request", "%s %s (%u body bytes)", request.method.c_str(), request.path.c_str(),
                         (unsigned)request.body.contentLength);
        return request;
    }
}
