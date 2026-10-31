// ESP32 MiniWebServer Framework - https://github.com/LemurDaniel/ESP32__MiniWebServer-Framework
// Copyright © 2026, Daniel Landau
// MIT License

// NOTE:
//   Implementation of utility.file.h.

//
//   Chapters:
//     HELPERS            Mount check, JSON parsing, temp files and string helpers.
//     FOLDERS            List, clear and remove folders.
//     FILES              Info, exists, remove, move and read.
//     JSON STORAGE       JSON documents in NVS and LittleFS.
//     SHARED INSTANCES   The global handler and the framework folders.
//

#include <utils/utility.file.h>
#include <utils/utility.log.h>

namespace EspWeb
{
    namespace
    {
        /** @brief NVS namespace holding every JSON config of the framework. */
        constexpr const char *NVS_NAMESPACE = "espweb";

        /** @brief NVS keys are limited to 15 characters. */
        constexpr size_t NVS_MAX_KEY_LENGTH = 15;

        /** @brief Characters used by randomString(). */
        constexpr const char RANDOM_CHARSET[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    }

    // ===========================================================================
    // ===============================   HELPERS   ===============================
    // ===========================================================================
    //
    //   isMounted       Mounts LittleFS if needed and logs a failure.
    //   parseJson       Parses a JSON string and logs a parse error.
    //   getTempFolder   Returns the temp folder.
    //   getTempFile     Returns a random path inside the temp folder.
    //   hashKey         Hashes any key into a valid NVS key.
    //   randomString    Random alphanumeric string.
    //   trim            Strips spaces and tabs on both ends.
    //   split           Splits and trims by a delimiter.
    //

    namespace
    {
        bool isMounted()
        {
            if (LittleFS.begin())
            {
                return true;
            }

            ESPWEB_LOG_ERROR("FileHandler", "Failed to mount LittleFS");
            return false;
        }

        JsonDocument parseJson(const std::string &json, const std::string &source)
        {
            JsonDocument doc;
            const DeserializationError error = deserializeJson(doc, json);
            if (error)
            {
                ESPWEB_LOG_ERROR("FileHandler", "Failed to parse JSON from %s: %s", source.c_str(), error.c_str());
                return JsonDocument();
            }

            return doc;
        }
    }
    std::string FileHandler::getTempFolder()
    {
        return _tempFolder;
    }

    std::string FileHandler::getTempFile()
    {
        return _tempFolder + "/" + randomString(8);
    }

    // FNV-1a: XOR each byte into the hash, then multiply by the FNV prime.
    // The 16 hex digits are cut to the 15 characters NVS allows.
    std::string FileHandler::hashKey(const std::string &key)
    {
        constexpr uint64_t FNV_OFFSET_BASIS = 14695981039346656037ull;
        constexpr uint64_t FNV_PRIME = 1099511628211ull;

        uint64_t hash = FNV_OFFSET_BASIS;
        for (unsigned char c : key)
        {
            hash = (hash ^ c) * FNV_PRIME;
        }

        char buf[17];
        snprintf(buf, sizeof(buf), "%016llx", hash);
        buf[NVS_MAX_KEY_LENGTH] = '\0';
        return std::string(buf);
    }

    std::string FileHandler::randomString(int size)
    {
        constexpr size_t CHARSET_SIZE = sizeof(RANDOM_CHARSET) - 1;

        std::string result;
        result.reserve(size);
        for (int i = 0; i < size; i++)
        {
            result += RANDOM_CHARSET[random(0, CHARSET_SIZE)];
        }
        return result;
    }

    std::string FileHandler::trim(const std::string &s)
    {
        const size_t start = s.find_first_not_of(" \t");
        if (start == std::string::npos)
        {
            return {};
        }

        const size_t end = s.find_last_not_of(" \t");
        return s.substr(start, end - start + 1);
    }

    // "/api/users/" split by "/" => ["", "api", "users"]
    std::vector<std::string> FileHandler::split(const std::string &text, const std::string &splitter)
    {
        std::vector<std::string> elements;

        size_t pos = 0;
        while (pos < text.size())
        {
            size_t end = text.find(splitter, pos);
            if (end == std::string::npos)
            {
                end = text.size();
            }

            elements.push_back(trim(text.substr(pos, end - pos)));
            pos = end + splitter.size();
        }

        return elements;
    }

    // ===========================================================================
    // ===============================   FOLDERS   ===============================
    // ===========================================================================
    //
    //   listFiles      Entries directly inside a folder.
    //   clearFolder    Deletes the content, creates a missing folder.
    //   removeFolder   Deletes the content and the folder.
    //

