// ESP32 MiniWebServer Framework - https://github.com/LemurDaniel/ESP32__MiniWebServer-Framework
// Copyright © 2026, Daniel Landau
// MIT License

// NOTE:
//   Implementation of utility.token.h.

//
//   Chapters:
//     TOKEN            Factories and checks of a single token.
//     CREDENTIALS      Admin credential store, salt and SHA-256 hashing.
//     TOKEN LOOKUP     Resolves a raw token from a request.
//     SESSION TOKENS   Issue and revoke in-memory session tokens.
//     API TOKENS       Create, remove and list persistent API tokens.
//

#include <algorithm>

#include <utils/utility.token.h>
#include <utils/utility.log.h>

namespace EspWeb
{
    namespace
    {
        /** @brief Keys inside the admin credentials store. */
        constexpr const char *KEY_ADMIN_SALT = "admin_salt";
        constexpr const char *KEY_ADMIN_USER = "admin_user";
        constexpr const char *KEY_ADMIN_PWD = "admin_pwd";

        /** @brief Length of a generated admin salt. */
        constexpr int SALT_LENGTH = 32;

        /** @brief Length of a SHA-256 digest in bytes. */
        constexpr size_t SHA256_LENGTH = 32;

        /** @brief Current uptime in seconds, the time base of session token expiry. */
        long uptimeSeconds()
        {
            return ::millis() / 1000;
        }
    }
    std::vector<std::string> TOKEN_ACTIONS = {"admin"};

    // ===========================================================================
    // ================================   TOKEN   ================================
    // ===========================================================================
    //
    //   NullToken            An invalid placeholder.
    //   createSessionToken   A random token that expires after the given seconds.
    //   createApiToken       A random named token without expiry.
    //   expires              Expiry in seconds since boot.
    //   isPrivileged         Whether the token carries "admin".
    //   isValid              Whether the token is not a NullToken.
    //   isAllowed            Whether the token carries the given action.
    //

    Token Token::NullToken()
    {
        Token token;
        token._isValid = false;
        return token;
    }

    Token Token::createSessionToken(long durationSeconds, const std::vector<std::string> &action)
    {
        Token token;
        token.name = "Session Token";
        token.value = TokenManager::instance().generateSHA256(fileHandler.randomString());
        token.action = action;
        token._expires = uptimeSeconds() + durationSeconds;
        token._isSessionToken = true;
        return token;
    }

    Token Token::createApiToken(const std::string &name, const std::vector<std::string> &action)
    {
        Token token;
        token.name = name;
        token.value = TokenManager::instance().generateSHA256(fileHandler.randomString());
        token.action = action;
        token._isApiToken = true;
        return token;
    }

    const long Token::expires()
    {
        return _expires;
    }
    bool Token::isPrivileged()
    {
        return isAllowed("admin");
    }
    bool Token::isValid()
    {
        return _isValid;
    }

    bool Token::isAllowed(const std::string &action)
    {
        if (!_isValid)
        {
            return false;
        }

        return std::find(this->action.begin(), this->action.end(), action) != this->action.end();
    }

    // ===========================================================================
    // =============================   CREDENTIALS   =============================
    // ===========================================================================
    //
    //   setValue           Writes one key into the cache and the store.
    //   getValue           Reads one key, creates the defaults on first access.
    //   generateSHA256     Salted SHA-256 as lowercase hex.
    //   setSalt            Stores a new salt.
    //   setCredentials     Stores username and hashed password.
    //   checkCredentials   Compares username and hashed password.
    //

    void TokenManager::setValue(const std::string &key, const std::string &value)
    {
        if (_admin)
        {
            (*_admin)[key] = value;
        }

        JsonDocument doc = fileHandler.readJsonConfig(ADMIN_CREDENTIALS_FILE);
        doc[key] = value;
        fileHandler.writeJsonConfig(ADMIN_CREDENTIALS_FILE, doc);
    }

    /*
        > When the key is stored
            - Return it.
        > When the key is missing (first boot)
            - Salt:     generate a random one and store it.
            - User:     return the default user without storing it.
            - Password: hash the default password and store it.
    */
    std::string TokenManager::getValue(const std::string &key)
    {
        if (nullptr == _admin)
        {
            _admin = new JsonDocument();
            *_admin = fileHandler.readJsonConfig(ADMIN_CREDENTIALS_FILE);
        }

        // When the key is stored
        if ((*_admin)[key].is<std::string>())
        {
            return (*_admin)[key].as<std::string>();
        }

        // When the key is missing (first boot)
        if (key == KEY_ADMIN_SALT)
        {
            const std::string salt = fileHandler.randomString(SALT_LENGTH);
            setSalt(salt);
            return salt;
        }
        if (key == KEY_ADMIN_USER)
        {
            return DEFAULT_ADMIN_USER;
        }
        if (key == KEY_ADMIN_PWD)
        {
            const std::string hashed = generateSHA256(DEFAULT_ADMIN_PWD);
            setValue(KEY_ADMIN_PWD, hashed);
            return hashed;
        }

        return "";
    }

    std::string TokenManager::generateSHA256(const std::string &text)
    {
        const std::string salt = getValue(KEY_ADMIN_SALT);

        mbedtls_sha256_context ctx;
        mbedtls_sha256_init(&ctx);
        mbedtls_sha256_starts(&ctx, 0);
        mbedtls_sha256_update(&ctx, (const unsigned char *)text.c_str(), text.length());
        mbedtls_sha256_update(&ctx, (const unsigned char *)salt.c_str(), salt.length());

        unsigned char shaResult[SHA256_LENGTH];
        mbedtls_sha256_finish(&ctx, shaResult);
        mbedtls_sha256_free(&ctx);

        std::string hashStr;
        hashStr.reserve(SHA256_LENGTH * 2);
        for (size_t i = 0; i < SHA256_LENGTH; i++)
        {
            char buf[3];
            snprintf(buf, sizeof(buf), "%02x", shaResult[i]);
            hashStr += buf;
        }
        return hashStr;
    }

