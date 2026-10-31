// ESP32 MiniWebServer Framework - https://github.com/LemurDaniel/ESP32__MiniWebServer-Framework
// Copyright © 2026, Daniel Landau
// MIT License

// NOTE:
//   Implementation of router.h.

//
//   Chapters:
//     REGISTRATION   Routes and middlewares of a router.
//     MIDDLEWARES    The built-in CORS and auth middlewares.
//     TOKENS         Shortcuts into the TokenManager.
//

#include <router/router.h>

namespace EspWeb
{
    FileHandler Router::fs;

    // ===========================================================================
    // ============================   REGISTRATION   =============================
    // ===========================================================================
    //
    //   route   Adds a route with one or several handlers.
    //   use     Appends a middleware to the chain of a prefix.
    //

    Router::Router() {}

    void Router::route(const std::string &method, const std::string &path, std::vector<RequestHandler> handlers)
    {
        routes.push_back({method, path, handlers});
    }

    void Router::route(const std::string &method, const std::string &path, RequestHandler handler)
    {
        route(method, path, std::vector<RequestHandler>{handler});
    }

    void Router::use(const std::string &prefix, const RequestHandler &handler)
    {
        // operator[] creates the chain on first use.
        middlewares[prefix].push_back(handler);
    }

    // ===========================================================================
    // =============================   MIDDLEWARES   =============================
    // ===========================================================================
    //
    //   cors   Sets the allowed origin.
    //   auth   Rejects requests without a valid token.
    //

    RequestHandler Router::cors(const std::string &origin)
    {
        return [origin](Request &req, Response &res)
        {
            res.header("Access-Control-Allow-Origin", origin);
        };
    }

    RequestHandler Router::auth()
    {
        return [](Request &req, Response &res)
        {
            if (!req.token.isValid())
            {
                res.Unauthorized().finalize();
            }
        };
    }

    // ===========================================================================
    // ===============================   TOKENS   ================================
    // ===========================================================================
    //
    //   getSessionToken   Delegates to TokenManager::getSessionToken.
    //   getApiToken       Delegates to TokenManager::getApiToken.
    //   removeApiToken    Delegates to TokenManager::removeApiToken.
    //

    std::string Router::getSessionToken(long seconds, const std::vector<std::string> &actions)
    {
        return TokenManager::instance().getSessionToken(seconds, actions).value;
    }

    std::string Router::getApiToken(const std::string &name, const std::vector<std::string> &actions)
    {
        return TokenManager::instance().getApiToken(name, actions).value;
    }

    void Router::removeApiToken(const std::string &name)
    {
        TokenManager::instance().removeApiToken(name);
    }
}