    std::vector<FileInfo> FileHandler::listFiles(const std::string &path)
    {
        std::vector<FileInfo> files;
        if (!isMounted())
        {
            return files;
        }

        File folder = LittleFS.open(path.c_str());
        if (!folder || !folder.isDirectory())
        {
            return files;
        }

        std::string filePath = folder.getNextFileName().c_str();
        while (!filePath.empty())
        {
            files.push_back(getInfo(filePath));
            filePath = folder.getNextFileName().c_str();
        }

        return files;
    }

    void FileHandler::clearFolder(const std::string &path)
    {
        if (!isMounted())
        {
            return;
        }

        File dir = LittleFS.open(path.c_str());
        if (!dir)
        {
            LittleFS.mkdir(path.c_str());
            return;
        }

        if (!dir.isDirectory())
        {
            dir.close();
            return;
        }

        // The entry is closed before deleting, LittleFS cannot remove open files.
        File entry = dir.openNextFile();
        while (entry)
        {
            const std::string entryPath = entry.path();
            const bool isDir = entry.isDirectory();
            entry.close();

            if (isDir)
            {
                removeFolder(entryPath);
            }
            else
            {
                LittleFS.remove(entryPath.c_str());
            }

            entry = dir.openNextFile();
        }
        dir.close();
    }

    void FileHandler::removeFolder(const std::string &path)
    {
        clearFolder(path);
        LittleFS.rmdir(path.c_str());
    }

    // ===========================================================================
    // ================================   FILES   ================================
    // ===========================================================================
    //
    //   getInfo      Name, path and extension of an entry.
    //   exists       Whether an entry exists.
    //   removeFile   Deletes a single file.
    //   moveFile     Moves a file into a folder.
    //   readFile     Reads a whole file into a string.
    //

    FileInfo FileHandler::getInfo(const std::string &path)
    {
        FileInfo info;
        if (!isMounted())
        {
            return info;
        }

        File file = LittleFS.open(path.c_str(), "r");
        if (!file)
        {
            ESPWEB_LOG_ERROR("FileHandler", "Failed to open file: %s", path.c_str());
            return info;
        }

        info.isDirectory = file.isDirectory();
        info.name = file.name();
        info.path = file.path();
        file.close();

        // "index.html" => baseName "index", extension "html"
        // ".hidden"    => baseName ".hidden", no extension
        info.baseName = info.name;
        info.extension = "";

        const size_t posDot = info.name.rfind('.');
        if (posDot != std::string::npos && posDot > 0)
        {
            info.extension = info.name.substr(posDot + 1);
            info.baseName = info.name.substr(0, posDot);
        }

        return info;
    }

    bool FileHandler::exists(const std::string &path)
    {
        if (!isMounted())
        {
            return false;
        }

        return LittleFS.exists(path.c_str());
    }

    bool FileHandler::removeFile(const std::string &path)
    {
        if (!isMounted())
        {
            return false;
        }

        if (!LittleFS.exists(path.c_str()))
        {
            ESPWEB_LOG_DEBUG("FileHandler", "No file found at %s to remove", path.c_str());
            return false;
        }

        return LittleFS.remove(path.c_str());
    }

    bool FileHandler::moveFile(const std::string &path, const std::string &destination)
    {
        if (!isMounted())
        {
            return false;
        }

        if (!LittleFS.exists(path.c_str()))
        {
            ESPWEB_LOG_WARN("FileHandler", "No file found at %s to move", path.c_str());
            return false;
        }

        const FileInfo info = getInfo(path);
        const std::string destinationPath = destination + "/" + info.name;

        ESPWEB_LOG_DEBUG("FileHandler", "Move from %s to %s", path.c_str(), destinationPath.c_str());
        return LittleFS.rename(path.c_str(), destinationPath.c_str());
    }

    std::string FileHandler::readFile(const std::string &path)
    {
        if (!isMounted())
        {
            return "";
        }

        // A missing file is expected, e.g. when readJsonConfig looks for an old config to migrate.
        if (!LittleFS.exists(path.c_str()))
        {
            ESPWEB_LOG_DEBUG("FileHandler", "File %s not found", path.c_str());
            return "";
        }

        File file = LittleFS.open(path.c_str(), "r");
        if (!file)
        {
            ESPWEB_LOG_ERROR("FileHandler", "Failed to open file %s for reading", path.c_str());
            return "";
        }

        const size_t size = file.size();
        std::string content(size, '\0');
        file.readBytes(&content[0], size);
        file.close();

        return content;
    }

