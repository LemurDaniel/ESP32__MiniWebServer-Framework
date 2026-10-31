// ESP32 MiniWebServer Framework - https://github.com/LemurDaniel/ESP32__MiniWebServer-Framework
// Copyright © 2026, Daniel Landau
// MIT License

// NOTE:
//   Built-in admin panel: login, dashboard, credentials, API tokens, WiFi and OTA updates.
//   Mounted by MiniServer::start() unless MiniServer::disableAdmin() was called.
//
//   server.registerRouter(EspWeb::AdminRouter(true));

//
//   Chapters:
//     SETTINGS                Custom dashboard links.
//     ADMIN HANDLERS          Declarations of every admin route handler.
//     REGISTER ADMIN ROUTES   The AdminRouter.
//

#pragma once

#include <Update.h>

#include <router/router.h>
#include <utils/utility.wifi.h>

namespace EspWeb
{
    // ===========================================================================
    // ==============================   SETTINGS   ===============================
    // ===========================================================================
    //
    //   CUSTOM_LINKS   Links shown in the dashboard navigation.
    //

    /** @brief Map of custom navigation links injected into the admin dashboard. Key = display name, value = href. */
    extern std::map<std::string, std::string> CUSTOM_LINKS;

    // ===========================================================================
    // ===========================   ADMIN HANDLERS   ============================
    // ===========================================================================
    //
    //   Pages          Login page, dashboard page and the custom links.
    //   Middleware     auth_handler protects /admin and /upload.
    //   Credentials    Login, logout, credential update and restart.
    //   API tokens     List, create and delete permanent tokens.
    //   WiFi           Active, scanned and saved networks.
    //   Updates        Firmware, website upload and website reset.
    //

    // Pages

    /** @brief Returns the custom links JSON used by the admin dashboard frontend. */
    void get_AdminLinks(Request &req, Response &res);

    /** @brief Serves the admin login HTML page. */
    void get_AdminLogin(Request &req, Response &res);

    /** @brief Serves the admin dashboard HTML page (requires authentication). */
    void get_AdminDashboard(Request &req, Response &res);

    // Middleware

    /**
     * @brief Middleware that enforces admin authentication.
     *
     * Validates the admin session cookie. Redirects to `/admin` on failure.
     * Registered automatically on all `/admin` and `/upload` prefixes.
     */
    void auth_handler(Request &req, Response &res);

    // Credentials

    /** @brief Revokes the admin session token of the request. */
    void get_AdminLogout(Request &req, Response &res);

    /** @brief Validates username/password from the request body and returns a session token. */
    void post_AdminLogin(Request &req, Response &res);

    /** @brief Updates the admin username and password. */
    void post_AdminUpdateAuth(Request &req, Response &res);

    /** @brief Triggers an ESP32 restart. */
    void post_AdminRestart(Request &req, Response &res);

    // API tokens

    /** @brief Returns the list of all stored permanent API tokens. */
    void get_PermTokens(Request &req, Response &res);

    /** @brief Returns the configured list of available token actions. */
    void get_TokenActions(Request &req, Response &res);

    /** @brief Creates a new permanent API token from the request body. */
    void post_PermToken(Request &req, Response &res);

    /** @brief Deletes a permanent API token by name. */
    void delete_PermToken(Request &req, Response &res);

    // WiFi

    /** @brief Returns details of the currently active WiFi connection. */
    void get_WiFiActive(Request &req, Response &res);

    /** @brief Scans for nearby WiFi networks and returns the results. */
    void get_WiFiScan(Request &req, Response &res);

    /** @brief Returns all saved WiFi network configurations. */
    void get_WiFiSavedNetworks(Request &req, Response &res);

    /** @brief Removes a saved WiFi network by SSID. */
    void delete_WiFiSavedNetwork(Request &req, Response &res);

    /** @brief Saves a new WiFi network configuration (SSID + password). */
    void post_WiFiSavedNetwork(Request &req, Response &res);

    // Updates

    /** @brief Receives a file upload and stores it in the web folder. */
    void post_UploadFile(Request &req, Response &res);

    /** @brief Receives a firmware binary, performs an OTA update and restarts. */
    void post_UpdateCode(Request &req, Response &res);

    /** @brief Clears all files in the web assets folder. */
    void get_ClearWebsite(Request &req, Response &res);

    // ===========================================================================
    // ========================   REGISTER ADMIN ROUTES   ========================
    // ===========================================================================
    //
    //   AdminRouter   Wires the middleware and every admin route.
    //

    /**
     * @brief Pre-configured Router that registers all built-in admin and upload routes.
     *
     * Automatically wires the auth_handler middleware on `/admin` and `/upload` prefixes.
     * If `enableDashboard` is non-zero, the HTML pages for login and dashboard are also served.
     */
    class AdminRouter : public EspWeb::Router
    {
    public:
        /**
         * @brief Constructs the AdminRouter and registers all admin routes.
         * @param enableDashboard Non-zero to also register the login and dashboard HTML page routes.
         */
        AdminRouter(int enableDashboard)
        {
            // Middleware, it lets /admin and /admin/login through without a token.
            use("/admin", auth_handler);
            use("/upload", auth_handler);

            // Pages
            if (enableDashboard)
            {
                route("GET", "/admin", get_AdminLogin);
                route("GET", "/admin/dashboard", get_AdminDashboard);
            }
            route("GET", "/admin/links", get_AdminLinks);

            // Credentials
            route("POST", "/admin/login", post_AdminLogin);
            route("GET", "/admin/logout", get_AdminLogout);
            route("POST", "/admin/auth", post_AdminUpdateAuth);
            route("POST", "/admin/restart", post_AdminRestart);

            // API tokens
            route("GET", "/admin/tokens", get_PermTokens);
            route("GET", "/admin/token/actions", get_TokenActions);
            route("POST", "/admin/token", post_PermToken);
            route("DELETE", "/admin/token", delete_PermToken);

            // WiFi
            route("GET", "/admin/wifi/scan", get_WiFiScan);
            route("GET", "/admin/wifi/active", get_WiFiActive);
            route("GET", "/admin/wifi/networks", get_WiFiSavedNetworks);
            route("POST", "/admin/wifi/network", post_WiFiSavedNetwork);
            route("DELETE", "/admin/wifi/network", delete_WiFiSavedNetwork);

            // Updates
            route("POST", "/upload/code", post_UpdateCode);
            route("POST", "/upload/file", post_UploadFile);
            route("GET", "/upload/clear/website", get_ClearWebsite);
        }
    };
}
