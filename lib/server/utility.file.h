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

    const std::string TEMP_FOLDER = "/tmp";

    inline bool fileExists(const std::string &filePath)
    {
        return LittleFS.exists(filePath.c_str());
    }

    struct FileInfo
    {
        std::string name;
        std::string path;
        std::string extension;
        std::string baseName;
    };

    /*-------------------------------------------------------------------------------------------------
     *
     * Folder and Folder Contents
     *
     **/

    inline std::string getTempFolder()
    {
        return TEMP_FOLDER + std::to_string(millis());
    }

    inline std::vector<FileInfo> listFiles(const std::string &folderPath, std::vector<FileInfo> &files, const std::string &prefix = "")
    {
        File folder = LittleFS.open(folderPath.c_str());
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

    inline std::vector<FileInfo> listFiles(const std::string &folderPath)
    {
        std::vector<FileInfo> files;
        return listFiles(folderPath, files);
    }

    static void clearFolder(const std::string &folderPath)
    {
        File dir = LittleFS.open(folderPath.c_str());
        if (!dir || !dir.isDirectory())
        {
            LittleFS.mkdir(folderPath.c_str());
            return;
        }
        File entry = dir.openNextFile();
        while (entry)
        {
            LittleFS.remove(entry.path());
            entry = dir.openNextFile();
        }
        dir.close();
    }

    /*-------------------------------------------------------------------------------------------------
     *
     * Handle Files
     *
     **/

    inline int removeFile(const std::string &filePath)
    {
        if (fileExists(filePath))
        {
            if (LittleFS.remove(filePath.c_str()))
            {
                Serial.printf("✅ Successfully removed file %s\n", filePath.c_str());
                return 0;
            }
            else
            {
                Serial.printf("❌ CRITICAL: Failed to remove file %s!\n", filePath.c_str());
                return -1;
            }
        }
        else
        {
            Serial.printf("ℹ️ No file found at %s to remove.\n", filePath.c_str());
            return -1;
        }
    }

    /*-------------------------------------------------------------------------------------------------
     *
     * Read and write JSON
     *
     **/

    inline JsonDocument readJsonFile(const std::string &filePath)
    {
        if (!fileExists(filePath))
        {
            Serial.printf("❌ CRITICAL: JSON file %s not found!\n", filePath.c_str());
            return JsonDocument();
        }

        File file = LittleFS.open(filePath.c_str(), "r");
        if (!file)
        {
            Serial.printf("❌ CRITICAL: Failed to open JSON file %s for reading!\n", filePath.c_str());
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
            Serial.printf("❌ CRITICAL: Failed to parse JSON file %s! Error: %s\n", filePath.c_str(), error.c_str());
            return JsonDocument();
        }

        return doc;
    }

    inline bool writeJsonFile(const std::string &filePath, const JsonDocument &doc)
    {
        File file = LittleFS.open(filePath.c_str(), "w");
        if (!file)
        {
            throw "CRITICAL: Failed to open File to write";
        }

        std::string jsonStr;
        serializeJson(doc, jsonStr);

        file.print(jsonStr.c_str());
        file.close();

        Serial.printf("✅ Successfully wrote JSON file %s\n", filePath.c_str());

        return true;
    }

}