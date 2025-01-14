/*

Simple server that still records RF signals, but also allows for downloading via a web interface.

*IMPORTANT*: Set the `WIFI_SSID` and `WIFI_PASSWORD` to your WiFi network in mysettings.h
You will need to rename mysettings.h.example to mysettings.h and fill in your WiFi details.

*/
#include <Arduino.h>
#include <RF433.h>
#include <WiFi.h>
#include <WebServer.h>
#include "mysettings.h"

#ifdef ESP32
RF433 rf433(-1, 16);
#else
RF433 rf433(-1, D2);
#endif

WebServer server(80);
void handleFileList()
{
    if (!LittleFS.begin())
    {
        server.send(500, "text/plain", "Failed to mount filesystem");
        return;
    }

    File root = LittleFS.open("/signals/");
    if (!root)
    {
        server.send(500, "text/plain", "Failed to open root directory");
        return;
    }

    String output = "<html><head><title>Signal Files</title></head><body>";
    output += "<h1>Signal Files</h1>";
    output += "<table border='1'><tr><th>Filename</th><th>Download</th><th>Send</th></tr>";

    File file = root.openNextFile();
    while (file)
    {
        String fileName = String(file.name());
        // Strip the trailing .txt
        fileName = fileName.substring(0, fileName.length() - 4);
        output += "<tr><td>" + fileName + "</td>";
        output += "<td><a href='/download?file=" + fileName + ".txt'>Download</a></td></tr>";
        output += "<td><a href='/send?file=" + fileName + "'>Send</a></td></tr>";
        file = root.openNextFile();
    }

    output += "</table></body></html>";

    server.send(200, "text/html", output);
}

void handleFileDownload()
{
    if (!server.hasArg("file"))
    {
        server.send(400, "text/plain", "Missing file parameter");
        return;
    }

    String filename = server.arg("file");
    if (!LittleFS.exists("/signals/" + filename))
    {
        server.send(404, "text/plain", "File not found");
        return;
    }

    File file = LittleFS.open("/signals/" + filename, "r");
    if (!file)
    {
        server.send(500, "text/plain", "Failed to open file");
        return;
    }

    server.sendHeader("Content-Disposition", "attachment; filename=\"" + filename + "\"");
    server.sendHeader("Content-Type", "application/octet-stream");
    server.streamFile(file, "application/octet-stream");
    file.close();
}

void handleFileSend()
{
    if (!server.hasArg("file"))
    {
        server.send(400, "text/plain", "Missing file parameter");
        return;
    }

    String filename = server.arg("file");
    int result = rf433.sendSignal(filename);
    if (result == 0)
    {
        server.send(200, "text/plain", "Signal sent successfully");
    }
    else
    {
        server.send(500, "text/plain", "Failed to send signal");
    }
}

void setup()
{
    Serial.begin(115200);
    for (int i = 0; i < 5; i++)
    {
        Serial.print(".");
        delay(1000);
    }

    Serial.print("\nSetup file system:");
    Serial.println(LittleFS.begin(true));

    LittleFS.mkdir("/signals");

    rf433.setup();

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(1000);
        Serial.println("Connecting to WiFi...");
    }
    Serial.println("Connected to WiFi");

    server.on("/", HTTP_GET, handleFileList);
    server.on("/download", HTTP_GET, handleFileDownload);
    server.on("/send", HTTP_GET, handleFileSend);
    server.begin();
    Serial.println("HTTP server started");
}

void handle_delay(uint32_t delay_ms)
{
    uint32_t end = millis() + delay_ms;
    while (millis() < end)
    {
        server.handleClient();
    }
}

int file_id = 0;
void loop()
{
    handle_delay(2000);
    Serial.print("Current IP address: ");
    Serial.println(WiFi.localIP());
}
