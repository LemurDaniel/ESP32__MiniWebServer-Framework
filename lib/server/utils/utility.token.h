// ESP32 MiniWebServer Framework - https://github.com/LemurDaniel/ESP32__MiniWebServer-Framework
// Copyright © 2026, Daniel Landau
// MIT License

// NOTE:
//   Session tokens, permanent API tokens and the admin credentials.
//   Requires: NVS (Preferences) for the persistent stores.
//
//   EspWeb::Token token = EspWeb::TokenManager::instance().checkToken(rawToken);
//   if (token.isAllowed("admin")) { ... }

//
//   Chapters:
//     SETTINGS         Store keys, cookie name and default credentials.
//     TOKEN            A single session or API token.
//     MANAGE TOKENS    The TokenManager singleton.
//

#pragma once

#include <map>
#include <string>
#include <vector>

#include <ArduinoJson.h>
#include <mbedtls/sha256.h>

#include <utils/utility.file.h>

// ===========================================================================
// ==============================   SETTINGS   ===============================
// ===========================================================================
//
//   ADMIN_PERMANENT_TOKENS   Store key of the API tokens.
//   ADMIN_CREDENTIALS_FILE   Store key of the admin credentials.
//   DEFAULT_ADMIN_COOKIE     Session cookie name.
//   DEFAULT_ADMIN_USER       Default admin username.
//   DEFAULT_ADMIN_PWD        Default admin password.
//   TOKEN_ACTIONS            Actions that can be granted to tokens.
//
// The store keys are hashed into NVS keys, changing them orphans existing data.

/** @brief Store key for the persistent API token store. */
#define ADMIN_PERMANENT_TOKENS "/admin_perm_token.json"

/** @brief Store key for the admin credentials store. */
#define ADMIN_CREDENTIALS_FILE "/admin_credentials.json"

/** @brief Name of the session cookie used for admin authentication. */
#define DEFAULT_ADMIN_COOKIE "adminToken"

/** @brief Default admin username (should be changed in production). */
#define DEFAULT_ADMIN_USER "admin"

/** @brief Default admin password (should be changed in production). */
#define DEFAULT_ADMIN_PWD "admin"

namespace EspWeb
{
    /** @brief Global list of action names that can be granted to tokens. `"admin"` is always first. */
    extern std::vector<std::string> TOKEN_ACTIONS;

    // ===========================================================================
    // ================================   TOKEN   ================================
    // ===========================================================================
    //
    //   Factories   NullToken, createSessionToken and createApiToken.
    //   Content     Value, name and the granted actions.
    //   Checks      Validity, kind, expiry and permissions.
    //

    /**
     * @brief Represents an authenticated token (session or API).
     *
     * Carries the token value, associated actions, and expiry time.
     * Use Token::NullToken() for unauthenticated / invalid states.
     */
    class Token
    {
        friend class TokenManager;

        // Factories
    public:
        Token() = default;

        /**
         * @brief Creates an invalid placeholder token.
         * @return A Token where isValid() returns false.
         */
        static Token NullToken();

        /**
         * @brief Creates a time-limited session token with a generated random value.
         * @param seconds Lifetime in seconds from now.
         * @param action  List of permitted action names.
         * @return A new Token with `_isSessionToken = true`.
         */
        static Token createSessionToken(long seconds, const std::vector<std::string> &action);

        /**
         * @brief Creates a permanent API token with a generated random value.
         * @param name   Human-readable identifier for the token.
         * @param action List of permitted action names.
         * @return A new Token with `_isApiToken = true`.
         */
        static Token createApiToken(const std::string &name, const std::vector<std::string> &action);

        // Content
    public:
        /** @brief List of action names this token is authorized for. */
        std::vector<std::string> action;

        /** @brief The raw token string value. */
        std::string value;

        /** @brief Human-readable name / identifier for this token. */
        std::string name;

        // Checks
    public:
        /**
         * @brief Returns the uptime in seconds when this token expires.
         * @return Expiry in seconds since boot, or -1 for non-expiring tokens.
         */
        const long expires();

        /**
         * @brief Checks whether this token has full admin (privileged) access.
         * @return `true` if the token carries the `"admin"` action.
         */
        bool isPrivileged();

        /**
         * @brief Checks whether this token is valid (not a NullToken()).
         * @return `true` if valid.
         */
        bool isValid();

        /** @brief Returns `true` if this is a session token (time-limited, stored in memory). */
        bool isSessionToken() { return _isSessionToken; }

        /** @brief Returns `true` if this is a permanent API token (no expiry, stored in NVS). */
        bool isApiToken() { return _isApiToken; }

        /**
         * @brief Checks whether this token is authorized for a specific action.
         * @param action Action name to check.
         * @return `true` if the token is valid and the action is in its action list.
         */
        bool isAllowed(const std::string &action);

