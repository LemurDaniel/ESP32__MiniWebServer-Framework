#include <Arduino.h>

#include <../lib/utilities.h>
#include <../lib/server/server.h>

#include <routes/routes.example.h>

ESP32WebServer::MiniServer Server = ESP32WebServer::MiniServer("0.0.0.0", 80);

void setup()
{
  Serial.begin(115200);

  custom_utils::connectWiFi("FRITZ!Box 6591 TPLink 2,4_EXT2", "**SECRET PASSWORD**");

  Server.index("/web/index.html");
  Server.registerRouter(routes_example::Router());
}

void loop()
{ 
  Serial.println("Waiting for clients...");
  // Main server loop
  Server.listenClient();
}