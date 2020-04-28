/*
*   
*    Copyright (C) 2017  CS.NOL  https://github.com/csnol/STM32-OTA
*
*    This program is free software: you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation, either version 3 of the License, 
*    and You have to keep below webserver code 
*    "<h2>Version 1.0 by <a style=\"color:white\" href=\"https://github.com/csnol/STM32-OTA\">CSNOL" 
*    in your sketch.
*
*    This program is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.
*
*    You should have received a copy of the GNU General Public License
*    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*    
*   It is assumed that the STM32 MCU is connected to the following pins with NodeMCU or ESP8266/8285. 
*   Tested and supported MCU : STM32F03xF/K/C/，F05xF/K/C,F10xx8/B
*
*   连接ESP8266模块和STM32系列MCU.    Connect ESP8266 to STM32 MCU
*
*   ESP8266/8285 Pin       STM32 MCU      NodeMCU Pin(ESP8266 based)
*   RXD                  	PA9             RXD
*   TXD                  	PA10            TXD
*   Pin4                 	BOOT0           D2
*   Pin5                 	RST             D1
*   Vcc                  	3.3V            3.3V
*   GND                  	GND             GND
*   En -> 10K -> 3.3V
*
*/

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
// #include "spiffs/spiffs.h"            // Delete for ESP8266-Arduino 2.4.2 version
#include <FS.h>
#include <ESP8266mDNS.h>
#include "stm32ota.h"

const String STM32_CHIPNAME[8] = {
  "Unknown Chip",
  "STM32F03xx4/6",
  "STM32F030x8/05x",
  "STM32F030xC",
  "STM32F103x4/6",
  "STM32F103x8/B",
  "STM32F103xC/D/E",
  "STM32F105/107"
};

#define NRST 5
#define BOOT0 4
#define LED LED_BUILTIN

const char* ssid = "Avilon";
const char* password = "avilon11";

IPAddress local_IP(192, 168, 1, 102);
IPAddress gateway(192,168 , 1, 1);
IPAddress subnet(255, 255, 255, 0);

ESP8266WebServer server(80);
File fsUploadFile;
uint8_t binread[256];
int bini = 0;
String stringtmp;
int rdtmp;


void handleFlash()
{
  String FileName, flashwr;
  int lastbuf = 0;
  uint8_t cflag, fnum = 256;
  Dir dir = SPIFFS.openDir("/");
  while (dir.next())
  {
    FileName = dir.fileName();
  }
  fsUploadFile = SPIFFS.open(FileName, "r");
  if (fsUploadFile) {
    bini = fsUploadFile.size() / 256;
    lastbuf = fsUploadFile.size() % 256;
    flashwr = String(bini) + "-" + String(lastbuf)+": ";
    for (int i = 0; i < bini; i++) {
      fsUploadFile.read(binread, 256);
      stm32SendCommand(STM32WR);
      while (!Serial.available()) ;
      cflag = Serial.read();
      if (cflag == STM32ACK)
        if (stm32Address(STM32STADDR + (256 * i)) == STM32ACK) {
          if (stm32SendData(binread, 255) == STM32ACK);        
          else flashwr = "Error";
        }
    }
    fsUploadFile.read(binread, lastbuf);
    stm32SendCommand(STM32WR);
    while (!Serial.available()) ;
    cflag = Serial.read();
    if (cflag == STM32ACK)
      if (stm32Address(STM32STADDR + (256 * bini)) == STM32ACK) {
        if (stm32SendData(binread, lastbuf) == STM32ACK)
          flashwr += "Finished";
        else flashwr = "Error";
      }
    fsUploadFile.close();
    server.send(200, "text/plain", "Flash: "+flashwr);
  }
}


void handleFileUpload()
{
  if (server.uri() != "/upload") return;
  HTTPUpload& upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    String filename = upload.filename;
    if (!filename.startsWith("/")) filename = "/" + filename;
    fsUploadFile = SPIFFS.open(filename, "w");
    filename = String();
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (fsUploadFile)
      fsUploadFile.write(upload.buf, upload.currentSize);
  } else if (upload.status == UPLOAD_FILE_END) {
    if (fsUploadFile)
      fsUploadFile.close();
  }
}

