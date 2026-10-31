// ESP32 MiniWebServer Framework - https://github.com/LemurDaniel/ESP32__MiniWebServer-Framework
// Copyright © 2026, Daniel Landau
// MIT License

// NOTE:
//   Implementation of utility.wifi.h.

//
//   Chapters:
//     STATE            AP mode, connection and readiness checks.
//     NETWORKS         Scanned, saved and nearby saved networks.
//     SAVED NETWORKS   Add, remove and clear saved networks.
//     CONNECTION       Active network, connect attempt, setup and the manager task.
//

#include <utils/utility.wifi.h>
#include <utils/utility.log.h>

namespace EspWeb
{
    namespace
    {
        /** @brief Interval of the connection check in wifiManagerTask. */
        constexpr uint32_t WIFI_CHECK_INTERVAL_MS = 30000;

        /** @brief Poll interval while waiting for an association. */
        constexpr uint32_t WIFI_CONNECT_POLL_MS = 500;
    }

    // ===========================================================================
    // ================================   STATE   ================================
    // ===========================================================================
    //
    //   isApMode          Whether the device runs only as access point.
    //   isWiFiConnected   Whether the device is associated with a network.
    //   isNetworkReady    Whether clients can reach the device.
    //

    int WiFiUtility::isApMode()
    {
        return WiFi.getMode() == WIFI_AP;
    }

    int WiFiUtility::isWiFiConnected()
    {
        return WiFi.status() == WL_CONNECTED;
    }

    bool WiFiUtility::isNetworkReady()
    {
        return WiFi.isConnected() || WiFi.getMode() == WIFI_AP;
    }

    // ===========================================================================
    // ==============================   NETWORKS   ===============================
    // ===========================================================================
    //
    //   scanNetworks         Every visible network with its signal strength.
    //   getSavedNetworks     Every saved network with its password.
    //   getNearestNetworks   Saved networks in range, strongest first.
    //

    std::vector<WiFiConfig> WiFiUtility::scanNetworks()
    {
        std::vector<WiFiConfig> ssids;

        const int count = WiFi.scanNetworks();
        for (int i = 0; i < count; ++i)
        {
            WiFiConfig config;
            config.ssid = WiFi.SSID(i).c_str();
            config.signalStrength = WiFi.RSSI(i);
            ssids.push_back(config);
        }

        return ssids;
    }

    // Store shape: { "networks": { "<ssid>": { "password": "<password>" } } }
    std::vector<WiFiConfig> WiFiUtility::getSavedNetworks()
    {
        std::vector<WiFiConfig> savedNetworks;

        JsonDocument doc = fileHandler.readJsonConfig(WIFI_CONFIG_FILE);
        for (JsonPair entry : doc["networks"].as<JsonObject>())
        {
            WiFiConfig config;
            config.ssid = entry.key().c_str();
            config.password = entry.value()["password"] | "";
            savedNetworks.push_back(config);
        }

        return savedNetworks;
    }

    std::vector<WiFiConfig> WiFiUtility::getNearestNetworks()
    {
        const std::vector<WiFiConfig> scannedNetworks = scanNetworks();
        const std::vector<WiFiConfig> savedNetworks = getSavedNetworks();
        std::vector<WiFiConfig> nearbyNetworks;

        // Saved network + signal strength of the scan
        for (const WiFiConfig &scanned : scannedNetworks)
        {
            const auto saved = std::find_if(savedNetworks.begin(), savedNetworks.end(),
                                            [&scanned](const WiFiConfig &config)
                                            {
                                                return config.ssid == scanned.ssid;
                                            });
            if (saved == savedNetworks.end())
            {
                continue;
            }

            WiFiConfig match = *saved;
            match.signalStrength = scanned.signalStrength;
            nearbyNetworks.push_back(match);
        }

        std::sort(nearbyNetworks.begin(), nearbyNetworks.end(),
                  [](const WiFiConfig &a, const WiFiConfig &b)
                  {
                      return a.signalStrength > b.signalStrength;
                  });

        return nearbyNetworks;
    }

    // ===========================================================================
    // ===========================   SAVED NETWORKS   ============================
    // ===========================================================================
    //
    //   addWiFiConfig      Saves or updates a network.
    //   removeWiFiConfig   Removes a network.
    //   clearWiFiConfig    Removes every network, also the legacy LittleFS file.
    //

    void WiFiUtility::addWiFiConfig(const std::string &ssid, const std::string &password)
    {
        JsonDocument doc = fileHandler.readJsonConfig(WIFI_CONFIG_FILE);
        doc["networks"][ssid]["password"] = password;
        fileHandler.writeJsonConfig(WIFI_CONFIG_FILE, doc);

        ESPWEB_LOG_INFO("WiFiUtility", "Saved network %s", ssid.c_str());
    }

