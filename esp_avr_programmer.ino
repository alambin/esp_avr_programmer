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
        String line     = client.readStringUntil('\r');
        int    httpCmd  = webServ.GetCommand(line);
        String urlParam = webServ.GetUrlParam(line);

        if (httpCmd == webServ.httpCmdIndex) {
            webServ.WSCmdIndex(&client);
        }
        else if (httpCmd == webServ.httpCmdDelete) {
            webServ.WSCmdDelete(&client, urlParam);
        }
        else if (httpCmd == webServ.httpCmdFlash) {
            webServ.WSCmdFlash(&client, urlParam);
        }
        else if (httpCmd == webServ.httpCmdList) {
            webServ.WSCmdList(&client);
        }
        else if (httpCmd == webServ.httpCmdUpload) {
            webServ.WSCmdUpload(&client, urlParam);
        }
        else {
            webServ.WSCmdIndex(&client);
        }

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
