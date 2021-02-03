#include <ESP8266WiFi.h>
#include "WebServ.h"

#define RESET_PIN 4

const char* ssid     = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

WiFiServer server(80);

WebServ webServ(RESET_PIN);

void
setup()
{
    pinMode(RESET_PIN, OUTPUT);
    digitalWrite(RESET_PIN, 1);

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
    }

    server.begin();
}

void
loop()
{
    WiFiClient client = server.available();

    if (client) {
        webServ.ExecuteCommand(&client);

        client.flush();
        while (client.available())
            client.read();
        client.stop();
    }
}

void
formatDevice()
{
    SPIFFS.begin();
    SPIFFS.format();
    SPIFFS.end();
}
