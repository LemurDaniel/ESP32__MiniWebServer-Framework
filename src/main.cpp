#include <Arduino.h>

#include <../lib/server/server.h>

#include <routes/routes.example.h>

void setup()
{
  Serial.begin(115200);

  ESP32WebServer::MiniServer* Server = ESP32WebServer::MiniServer::instance();

  // Will start enter WiFi setup, if this function isn't used.
  // Credentials are permanently stored via LittleFs.
  Server->connectWiFi("FRITZ!Box 6591 TPLink 2,4_EXT2", "***<PASSWORD>***");

  // For testing purposes, remove WiFi config to trigger AP mode
  // Server->clearWiFi();

  Server->index("/web/index.html");
  Server->registerRouter(routes_example::Router());

  Server->start("0.0.0.0", 80);
}

void loop()
{
  while (true)
  {
    delay(10);
  }
}