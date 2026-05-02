// ESP32 MiniWebServer Framework - https://github.com/LemurDaniel/ESP32__MiniWebServer-Framework
// Copyright © 2026, Daniel Landau
// MIT License

#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <LittleFS.h>

#include <vector>
#include <string>
#include <algorithm>

#include <utility.file.h>

#define WIFI_CONFIG_FILE "/WiFiConfig.json"

namespace ESP32WebServer
{

    struct WiFiConfig
    {
        std::string ssid;
        std::string password;
        int signalStrength = 0;
        std::string ipAddress = "Not Connected";
    };

    class WiFiUtility
    {
    private:
        WiFiUtility() = default;

        const std::string DEFAULT_WIFI_SSID = "ESP32_MiniWebServer";
        const int WIFI_TIMEOUT_SEC = 30;

    public:
        static WiFiUtility &instance()
        {
            static WiFiUtility inst;
            return inst;
        }

        int isApMode()
        {
            return WiFi.getMode() == WIFI_AP;
        }

        int isWiFiConnected()
        {
            return WiFi.status() == WL_CONNECTED;
        }

        bool isNetworkReady()
        {
            return WiFi.isConnected() || WiFi.getMode() == WIFI_AP;
        }

        void clearWiFiConfig()
        {
            removeFile(WIFI_CONFIG_FILE);
        }

        /*-------------------------------------------------------------------------------------------------
         *
         * Handle Saved / Nearby Networks
         *
         **/

        std::vector<WiFiConfig> scanNetworks()
        {
            std::vector<WiFiConfig> ssids;

            int n = WiFi.scanNetworks();
            for (int i = 0; i < n; ++i)
            {
                WiFiConfig config;
                config.ssid = WiFi.SSID(i).c_str();
                config.signalStrength = WiFi.RSSI(i);
                ssids.push_back(config);
            }

            return ssids;
        }

        std::vector<WiFiConfig> getSavedNetworks()
        {
            std::vector<WiFiConfig> savedNetworks;

            if (!fileExists(WIFI_CONFIG_FILE))
                return savedNetworks;

            JsonDocument doc = readJsonFile(WIFI_CONFIG_FILE);
            for (JsonPair entry : doc["networks"].as<JsonObject>())
            {
                WiFiConfig config;
                config.ssid = entry.key().c_str();
                config.password = entry.value()["password"] | "";
                savedNetworks.push_back(config);
            }

            return savedNetworks;
        }

        void addWiFiConfig(const std::string &ssid, const std::string &password)
        {
            JsonDocument doc;
            if (fileExists(WIFI_CONFIG_FILE))
                doc = readJsonFile(WIFI_CONFIG_FILE);

            doc["networks"][ssid]["password"] = password;

            writeJsonFile(WIFI_CONFIG_FILE, doc);
        }

        void removeWiFiConfig(const std::string &ssid)
        {
            if (!fileExists(WIFI_CONFIG_FILE))
                return;

            JsonDocument doc = readJsonFile(WIFI_CONFIG_FILE);
            doc["networks"].as<JsonObject>().remove(ssid.c_str());
            writeJsonFile(WIFI_CONFIG_FILE, doc);
        }

        WiFiConfig getActiveWiFi()
        {

            WiFiConfig activeWiFi;
            if (!isNetworkReady())
            {
                return activeWiFi;
            }

            activeWiFi.ssid = WiFi.SSID().c_str();
            activeWiFi.signalStrength = WiFi.RSSI();
            activeWiFi.ipAddress = WiFi.localIP().toString().c_str();

            std::vector<WiFiConfig> savedNetworks = getSavedNetworks();
            for (const WiFiConfig &config : savedNetworks)
            {
                if (config.ssid != activeWiFi.ssid)
                    continue;
                activeWiFi.password = config.password;
            }

            return activeWiFi;
        }

        std::vector<WiFiConfig> getNearestNetworks()
        {
            const std::vector<WiFiConfig> &scannedNetworks = scanNetworks();
            std::map<std::string, WiFiConfig> savedNetworks;
            std::vector<WiFiConfig> nearbyNetworks;

            for (const WiFiConfig &saved : getSavedNetworks())
            {
                savedNetworks.insert({saved.ssid, saved});
            }

            for (const WiFiConfig &scanned : scannedNetworks)
            {
                const auto &entry = savedNetworks.find(scanned.ssid);
                if (entry == savedNetworks.end())
                {
                    continue;
                }

                WiFiConfig match = entry->second;
                match.signalStrength = scanned.signalStrength;
                nearbyNetworks.push_back(match);
                break;
            }

            std::sort(nearbyNetworks.begin(), nearbyNetworks.end(), [](const WiFiConfig &a, const WiFiConfig &b)
                      { return a.signalStrength > b.signalStrength; });

            return nearbyNetworks;
        }

        /*-------------------------------------------------------------------------------------------------

            Setup Wifi


            Continually checks every 30 seconds if the device is connected to a WiFi.
            If it is NOT, then attempts its connection routine:

            -> Find the nearest Saved network and connect to it
            -> On Failure try the next network

            -> Fallback to AP-Mode if
                -> No Connection could be made
                -> No saved network is nearby


        */
        bool attemptConnect(WiFiConfig network)
        {
            Serial.printf("Connecting to: %s (%d dBm)\n", network.ssid.c_str(), network.signalStrength);

            WiFi.mode(WIFI_STA);
            WiFi.begin(network.ssid.c_str(), network.password.c_str());

            const int timeStart = millis() / 1000;
            while (!WiFi.isConnected())
            {
                vTaskDelay(pdMS_TO_TICKS(500));
                Serial.print(".");

                if ((int)(millis() / 1000) - timeStart >= WIFI_TIMEOUT_SEC)
                {
                    Serial.println("\nConnection timed out.");
                    WiFi.disconnect();
                    return false;
                }
            }

            Serial.printf("\nConnected! IP: %s\n", WiFi.localIP().toString().c_str());
            return true;
        }

        void setup()
        {
            if (isNetworkReady())
            {
                return;
            }

            std::vector<WiFiConfig> nearby = getNearestNetworks();
            for (const WiFiConfig &network : nearby)
            {
                if (attemptConnect(network))
                {
                    return;
                }
            }

            if (isNetworkReady())
            {
                return;
            }

            if (nearby.empty())
            {
                Serial.println("No saved networks in range, starting AP mode...");
            }
            else
            {
                Serial.println("No Connection could be made, starting in AP mode...");
            }

            WiFi.mode(WIFI_AP);
            WiFi.softAP(DEFAULT_WIFI_SSID.c_str());
            Serial.printf("AP IP: %s\n", WiFi.softAPIP().toString().c_str());
        }

        static void wifiManagerTask(void *param)
        {

            while (true)
            {
                vTaskDelay(pdMS_TO_TICKS(30000));
                WiFiUtility::instance().setup();
            }
        }
    };
}