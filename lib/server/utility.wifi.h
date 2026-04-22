// ESP32 MiniWebServer Framework - https://github.com/LemurDaniel/ESP32__MiniWebServer-Framework
// Copyright © 2026, Daniel Landau
// MIT License

// This is work in progress!

#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <LittleFS.h>

#include <ArduinoJson-v7.4.3.h>

namespace ESP32WebServer
{

    inline bool fileExists(const std::string &file_path)
    {
        if (!LittleFS.begin(true)) // Format if mount fails
        {
            Serial.println("❌ CRITICAL: LittleFS mount failed completely!");
            return false;
        }

        return LittleFS.exists(file_path.c_str());
    }

    inline JsonDocument readJsonFile(const std::string &file_path)
    {
        if (!fileExists(file_path))
        {
            Serial.printf("❌ CRITICAL: JSON file %s not found!\n", file_path.c_str());
            return JsonDocument();
        }

        File file = LittleFS.open(file_path.c_str(), "r");
        if (!file)
        {
            Serial.printf("❌ CRITICAL: Failed to open JSON file %s for reading!\n", file_path.c_str());
            return JsonDocument();
        }

        size_t size = file.size();
        std::string jsonStr(size, '\0');
        file.readBytes(&jsonStr[0], size);
        file.close();

        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, jsonStr);
        if (error)
        {
            Serial.printf("❌ CRITICAL: Failed to parse JSON file %s! Error: %s\n", file_path.c_str(), error.c_str());
            return JsonDocument();
        }

        return doc;
    }

    inline bool writeJsonFile(const std::string &file_path, const JsonDocument &doc)
    {
        if (!LittleFS.begin(true)) // Format if mount fails
        {
            Serial.println("❌ CRITICAL: LittleFS mount failed completely!");
            return false;
        }

        File file = LittleFS.open(file_path.c_str(), "w");
        if (!file)
        {
            Serial.printf("❌ CRITICAL: Failed to open JSON file %s for writing!\n", file_path.c_str());
            return false;
        }

        char jsonStr[512];
        serializeJson(doc, jsonStr, sizeof(jsonStr));

        file.print(jsonStr);
        file.close();

        return true;
    }

    inline void setupWiFi()
    {

        const std::string wifiConfigPath = "/WiFiConfig.json";

        /**
         * Checks for the presence of a WiFi config File.
         * If not found, ESP32 will start in AP mode with SSID "ESP32_MiniWebServer".
         * - An admin page will be available at http://<ESP32_IP>/admin
         * - Credentials and SSID can be set up on the admin page
         * - Default Password for admin:
         *      Username: admin
         *      Password: admin
         *
         * If found, ESP32 will attempt to connect to the WiFi network using the credentials in the file.
         * - The admin page will still be accessible with the user-provided credentials.
         * - The WiFi config and credentials will be stored in the config file on the ESP32
         **/

        if (fileExists(wifiConfigPath))
        {
            JsonDocument wifiConfig = readJsonFile(wifiConfigPath);
            std::string ssid = wifiConfig["ssid"] | "";
            std::string password = wifiConfig["password"] | "";

            WiFi.begin(ssid.c_str(), password.c_str());

            Serial.println();
            Serial.print("Connecting to WiFi...");
            while (WiFi.status() != WL_CONNECTED)
            {
                delay(500);
                Serial.print(".");
            }

            Serial.println("Connected!");
            Serial.printf("IP Address: %s\n", WiFi.localIP().toString().c_str());
            Serial.printf("Signal Strength: %d dBm\n", WiFi.RSSI());
            Serial.println();

            return;
        }

        WiFi.mode(WIFI_AP);
        WiFi.softAP("ESP32_MiniWebServer");
        Serial.println(WiFi.softAPIP()); // TODO: User-Friendly DNS-based access to admin page (e.g. http://esp32.local)
    }

}