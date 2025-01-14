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
RF433 rf433(15, -1);
#else
RF433 rf433(D1, -1);
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

    String output = "<html><head><title>Files</title></head><body>";
    output += "<h1>Files</h1>";
    output += "<table border='1'><tr><th>Filename</th><th>Action</th></tr>";

    File file = root.openNextFile();
    while (file)
    {
        output += "<tr><td>" + String(file.name()) + "</td>";
        output += "<td><a href='/download?file=" + String(file.name()) + "'>Download</a></td></tr>";
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
    if (!LittleFS.begin())
    {
        server.send(500, "text/plain", "Failed to mount filesystem");
        return;
    }

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
    Serial.print("\n\nReady to receive, in 3  ");
    handle_delay(1000);
    Serial.print("2  ");
    handle_delay(1000);
    Serial.print("1  ");
    handle_delay(1000);
    Serial.println("\nStarting receive...");
    String filename = "test_" + String(file_id);
    int result = rf433.recordSignal(filename);
    if (result == 0)
    {
        Serial.println("Receive completed successfully, file written to: " + filename);
        file_id++;
    }
    else
    {
        Serial.print("Receive errored, no file was written, return code: ");
        Serial.println(result);
    }
    handle_delay(2000);
    Serial.print("Current IP address: ");
    Serial.println(WiFi.localIP());
}
