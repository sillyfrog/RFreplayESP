/*
  Modified by sillyfrog the original FSWebServer for showing RF sending.

*** IMPORTANT ***
* For this to work as expected, program at 160MHz, not the default 80 MHZ
* This can be selected from Tools > CPU Frequency in the Arduino IDE.
* The overhead of the web server appears to push the time off.
*****************

  Original copyright:

  FSWebServer - Example WebServer with SPIFFS backend for esp8266
  Copyright (c) 2015 Hristo Gochkov. All rights reserved.
  This file is part of the ESP8266WebServer library for Arduino environment.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.
  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

  upload the contents of the data folder with MkSPIFFS Tool ("ESP8266 Sketch Data Upload" in Tools menu in Arduino IDE)
  or you can upload the contents of a folder if you CD in that folder and run the following command:
  for file in `ls -A1`; do curl -F "file=@$PWD/$file" esp8266fs.local/edit; done

  access the sample web page at http://esp8266fs.local
  edit the page by going to http://esp8266fs.local/edit
*/
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <FS.h>
#include <RF433.h>

#define DBG_OUTPUT_PORT Serial

// Set the port the transmitter is connectd to here
RF433 rf433(-1, D2);

/*
 * This file should contain your WiFi Settings.
 * It should be named "mysettings.h", and placed in your project directory.
 * The contents of the file are:

char ssid[] = "xxx";  //  your network SSID (name)
char pass[] = "xxx";       // your network password

 * I do it this way so I can exclude my WiFi settings in my .gitignore file.
 * It also means it's less likely someone will accedently commit their WiFi settings.
 */
#include "mysettings.h"

const char* host = "esp8266fs";



ESP8266WebServer server(80);
//holds the current upload
File fsUploadFile;

//format bytes
String formatBytes(size_t bytes) {
    if (bytes < 1024) {
        return String(bytes)+"B";
    }
    else if (bytes < (1024 * 1024)) {
        return String(bytes/1024.0)+"KB";
    }
    else if (bytes < (1024 * 1024 * 1024)) {
        return String(bytes/1024.0/1024.0)+"MB";
    }
    else {
        return String(bytes/1024.0/1024.0/1024.0)+"GB";
    }
}

String getContentType(String filename) {
    if (server.hasArg("download")) return "application/octet-stream";
    else if (filename.endsWith(".htm")) return "text/html";
    else if (filename.endsWith(".html")) return "text/html";
    else if (filename.endsWith(".css")) return "text/css";
    else if (filename.endsWith(".js")) return "application/javascript";
    else if (filename.endsWith(".png")) return "image/png";
    else if (filename.endsWith(".gif")) return "image/gif";
    else if (filename.endsWith(".jpg")) return "image/jpeg";
    else if (filename.endsWith(".ico")) return "image/x-icon";
    else if (filename.endsWith(".xml")) return "text/xml";
    else if (filename.endsWith(".pdf")) return "application/x-pdf";
    else if (filename.endsWith(".zip")) return "application/x-zip";
    else if (filename.endsWith(".gz")) return "application/x-gzip";
    return "text/plain";
}

bool handleFileRead(String path) {
    DBG_OUTPUT_PORT.println("handleFileRead: " + path);
    if (path.endsWith("/systeminfo")) path = "/index.htm";
    String contentType = getContentType(path);
    String pathWithGz = path + ".gz";
    if (SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)) {
        if (SPIFFS.exists(pathWithGz))
            path += ".gz";
        File file = SPIFFS.open(path, "r");
        size_t sent = server.streamFile(file, contentType);
        file.close();
        return true;
    }
    return false;
}

void handleFileUpload() {
    if (server.uri() != "/edit") return;
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
        String filename = upload.filename;
        if (!filename.startsWith("/")) filename = "/"+filename;
        DBG_OUTPUT_PORT.print("handleFileUpload Name: "); DBG_OUTPUT_PORT.println(filename);
        fsUploadFile = SPIFFS.open(filename, "w");
        filename = String();
    }
    else if (upload.status == UPLOAD_FILE_WRITE) {
        //DBG_OUTPUT_PORT.print("handleFileUpload Data: "); DBG_OUTPUT_PORT.println(upload.currentSize);
        if (fsUploadFile)
            fsUploadFile.write(upload.buf, upload.currentSize);
    }
    else if (upload.status == UPLOAD_FILE_END) {
        if (fsUploadFile)
            fsUploadFile.close();
        DBG_OUTPUT_PORT.print("handleFileUpload Size: "); DBG_OUTPUT_PORT.println(upload.totalSize);
    }
}

