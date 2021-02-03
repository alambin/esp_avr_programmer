#ifndef WEBSERV_H
#define WEBSERV_H

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <FS.h>
#include "IntelHexParse.h"
#include "Stk500.h"
#include "WebServ.h"

class WebServ
{
public:
    enum class HttpCommandType : uint8_t
    {
        Index = 0,
        Flash,
        Upload,
        Delete,
        List
    };

    WebServ(int i);
    void ExecuteCommand(WiFiClient* client);

private:
    HttpCommandType GetCommand(String const& str);
    String          GetUrlParam(String const& str);
    String          HttpSimplePage(String const& text);
    String          HttpRawText(String const& text);

    void WSCmdIndex(WiFiClient* client);
    void WSCmdFlash(WiFiClient* client, String const& filename);
    void WSCmdUpload(WiFiClient* client, String const& filename);
    void WSCmdDelete(WiFiClient* client, String const& filename);
    void WSCmdList(WiFiClient* client);

    String DefaultHeader(bool is_zipped);
    String DefaultFooter();
    void   PrintPage(WiFiClient* client, String const& page);
    String GetDirList();

    int _resetPin = 0;
};

#endif  // WEBSERV_H