void handleFileDelete() {
  String FileList = "File: ";
  String FName;
  Dir dir = SPIFFS.openDir("/");
  while (dir.next()) {
    FName = dir.fileName();
  }
  FileList += FName;
  if (SPIFFS.exists(FName)) {
    server.send(200, "text/plain", "Deleted : " + FileList);
    SPIFFS.remove(FName);
  }
  else
    return server.send(404, "text/plain", "File Not found: 404");
}


void handleListFiles()
{
  String FileList = "Bootloader Ver: ";
  String Listcode;
  char blversion = 0;
  Dir dir = SPIFFS.openDir("/");
  blversion = stm32Version();
  FileList += String((blversion >> 4) & 0x0F) + "." + String(blversion & 0x0F) + "<br> MCU: ";
  FileList += STM32_CHIPNAME[stm32GetId()];
  FileList += "  File: ";
  while (dir.next())
  {
    String FileName = dir.fileName();
    File f = dir.openFile("r");
    String FileSize = String(f.size());
    int whsp = 6 - FileSize.length();
    while (whsp-- > 0)
    {
      FileList += " ";
    }
    FileList +=  FileName + "   Size:" + FileSize;
  }
  server.send(200, "text/plain", "FileList: "+  FileList );
}

void setup(void)
{
  SPIFFS.begin();
  Serial.begin(115200, SERIAL_8E1);
  pinMode(BOOT0, OUTPUT);
  pinMode(NRST, OUTPUT);
  pinMode(LED, OUTPUT);
  
  WiFi.mode(WIFI_STA);
  WiFi.config(local_IP, gateway, subnet);
  WiFi.begin(ssid, password);
  RunMode();
  delay(500);
  
  for ( int i = 0; i < 3; i++) {
    digitalWrite(LED, !digitalRead(LED));
    delay(100);
  }
  
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
 
    
    server.on("/list", HTTP_GET, handleListFiles);
    
    server.on("/program", HTTP_GET, handleFlash);
    
    server.on("/RunMode", HTTP_GET, []() {
      RunMode();
      server.send(200, "text/plain", "Runstate : OK");
    });
    
    server.on("/EraseFlash", HTTP_GET, []() {
      if (stm32Erase() == STM32ACK)
        stringtmp = "Erase OK";
      else if (stm32Erasen() == STM32ACK)
        stringtmp = "Erase OK";
      else
        stringtmp = "Erase failure";
      server.send(200, "text/plain", stringtmp);
    });
    
    server.on("/delete", HTTP_GET, handleFileDelete);
    server.onFileUpload(handleFileUpload);
    server.on("/upload", HTTP_POST, []() {
      server.send(200, "text/plain", "Uploaded OK");
    });
    server.on("/FlashMode", HTTP_GET, []() {
      
      FlashMode();     
      Serial.write(STM32INIT);
      delay(10);
      if (Serial.available() > 0);
      rdtmp = Serial.read();
      if (rdtmp == STM32ACK)   {
        stringtmp = STM32_CHIPNAME[stm32GetId()];
      }
      else if (rdtmp == STM32NACK) {
        Serial.write(STM32INIT);
        delay(10);
        if (Serial.available() > 0);
        rdtmp = Serial.read();
        if (rdtmp == STM32ACK)   {
          stringtmp = STM32_CHIPNAME[stm32GetId()];
        }
      }
      else
        stringtmp = "ERROR";
        server.send(200, "text/plain",  "Init MCU :" + stringtmp);
    });


    
    server.begin();
  
}

void loop(void) {
  server.handleClient();
}


void FlashMode()  {    //Tested  Change to flashmode
  digitalWrite(BOOT0, HIGH);
  delay(100);
  digitalWrite(NRST, LOW);
  digitalWrite(LED, LOW);
  delay(50);
  digitalWrite(NRST, HIGH);
  delay(200);
  for ( int i = 0; i < 3; i++) {
    digitalWrite(LED, !digitalRead(LED));
    delay(100);
  }
}

void RunMode()  {    //Tested  Change to runmode
  digitalWrite(BOOT0, LOW);
  delay(100);
  digitalWrite(NRST, LOW);
  digitalWrite(LED, LOW);
  delay(50);
  digitalWrite(NRST, HIGH);
  delay(200);
  for ( int i = 0; i < 3; i++) {
    digitalWrite(LED, !digitalRead(LED));
    delay(100);
  }
}