    void WiFiUtility::removeWiFiConfig(const std::string &ssid)
    {
        JsonDocument doc = fileHandler.readJsonConfig(WIFI_CONFIG_FILE);
        doc["networks"].as<JsonObject>().remove(ssid.c_str());
        fileHandler.writeJsonConfig(WIFI_CONFIG_FILE, doc);

        ESPWEB_LOG_INFO("WiFiUtility", "Removed network %s", ssid.c_str());
    }

    void WiFiUtility::clearWiFiConfig()
    {
        fileHandler.removeFile(WIFI_CONFIG_FILE);
        fileHandler.removeJsonConfig(WIFI_CONFIG_FILE);

        ESPWEB_LOG_INFO("WiFiUtility", "Cleared all saved networks");
    }

    // ===========================================================================
    // =============================   CONNECTION   ==============================
    // ===========================================================================
    //
    //   getActiveWiFi     The connected network with its saved password.
    //   attemptConnect    Connects to one network within the timeout.
    //   setup             Connects to a saved network or falls back to AP mode.
    //   wifiManagerTask   Calls setup periodically.
    //

    WiFiConfig WiFiUtility::getActiveWiFi()
    {
        WiFiConfig activeWiFi;
        if (!isNetworkReady())
        {
            return activeWiFi;
        }

        activeWiFi.ssid = WiFi.SSID().c_str();
        activeWiFi.signalStrength = WiFi.RSSI();
        activeWiFi.ipAddress = WiFi.localIP().toString().c_str();

        for (const WiFiConfig &config : getSavedNetworks())
        {
            if (config.ssid == activeWiFi.ssid)
            {
                activeWiFi.password = config.password;
            }
        }

        return activeWiFi;
    }

    bool WiFiUtility::attemptConnect(WiFiConfig network)
    {
        ESPWEB_LOG_INFO("WiFiUtility", "Connecting to %s (%d dBm)", network.ssid.c_str(), network.signalStrength);

        // Keep an already-running AP alive while attempting to reconnect, so clients
        // currently on the AP (e.g. configuring WiFi via the admin page) aren't dropped.
        WiFi.mode(isApMode() ? WIFI_AP_STA : WIFI_STA);
        WiFi.begin(network.ssid.c_str(), network.password.c_str());

        const int timeStart = millis() / 1000;
        while (!WiFi.isConnected())
        {
            vTaskDelay(pdMS_TO_TICKS(WIFI_CONNECT_POLL_MS));

            if ((int)(millis() / 1000) - timeStart >= _WIFI_TIMEOUT_SEC)
            {
                ESPWEB_LOG_WARN("WiFiUtility", "Connection to %s timed out", network.ssid.c_str());
                WiFi.disconnect();
                return false;
            }
        }

        ESPWEB_LOG_INFO("WiFiUtility", "Connected, IP: %s", WiFi.localIP().toString().c_str());
        return true;
    }

    /*
        > When connected
            - Do nothing.
        > When in AP mode and no saved network is in range
            - Leave the AP untouched, restarting it would drop its clients.
        > Otherwise
            - Try every saved network in range, strongest first, stop on the first success.
            - If all fail and the AP was running, keep it (it stayed alive via WIFI_AP_STA).
            - If all fail and no AP was running, start the AP.
    */
    void WiFiUtility::setup()
    {
        // When connected
        if (isWiFiConnected())
        {
            return;
        }

        // When in AP mode and no saved network is in range
        const bool wasApMode = isApMode();
        const std::vector<WiFiConfig> nearby = getNearestNetworks();
        if (wasApMode && nearby.empty())
        {
            return;
        }

        // Otherwise
        for (const WiFiConfig &network : nearby)
        {
            if (attemptConnect(network))
            {
                return;
            }
        }

        if (wasApMode)
        {
            ESPWEB_LOG_INFO("WiFiUtility", "No connection could be made, staying in AP mode");
            return;
        }

        ESPWEB_LOG_INFO("WiFiUtility", "No saved networks in range, starting AP mode");
        WiFi.mode(WIFI_AP);
        WiFi.softAP(DEFAULT_WIFI_SSID);
        ESPWEB_LOG_INFO("WiFiUtility", "AP %s started, IP: %s", DEFAULT_WIFI_SSID, WiFi.softAPIP().toString().c_str());
    }

    void WiFiUtility::wifiManagerTask(void *param)
    {
        while (true)
        {
            vTaskDelay(pdMS_TO_TICKS(WIFI_CHECK_INTERVAL_MS));
            WiFiUtility::instance().setup();
        }
    }
}
