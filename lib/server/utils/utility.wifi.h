// ESP32 MiniWebServer Framework - https://github.com/LemurDaniel/ESP32__MiniWebServer-Framework
// Copyright © 2026, Daniel Landau
// MIT License

// NOTE:
//   WiFi connection management: saved networks, STA connect and the AP fallback.
//   Requires: NVS (Preferences) for the saved networks.
//
//   EspWeb::WiFiUtility::instance().addWiFiConfig("<SSID>", "<PASSWORD>");
//   EspWeb::WiFiUtility::instance().setup();

//
//   Chapters:
//     SETTINGS      Store key and the AP SSID.
//     WIFI CONFIG   One saved or scanned network.
//     MANAGE WIFI   The WiFiUtility singleton.
//

#pragma once

#include <Arduino.h>
#include <ESPmDNS.h>
#include <LittleFS.h>
#include <WiFi.h>

#include <algorithm>
#include <string>
#include <vector>

#include <utils/utility.file.h>

// ===========================================================================
// ==============================   SETTINGS   ===============================
// ===========================================================================
//
//   WIFI_CONFIG_FILE    Store key of the saved networks.
//   DEFAULT_WIFI_SSID   SSID of the fallback access point.
//

// The trailing space is part of the key: it is hashed into the NVS key of existing devices.
// Removing it orphans every saved network.
/** @brief Store key for the saved WiFi network configuration. */
#define WIFI_CONFIG_FILE "/WiFiConfig.json "

/** @brief Default AP-mode SSID used when no saved network is available. */
#define DEFAULT_WIFI_SSID "ESP32_MiniWebServer"

namespace EspWeb
{
    // ===========================================================================
    // =============================   WIFI CONFIG   =============================
    // ===========================================================================
    //
    //   WiFiConfig   SSID, password, signal strength and IP of a network.
    //

    /**
     * @brief Describes a WiFi network configuration entry.
     */
    struct WiFiConfig
    {
        /** @brief Network SSID. */
        std::string ssid;

        /** @brief Network password. */
        std::string password;

        /** @brief Signal strength in dBm (populated during scans; 0 if not scanned). */
        int signalStrength = 0;

        /** @brief Current IP address string; `"Not Connected"` when not associated. */
        std::string ipAddress = "Not Connected";
    };

    // ===========================================================================
    // =============================   MANAGE WIFI   =============================
    // ===========================================================================
    //
    //   Singleton        instance, the only way to reach the utility.
    //   State            AP mode, connection and readiness checks.
    //   Networks         Scanned, saved and nearby saved networks.
    //   Saved networks   Add, remove and clear saved networks.
    //   Connection       Active network, connect attempt, setup and the manager task.
    //

    /**
     * @brief Singleton utility for WiFi connection management and AP/STA mode control.
     *
     * Continuously monitors the connection state in a background FreeRTOS task and
     * reconnects to the nearest saved network on failure. Falls back to AP mode when
     * no saved network is reachable. Access via WiFiUtility::instance().
     */
    class WiFiUtility
    {
        // Singleton
    public:
        /**
         * @brief Returns the singleton WiFiUtility instance.
         * @return Reference to the global WiFiUtility.
         */
        static WiFiUtility &instance()
        {
            static WiFiUtility inst;
            return inst;
        }

    private:
        WiFiUtility() = default;

        // State
    public:
        /**
         * @brief Checks whether the device is currently in AP (access point) mode.
         * @return Non-zero if in AP mode, 0 otherwise.
         */
        int isApMode();

        /**
         * @brief Checks whether the device has an active WiFi association.
         * @return Non-zero if connected to a WiFi network, 0 otherwise.
         */
        int isWiFiConnected();

        /**
         * @brief Checks whether the network can serve clients (connected or in AP mode).
         * @return `true` if connected to a network or running as access point.
         */
        bool isNetworkReady();

        // Networks
    public:
        /**
         * @brief Performs a blocking WiFi scan and returns all visible networks.
         * @return Vector of WiFiConfig entries for all detected networks.
         */
        std::vector<WiFiConfig> scanNetworks();

        /**
         * @brief Loads all saved network configurations from NVS.
         * @return Vector of saved WiFiConfig entries (no signal strength data).
         */
        std::vector<WiFiConfig> getSavedNetworks();

        /**
         * @brief Returns saved networks that are currently detectable, sorted by signal strength.
         * @return Vector of WiFiConfig entries present in both saved list and current scan.
         */
        std::vector<WiFiConfig> getNearestNetworks();

        // Saved networks
    public:
        /**
         * @brief Saves a WiFi network configuration, replacing the password of a known SSID.
         * @param ssid     Network SSID.
         * @param password Network password.
         */
        void addWiFiConfig(const std::string &ssid, const std::string &password);

        /**
         * @brief Removes a saved WiFi network configuration.
         * @param ssid SSID of the network to remove.
         */
        void removeWiFiConfig(const std::string &ssid);

        /**
         * @brief Removes all saved WiFi credentials, causing AP-mode fallback on next setup().
         */
        void clearWiFiConfig();

        // Connection
    public:
        /**
         * @brief Returns the currently active WiFi connection details.
         * @return WiFiConfig with SSID, IP address, and signal strength of the active connection,
         *         or a default entry if not connected.
         */
        WiFiConfig getActiveWiFi();

        /**
         * @brief Attempts to connect to a specific WiFi network.
         * @param network WiFiConfig containing the SSID and password to connect to.
         * @return `true` if the connection succeeded within the timeout, `false` otherwise.
         */
        bool attemptConnect(WiFiConfig network);

        /**
         * @brief Connects to the strongest saved network in range, or falls back to AP mode.
         *
         * Connection routine:
         * 1. Return early when already connected.
         * 2. Try every saved network in range, strongest first.
         * 3. Fall back to AP mode (`DEFAULT_WIFI_SSID`) if no connection could be made.
         *    A running AP is kept, so its clients are not dropped.
         */
        void setup();

        /**
         * @brief FreeRTOS task that calls setup() every 30 seconds to restore connectivity.
         * @param param Unused.
         */
        static void wifiManagerTask(void *param);

    private:
        /** @brief Seconds to wait for a WiFi association attempt before giving up. */
        const int _WIFI_TIMEOUT_SEC = 30;
    };
}