void handleFileDelete() {
    if (server.args() == 0) return server.send(500, "text/plain", "BAD ARGS");
    String path = server.arg(0);
    DBG_OUTPUT_PORT.println("handleFileDelete: " + path);
    if (path == "/")
        return server.send(500, "text/plain", "BAD PATH");
    if (!SPIFFS.exists(path))
        return server.send(404, "text/plain", "FileNotFound");
    SPIFFS.remove(path);
    server.send(200, "text/plain", "");
    path = String();
}

void handleFileCreate() {
    if (server.args() == 0)
        return server.send(500, "text/plain", "BAD ARGS");
    String path = server.arg(0);
    DBG_OUTPUT_PORT.println("handleFileCreate: " + path);
    if (path == "/")
        return server.send(500, "text/plain", "BAD PATH");
    if (SPIFFS.exists(path))
        return server.send(500, "text/plain", "FILE EXISTS");
    File file = SPIFFS.open(path, "w");
    if (file)
        file.close();
    else
        return server.send(500, "text/plain", "CREATE FAILED");
    server.send(200, "text/plain", "");
    path = String();
}

void handleFileList() {
    if (!server.hasArg("dir")) {
        server.send(500, "text/plain", "BAD ARGS"); return;
    }

    String path = server.arg("dir");
    DBG_OUTPUT_PORT.println("handleFileList: " + path);
    Dir dir = SPIFFS.openDir(path);
    path = String();

    String output = "[";
    while (dir.next()) {
        File entry = dir.openFile("r");
        if (output != "[") output += ',';
        bool isDir = false;
        output += "{\"type\":\"";
        output += (isDir)?"dir":"file";
        output += "\",\"name\":\"";
        output += String(entry.name()).substring(1);
        output += "\"}";
        entry.close();
    }

    output += "]";
    server.send(200, "text/json", output);
}

void handleRoot() {
    String page = "<html><title>Send RF Signals</title></html><body>";
    page += "<h1>Send RF Signals</h1>";
    page += "<h2>Other Pages</h2>";
    page += "<ul><li><a href='/edit'>View files on SPIFFS</a></li>";
    page += "<li><a href='/systeminfo'>System Info</a></li></ul>";
    page += "<h2>Stored Signals</h2>";

    page += "<ul>\n";
    Dir dir = SPIFFS.openDir("/signals");
    while (dir.next()) {
        page += "<li><a href='/sendsignal?fn=" + dir.fileName();
        page += "'>" + dir.fileName();
        page += "</a></li>\n";
    }
    page += "</ul></body></html>";
    server.send(200, "text/html", page);
}

void handleSendSignal() {
    String fn = server.arg("fn");
    if (fn.length() < 10) {
        // The file name is no where near long enough, error out
        server.send(500, "text/plain", "No valid fn argument provided");
        return;
    }
    int result = rf433.sendSignal(fn);
    if (result == 0) {
        server.send(200, "text/plain", "Signal successfully sent");
    }
    else {
        String out = "Error sending, retun code: ";
        out += result;
        server.send(500, "text/plain", out);
    }
}

uint16_t upData[] ={ 390, 1300, 1235, 455, 390, 1300, 1235, 455, 390, 1300, 1235, 455, 390, 1300, 1235, 455, 1235, 455, 1235, 455, 1235, 455, 1235, 455, 1235, 455, 1235, 455, 390, 1300, 1235, 455, 390, 1300, 390, 1300, 390, 1300, 390, 1300, 390, 1300, 390, 1300, 1235, 455, 1235, 455, 390 };
uint16_t downData[] ={ 390, 1300, 1235, 455, 390, 1300, 1235, 455, 390, 1300, 1235, 455, 390, 1300, 1235, 455, 1235, 455, 1235, 455, 1235, 455, 1235, 455, 1235, 455, 1235, 455, 390, 1300, 1235, 455, 390, 1300, 390, 1300, 1235, 455, 1235, 455, 390, 1300, 390, 1300, 390, 1300, 390, 1300, 390 };
#define DELAY 13100

