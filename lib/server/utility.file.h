// ESP32 MiniWebServer Framework - https://github.com/LemurDaniel/ESP32__MiniWebServer-Framework
// Copyright © 2026, Daniel Landau
// MIT License

// This is work in progress!

#pragma once

#include <Arduino.h>
#include <LittleFS.h>

#include <ArduinoJson-v7.4.3.h>

namespace ESP32WebServer
{

    inline bool fileExists(const std::string &file_path)
    {
        return LittleFS.exists(file_path.c_str());
    }

    struct FileInfo
    {
        std::string name;
        std::string path;
        std::string extension;
        std::string baseName;
    };

    inline std::vector<FileInfo> listFiles(const std::string &folder_path, std::vector<FileInfo> &files, const std::string &prefix = "")
    {
        File folder = LittleFS.open(folder_path.c_str());
        if (!folder || !folder.isDirectory())
        {
            throw std::runtime_error("Path is not a directory");
        }

        File file = folder.openNextFile();
        while (file)
        {
            if (file.isDirectory())
            {
                listFiles(folder.path(), files, prefix + "/" + folder.name());
                continue;
            }

            FileInfo info;
            info.name = file.name();
            info.path = file.path();
            info.baseName = info.name;
            info.extension = "";

            size_t dot = info.name.find_last_of('.');
            if (dot != std::string::npos && dot > 0)
            {
                info.extension = info.name.substr(dot);
                info.baseName = info.name.substr(0, dot);
            }

            files.push_back(info);
            file = folder.openNextFile();
        }

        return files;
    }

    inline std::vector<FileInfo> listFiles(const std::string &folder_path)
    {
        std::vector<FileInfo> files;
        return listFiles(folder_path, files);
    }

    inline int removeFile(const std::string &file_path)
    {
        if (fileExists(file_path))
        {
            if (LittleFS.remove(file_path.c_str()))
            {
                Serial.printf("✅ Successfully removed file %s\n", file_path.c_str());
                return 0;
            }
            else
            {
                Serial.printf("❌ CRITICAL: Failed to remove file %s!\n", file_path.c_str());
                return -1;
            }
        }
        else
        {
            Serial.printf("ℹ️ No file found at %s to remove.\n", file_path.c_str());
            return -1;
        }
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
        File file = LittleFS.open(file_path.c_str(), "w");
        if (!file)
        {
            throw "CRITICAL: Failed to open File to write";
        }

        char jsonStr[512];
        serializeJson(doc, jsonStr, sizeof(jsonStr));

        file.print(jsonStr);
        file.close();

        Serial.printf("✅ Successfully wrote JSON file %s\n", file_path.c_str());

        return true;
    }

}