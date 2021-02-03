#include "WebServ.h"


WebServ::WebServ(int resetPin)
  : resetPin_(resetPin)
{
}

void
WebServ::WSCmdIndex(WiFiClient* client)
{
    SPIFFS.begin();
    File file = SPIFFS.open("/index.htm.gz", "r");

    if (file) {
        int  fs = file.size();
        byte buff[1024];
        int  i = 0;

        client->print(DefaultHeader(true));
        while (fs > 0) {
            i = (fs < 1024) ? fs : 1024;
            file.read(buff, i);
            client->write((const uint8_t*)buff, i);
            fs -= 1024;
        }
        client->print(DefaultFooter());
    }
    else {
        String html = HttpSimplePage(F("spiffs error: index.htm not found."));
        PrintPage(client, html);
    }

    file.close();
    SPIFFS.end();
}

void
WebServ::WSCmdList(WiFiClient* client)
{
    String text = HttpRawText(GetDirList());
    PrintPage(client, text);
}

void
WebServ::WSCmdDelete(WiFiClient* client, String const& filename)
{
    SPIFFS.begin();
    SPIFFS.remove(filename);
    SPIFFS.end();

    String text = HttpRawText(GetDirList());
    PrintPage(client, text);
}

void
WebServ::WSCmdFlash(WiFiClient* client, String const& filename)
{
    Serial.begin(57600);
    // Serial.swap();
    Serial.flush();
    while (Serial.read() != -1)
        ;
    Stk500 stk500(&Serial, resetPin_);

    SPIFFS.begin();

    File file = SPIFFS.open(filename, "r");

    if (file) {
        stk500.setupDevice();
        IntelHexParse hexParse = IntelHexParse();

        while (file.available()) {
            byte   buff[50];
            String data = file.readStringUntil('\n');
            data.getBytes(buff, data.length());
            hexParse.ParseLine(buff);

            if (hexParse.IsPageReady()) {
                byte* page    = hexParse.GetMemoryPage();
                byte* address = hexParse.GetLoadAddress();
                stk500.flashPage(address, page);
            }
        }
    }

    stk500.exitProgMode();
    file.close();
    SPIFFS.end();
}


void
WebServ::WSCmdUpload(WiFiClient* client, String const& filename)
{
    int contentLen = 0;

    while (client->connected()) {
        if (client->available()) {
            String line = client->readStringUntil('\n');

            if (line.startsWith("Content-Length")) {
                contentLen = line.substring(16, (line.length() - 1)).toInt();
            }

            if (line.length() == 1 && line[0] == '\r') {
                SPIFFS.begin();
                String path = "/hex/" + filename;
                File   file = SPIFFS.open(path, "w+");

                if (file) {
                    int i = 0;
                    while (i < contentLen) {
                        file.write(client->read());
                        i++;
                    }
                    file.close();
                }
                SPIFFS.end();

                delay(10);
                String html = HttpSimplePage("DONE");
                client->println(html);
                delay(10);

                break;
            }
        }
    }
}

WebServ::HttpCommandType
WebServ::GetCommand(String const& str)
{
    if (str.startsWith("GET /files")) {
        return HttpCommandType::List;
    }
    else if (str.startsWith("GET /delete")) {
        return HttpCommandType::Delete;
    }
    else if (str.startsWith("GET /flash")) {
        return HttpCommandType::Flash;
    }
    else if (str.startsWith("POST /upload")) {
        return HttpCommandType::Upload;
    }
    else {
        return WebServ::HttpCommandType::Index;
    }
}

String
WebServ::GetUrlParam(String const& str)
{
    String result = "";
    if (str.indexOf("&") > -1) {
        int pStart = str.indexOf("&") + 1;
        int pEnd   = str.indexOf(" ", pStart);
        result     = str.substring(pStart, pEnd);
    }

    return result;
}

void
WebServ::ExecuteCommand(WiFiClient* client)
{
    String line{client->readStringUntil('\r')};
    auto   command    = GetCommand(line);
    auto   parameters = GetUrlParam(line);

    switch (command) {
    case HttpCommandType::Flash:
        WSCmdFlash(client, parameters);
        break;
    case HttpCommandType::Upload:
        WSCmdUpload(client, parameters);
        break;
    case HttpCommandType::Delete:
        WSCmdDelete(client, parameters);
        break;
    case HttpCommandType::List:
        WSCmdList(client);
        break;
    default:
    case HttpCommandType::Index:
        WSCmdIndex(client);
        break;
    }
}

String
WebServ::HttpSimplePage(String const& text)
{
    String html = DefaultHeader(false);
    html += "<!DOCTYPE HTML><html>" + text + "</html>";
    html += DefaultFooter();

    return html;
}

String
WebServ::HttpRawText(String const& text)
{
    String html = DefaultHeader(false);
    html += text;
    html += DefaultFooter();

    return html;
}


String
WebServ::DefaultHeader(bool is_zipped)
{
    if (is_zipped) {
        return String(F("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Encoding: "
                        "gzip\r\nAccess-Control-Allow-Origin: *\r\nConnection: close\r\n\r\n"));
    }
    return String(
        F("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nAccess-Control-Allow-Origin: *\r\nConnection: close\r\n\r\n"));
}

String
WebServ::DefaultFooter()
{
    return String(F("\r\n"));
}

void
WebServ::PrintPage(WiFiClient* client, String const& page)
{
    while (client->connected()) {
        if (client->available()) {
            String line = client->readStringUntil('\r');
            if (line.length() == 1 && line[0] == '\n') {
                client->println(page);
                break;
            }
        }
    }
}

String
WebServ::GetDirList()
{
    String list = "";
    SPIFFS.begin();
    Dir dir = SPIFFS.openDir("/hex");
    while (dir.next()) {
        list += dir.fileName() + ";";
        File f = dir.openFile("r");
        list += String(f.size()) + ";\n";
    }
    SPIFFS.end();
    return list;
}
