

#include <routes/routes.server.manager.h>

#include <HTTPClient.h>
#include <WiFiUdp.h>

namespace routes_server_manager
{
    int Router::isShuttingDown = false;
    int Router::isPoweringUp = false;

    void Router::get_shutdown(const ESP32WebServer::Request &req, ESP32WebServer::Response &res)
    {

        if (isPoweringUp)
        {
            res.status(403).text("Server is powering up");
            return;
        }

        HTTPClient http;
        WiFiClientSecure client;

        // Ignore Self-Signed SSL Certificate
        client.setInsecure();

        http.begin(client, "https://nfs.fritz.box/api/exec/shutdown/");

        // http.addHeader("Authorization", "Basic TODO");

        if (http.GET() == 200)
        {
            isShuttingDown = true;
            res.OK().text("Sucessfully send shutdown");
        }
        else
        {
            res.status(http.GET()).text("Something went wrong");
        }

        http.end();
    }

    void Router::get_startup(const ESP32WebServer::Request &req, ESP32WebServer::Response &res)
    {
        if (isShuttingDown)
        {
            res.status(403).text("Server is shutting down");
            return;
        }

        const std::string macAddress = "10:FF:E0:85:15:8B"; // Hardcoded nfs.fritz.box MAC Adress

        byte mac[6];
        // MAC-String in Byte-Array umwandeln
        sscanf(macAddress.c_str(), "%x:%x:%x:%x:%x:%x", &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);

        byte packet[102];

        // 1. Teil: 6 Bytes mit 0xFF (Sync Stream)
        for (int i = 0; i < 6; i++)
        {
            packet[i] = 0xFF;
        }

        // 2. Teil: Die MAC-Adresse 16 Mal wiederholen
        for (int i = 1; i <= 16; i++)
        {
            memcpy(&packet[i * 6], mac, 6);
        }

        // Paket per UDP an die Broadcast-Adresse senden (Port 9 ist Standard für WOL)
        IPAddress broadcastIP(192, 168, 178, 255);

        WiFiUDP udp;
        udp.beginPacket(broadcastIP, 9);
        udp.write(packet, sizeof(packet));
        udp.endPacket();

        res.OK().text("🪄 Magic Packet was sent");
    }

    void Router::get_status(const ESP32WebServer::Request &req, ESP32WebServer::Response &res)
    {
        // Check if NFS Server is up and service health.

        // TODO check server reachable
        int serverOnline = true;

        if(isShuttingDown && !serverOnline) {
            isShuttingDown = false;
            isPoweringUp = false;
        }
        else if(isPoweringUp && serverOnline) {
            isPoweringUp = false;
            isShuttingDown = false;
        }

        // TODO
    }
}