void handleSendSignalDur() {
    String secondsstr = server.arg("time");
    float milliseconds = secondsstr.toFloat() * 1000;
    Serial.print("Sending for: ");
    Serial.println(milliseconds);

    unsigned long endtime = millis() + milliseconds;
    int result;
    int state=0;
    int _tx_pin = D2;
    unsigned long nexttime = micros();
    while (millis() < endtime) {
        for (int i=0; i<sizeof(downData)/2; i++) {
            state = !state;
            digitalWrite(_tx_pin, state);
            uint16_t ndelay = downData[i];
            nexttime += ndelay;
            while (micros() < nexttime) {
                //pass
            }
        }
        nexttime += DELAY;
        delay(12);
        while (micros() < nexttime) {
            //pass
        }
    }
    if (result == 0) {
        server.send(200, "text/plain", "Signal successfully sent");
    }
    else {
        String out = "Error sending, retun code: ";
        out += result;
        server.send(500, "text/plain", out);
    }
}

void setup(void) {
    DBG_OUTPUT_PORT.begin(115200);
    DBG_OUTPUT_PORT.print("\n");
    DBG_OUTPUT_PORT.setDebugOutput(true);
    SPIFFS.begin();
    {
        Dir dir = SPIFFS.openDir("/");
        while (dir.next()) {
            String fileName = dir.fileName();
            size_t fileSize = dir.fileSize();
            DBG_OUTPUT_PORT.printf("FS File: %s, size: %s\n", fileName.c_str(), formatBytes(fileSize).c_str());
        }
        DBG_OUTPUT_PORT.printf("\n");
    }

    rf433.setup();

    //WIFI INIT
    DBG_OUTPUT_PORT.printf("Connecting to %s\n", ssid);
    if (String(WiFi.SSID()) != String(ssid)) {
        WiFi.begin(ssid, pass);
    }

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        DBG_OUTPUT_PORT.print(".");
    }
    DBG_OUTPUT_PORT.println("");
    DBG_OUTPUT_PORT.print("Connected! IP address: ");
    DBG_OUTPUT_PORT.println(WiFi.localIP());

    MDNS.begin(host);
    DBG_OUTPUT_PORT.print("Open http://");
    DBG_OUTPUT_PORT.print(host);
    DBG_OUTPUT_PORT.println(".local/edit to see the file browser");


    //SERVER INIT
    //list directory
    server.on("/list", HTTP_GET, handleFileList);
    //load editor
    server.on("/edit", HTTP_GET, []() {
        if (!handleFileRead("/edit.htm")) server.send(404, "text/plain", "FileNotInstalled");
        });
    //create file
    server.on("/edit", HTTP_PUT, handleFileCreate);
    //delete file
    server.on("/edit", HTTP_DELETE, handleFileDelete);
    //first callback is called after the request has ended with all parsed arguments
    //second callback handles file uploads at that location
    server.on("/edit", HTTP_POST, []() { server.send(200, "text/plain", ""); }, handleFileUpload);

    //called when the url is not defined here
    //use it to load content from SPIFFS
    server.onNotFound([]() {
        if (!handleFileRead(server.uri()))
            server.send(404, "text/plain", "FileNotFound");
        });

    //get heap status, analog input value and all GPIO statuses in one json call
    server.on("/all", HTTP_GET, []() {
        String json = "{";
        json += "\"heap\":"+String(ESP.getFreeHeap());
        json += ", \"analog\":"+String(analogRead(A0));
        json += ", \"gpio\":"+String((uint32_t)(((GPI | GPO) & 0xFFFF) | ((GP16I & 0x01) << 16)));
        json += "}";
        server.send(200, "text/json", json);
        json = String();
        });

    server.on("/", handleRoot);
    server.on("/sendsignal", handleSendSignal);
    server.on("/sendsignaldur", handleSendSignalDur);

    server.begin();
    DBG_OUTPUT_PORT.println("HTTP server started");

}

void loop(void) {
    server.handleClient();
}