    // ===========================================================================
    // ============================   JSON STORAGE   =============================
    // ===========================================================================
    //
    //   readJsonConfig     Reads from NVS, migrates an old LittleFS file.
    //   writeJsonConfig    Writes to NVS.
    //   removeJsonConfig   Removes from NVS.
    //   readJsonFile       Reads a JSON file from LittleFS.
    //   writeJsonFile      Writes a JSON file to LittleFS.
    //

    /*
        > When the NVS key holds a value
            - Parse and return it.
        > When the NVS key is empty (backwards compatibility)
            - If a LittleFS file exists at the key path, parse it and migrate it into NVS.
            - Otherwise return an empty document.
    */
    JsonDocument FileHandler::readJsonConfig(const std::string &key)
    {
        Preferences prefs;
        if (!prefs.begin(NVS_NAMESPACE, false))
        {
            ESPWEB_LOG_ERROR("FileHandler", "Failed to open NVS namespace %s", NVS_NAMESPACE);
            return JsonDocument();
        }

        const std::string entry = prefs.getString(hashKey(key).c_str(), "").c_str();
        prefs.end();

        // When the NVS key holds a value
        if (!entry.empty())
        {
            return parseJson(entry, key);
        }

        // When the NVS key is empty (backwards compatibility)
        const std::string legacyEntry = readFile(key);
        if (legacyEntry.empty())
        {
            return JsonDocument();
        }

        JsonDocument doc = parseJson(legacyEntry, key);
        if (doc.isNull())
        {
            return doc;
        }

        writeJsonConfig(key, doc);
        ESPWEB_LOG_INFO("FileHandler", "Migrated JSON from LittleFS file to NVS key %s", key.c_str());
        return doc;
    }

    void FileHandler::writeJsonConfig(const std::string &key, const JsonDocument &doc)
    {
        std::string jsonStr;
        serializeJson(doc, jsonStr);

        Preferences prefs;
        if (!prefs.begin(NVS_NAMESPACE, false))
        {
            ESPWEB_LOG_ERROR("FileHandler", "Failed to open NVS namespace %s", NVS_NAMESPACE);
            return;
        }

        const size_t written = prefs.putString(hashKey(key).c_str(), jsonStr.c_str());
        prefs.end();

        if (written != jsonStr.length())
        {
            ESPWEB_LOG_ERROR("FileHandler", "Failed to write JSON to NVS key %s (wrote %u of %u bytes)", key.c_str(),
                             (unsigned)written, (unsigned)jsonStr.length());
            return;
        }

        ESPWEB_LOG_DEBUG("FileHandler", "Wrote JSON to NVS key %s", key.c_str());
    }

    void FileHandler::removeJsonConfig(const std::string &key)
    {
        Preferences prefs;
        if (!prefs.begin(NVS_NAMESPACE, false))
        {
            ESPWEB_LOG_ERROR("FileHandler", "Failed to open NVS namespace %s", NVS_NAMESPACE);
            return;
        }

        prefs.remove(hashKey(key).c_str());
        prefs.end();
    }

    JsonDocument FileHandler::readJsonFile(const std::string &path)
    {
        const std::string jsonStr = readFile(path);
        if (jsonStr.empty())
        {
            return JsonDocument();
        }

        return parseJson(jsonStr, path);
    }

    void FileHandler::writeJsonFile(const std::string &path, const JsonDocument &doc)
    {
        if (!isMounted())
        {
            return;
        }

        File file = LittleFS.open(path.c_str(), "w", true);
        if (!file)
        {
            ESPWEB_LOG_ERROR("FileHandler", "Failed to open JSON file %s for writing", path.c_str());
            return;
        }

        std::string jsonStr;
        serializeJson(doc, jsonStr);

        file.print(jsonStr.c_str());
        file.close();

        ESPWEB_LOG_DEBUG("FileHandler", "Wrote JSON file %s", path.c_str());
    }

    // ===========================================================================
    // ==========================   SHARED INSTANCES   ===========================
    // ===========================================================================
    //
    //   fileHandler   The global FileHandler.
    //   FOLDER_TEMP   Folder for temporary uploads.
    //   FOLDER_WEB    Root folder of the hosted website.
    //

    FileHandler fileHandler;
    std::string FOLDER_TEMP = "/tmp";
    std::string FOLDER_WEB = "/web";
}
