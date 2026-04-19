

#include <WiFi.h>

namespace custom_utils
{
    WiFiClass connectWiFi(const std::string ssid, const std::string pwd)
    {
        WiFi.mode(WIFI_STA);
        WiFi.begin(ssid.c_str(), pwd.c_str());

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

        return WiFi;
    }

}