    private:
        /** @brief False if the token was explicitly invalidated or never initialized. */
        bool _isValid = true;

        /** @brief Uptime in seconds when the token expires; -1 for non-expiring API tokens. */
        long _expires = -1;

        /** @brief True if this is a session token (time-limited, stored in memory). */
        bool _isSessionToken = false;

        /** @brief True if this is a permanent API token (no expiry, stored in NVS). */
        bool _isApiToken = false;
    };

    // ===========================================================================
    // ============================   MANAGE TOKENS   ============================
    // ===========================================================================
    //
    //   Singleton        instance, the only way to reach the manager.
    //   Credentials      Salt, admin credentials and the SHA-256 hash.
    //   Token lookup     checkToken resolves a raw token from a request.
    //   Session tokens   Issue and revoke in-memory session tokens.
    //   API tokens       Create, remove and list persistent API tokens.
    //

    /**
     * @brief Singleton manager for session and API tokens and admin credentials.
     *
     * Handles SHA-256 hashing, credential verification, and persistent storage
     * of API tokens in NVS. Access via TokenManager::instance().
     */
    class TokenManager
    {
        // Singleton
    public:
        /**
         * @brief Returns the singleton TokenManager instance.
         * @return Reference to the global TokenManager.
         */
        static TokenManager &instance()
        {
            static TokenManager _instance;
            return _instance;
        }

    private:
        TokenManager() {}
        TokenManager(const TokenManager &) = delete;
        void operator=(const TokenManager &) = delete;

        // Credentials
    public:
        /**
         * @brief Computes the salted SHA-256 hex digest of a string.
         * @param text Input string to hash, the admin salt is appended.
         * @return Lowercase hex-encoded SHA-256 digest.
         */
        std::string generateSHA256(const std::string &text);

        /**
         * @brief Sets the salt used when hashing admin passwords.
         * @param salt Salt string appended before hashing.
         */
        void setSalt(const std::string &salt);

        /**
         * @brief Stores new admin credentials (hashed with SHA-256 + salt).
         * @param username New admin username.
         * @param password Plaintext password (stored as a salted hash).
         */
        void setCredentials(const std::string &username, const std::string &password);

        /**
         * @brief Validates a username/password pair against stored credentials.
         * @param username Username to check.
         * @param password Plaintext password to check.
         * @return `true` if the credentials match.
         */
        bool checkCredentials(const std::string &username, const std::string &password);

    private:
        /**
         * @brief Reads a value from the admin credentials store, creating defaults on first access.
         * @param key Key to look up.
         * @return Value string, or empty string if not found.
         */
        std::string getValue(const std::string &key);

        /**
         * @brief Writes a value to the admin credentials store and persists it.
         * @param key   Key to write.
         * @param value Value to store.
         */
        void setValue(const std::string &key, const std::string &value);

        /** @brief Cached admin credentials JSON document loaded from NVS. */
        JsonDocument *_admin = nullptr;

        // Token lookup
    public:
        /**
         * @brief Looks up a raw token string and returns the matching Token.
         *
         * Expired session tokens are removed on lookup.
         * @param authToken Raw token string from a cookie or Authorization header.
         * @return Matching Token if valid; NullToken() otherwise.
         */
        Token checkToken(const std::string &authToken);

        // Session tokens
    public:
        /**
         * @brief Issues a new time-limited session token.
         * @param seconds  Lifetime in seconds from now.
         * @param actions  List of permitted action names.
         * @return The newly created Token.
         */
        Token getSessionToken(long seconds, const std::vector<std::string> &actions);

        /**
         * @brief Revokes a session token by its raw value.
         * @param token Raw token string to remove.
         */
        void removeSessionToken(const std::string &token);

    private:
        /** @brief In-memory map of active session token values → Token objects. */
        std::map<std::string, Token> _SESSION_TOKENS;

        // API tokens
    public:
        /**
         * @brief Creates a named permanent API token, replacing an existing one with the same name.
         * @param name   Token identifier.
         * @param action List of permitted action names.
         * @return The API Token.
         */
        Token getApiToken(const std::string &name, const std::vector<std::string> &action);

        /**
         * @brief Permanently removes a named API token from storage.
         * @param name Name of the API token to delete.
         */
        void removeApiToken(const std::string &name);

        /**
         * @brief Returns all currently stored API tokens.
         * @return Vector of all API Token objects.
         */
        std::vector<Token> listApiTokens();

    private:
        /** @brief Drops the cached API tokens, the next lookup reloads them from storage. */
        void invalidateApiTokens();

        /** @brief Lazily-loaded map of persistent API token values → Token objects. */
        std::map<std::string, Token> *_API_TOKENS = nullptr;
    };
}