    void TokenManager::setSalt(const std::string &salt)
    {
        setValue(KEY_ADMIN_SALT, salt);
    }

    void TokenManager::setCredentials(const std::string &username, const std::string &password)
    {
        const std::string passwordHash = generateSHA256(password);
        setValue(KEY_ADMIN_USER, username);
        setValue(KEY_ADMIN_PWD, passwordHash);
        ESPWEB_LOG_INFO("TokenManager", "Admin credentials updated");
    }

    bool TokenManager::checkCredentials(const std::string &username, const std::string &password)
    {
        return username == getValue(KEY_ADMIN_USER) && generateSHA256(password) == getValue(KEY_ADMIN_PWD);
    }

    // ===========================================================================
    // ============================   TOKEN LOOKUP   =============================
    // ===========================================================================
    //
    //   checkToken   Session tokens first, then API tokens, else a NullToken.
    //

    /*
        Order of precedence:
        - Match to a session token that has not expired (an expired one is removed)
        - Match to an API token
        - NullToken
    */
    Token TokenManager::checkToken(const std::string &authToken)
    {
        // Match to a session token that has not expired
        auto session = _SESSION_TOKENS.find(authToken);
        if (session != _SESSION_TOKENS.end())
        {
            if (uptimeSeconds() <= session->second.expires())
            {
                return session->second;
            }

            ESPWEB_LOG_DEBUG("TokenManager", "Session token expired, removing it");
            _SESSION_TOKENS.erase(session);
        }

        // Match to an API token
        if (nullptr == _API_TOKENS)
        {
            listApiTokens();
        }

        const auto api = _API_TOKENS->find(authToken);
        if (api != _API_TOKENS->end())
        {
            return api->second;
        }

        // NullToken
        return Token::NullToken();
    }

    // ===========================================================================
    // ===========================   SESSION TOKENS   ============================
    // ===========================================================================
    //
    //   getSessionToken      Issues and remembers a new session token.
    //   removeSessionToken   Revokes a session token.
    //

    Token TokenManager::getSessionToken(long seconds, const std::vector<std::string> &actions)
    {
        const Token token = Token::createSessionToken(seconds, actions);
        _SESSION_TOKENS.insert({token.value, token});
        return token;
    }

    void TokenManager::removeSessionToken(const std::string &token)
    {
        _SESSION_TOKENS.erase(token);
    }

    // ===========================================================================
    // =============================   API TOKENS   ==============================
    // ===========================================================================
    //
    //   invalidateApiTokens   Drops the cache.
    //   getApiToken           Creates and stores a named token.
    //   removeApiToken        Removes a named token from the store.
    //   listApiTokens         Loads the store into the cache and lists it.
    //
    // The store is keyed by name, the cache by token value for fast lookups:
    //   store  { "<name>": { "value": "<token>", "action": ["admin"] } }
    //   cache  { "<token>": Token }

    void TokenManager::invalidateApiTokens()
    {
        delete _API_TOKENS;
        _API_TOKENS = nullptr;
    }

    Token TokenManager::getApiToken(const std::string &name, const std::vector<std::string> &action)
    {
        const Token token = Token::createApiToken(name, action);

        JsonDocument doc = fileHandler.readJsonConfig(ADMIN_PERMANENT_TOKENS);

        JsonObject tokenObj = doc[name].to<JsonObject>();
        tokenObj["value"] = token.value;
        JsonArray actionArr = tokenObj["action"].to<JsonArray>();
        for (const std::string &entry : token.action)
        {
            actionArr.add(entry);
        }

        fileHandler.writeJsonConfig(ADMIN_PERMANENT_TOKENS, doc);
        invalidateApiTokens();

        ESPWEB_LOG_INFO("TokenManager", "API token '%s' created", name.c_str());
        return token;
    }

    void TokenManager::removeApiToken(const std::string &name)
    {
        JsonDocument doc = fileHandler.readJsonConfig(ADMIN_PERMANENT_TOKENS);
        doc.remove(name);
        fileHandler.writeJsonConfig(ADMIN_PERMANENT_TOKENS, doc);
        invalidateApiTokens();

        ESPWEB_LOG_INFO("TokenManager", "API token '%s' removed", name.c_str());
    }

    std::vector<Token> TokenManager::listApiTokens()
    {
        if (nullptr == _API_TOKENS)
        {
            _API_TOKENS = new std::map<std::string, Token>();

            const JsonDocument doc = fileHandler.readJsonConfig(ADMIN_PERMANENT_TOKENS);
            for (JsonPairConst kv : doc.as<JsonObjectConst>())
            {
                Token token;
                token.name = kv.key().c_str();
                token.value = kv.value()["value"].as<std::string>();
                token._isApiToken = true;
                for (JsonVariantConst action : kv.value()["action"].as<JsonArrayConst>())
                {
                    token.action.push_back(action.as<std::string>());
                }

                _API_TOKENS->insert({token.value, token});
            }
        }

        std::vector<Token> result;
        result.reserve(_API_TOKENS->size());
        for (const auto &entry : *_API_TOKENS)
        {
            result.push_back(entry.second);
        }

        return result;
    }
}
