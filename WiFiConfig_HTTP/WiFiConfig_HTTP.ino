#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>

#include <FS.h>   

//needed for library
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>        


////////////////////////////////////////////////////////////////////////////////

#define Reset_WiFi 12 //GPIO 12
#define LED 13 //GPIO 13

////////////////////////////////////////////////////////////////////////////////

#define Uid "SENSOR3P-V2"
#define Version "1.0.0"

////////////////////////////////////////////////////////////////////////////////

char DEBUG_buff[100];
char payload_buff[200];

////////////////////////////////////////////////////////////////////////////////

// Testing IP = 203.151.152.59:1880
char Gateway_ip[32];

////////////////////////////////////////////////////////////////////////////////

uint32_t GetData_timer = 0;
bool Need_register = true;

uint8_t Reconnect_attempt = 0;
bool Need_reconnect = false;

////////////////////////////////////////////////////////////////////////////////

//flag for saving data
bool shouldSaveConfig = false;

////////////////////////////////////////////////////////////////////////////////

//callback notifying us of the need to save config
void saveConfigCallback () {
  Serial1.println("Should save config");
  shouldSaveConfig = true;
}


void WIFI_Connect()
{
  
  WiFi.mode(WIFI_STA); 
  WiFi.reconnect();
  delay(1000);
  
  Serial1.print("WiFi connecting ......"); 
  WiFi.begin();
  
  // Wait for connection
  for (int i = 0; i < 25; i++)
  {
    if ( WiFi.status() != WL_CONNECTED ) {
      Serial1.print(".");
      delay(500);
    }
    else{
      Need_reconnect = false;
      Reconnect_attempt = 0;
      Serial1.println("\nWiFi connected");  
      Serial1.println("IP address: ");
      Serial1.println(WiFi.localIP());
      digitalWrite(LED, HIGH);
      return;
    }
  }
}

bool HTTP_Post(void) {
  
  HTTPClient http; 

  Serial1.println();
  Serial1.println("http://"+String(Gateway_ip)+"/api/uid/"+String(Uid)+"/"); 
  Serial1.println("{"+String(payload_buff)+",\"properties\":{\"RSSI\" :" +WiFi.RSSI()+ "}}"); 

  http.begin("http://"+String(Gateway_ip)+"/api/uid/"+String(Uid)+"/"); 
  http.addHeader("Content-Type", "application/json");
  
  int httpCode = http.POST("{"+String(payload_buff)+",\"properties\":{\"RSSI\" :" +WiFi.RSSI()+ "}}");
  String payload = http.getString();                                     
  
  Serial1.print(F("HTTP POST Result: "));
  Serial1.println(httpCode);   //Print HTTP return code
  Serial1.println(payload);

  if(httpCode == 200)
  {
    // Allocate JsonBuffer
    // Use arduinojson.org/assistant to compute the capacity.
    const size_t capacity = JSON_OBJECT_SIZE(3) + JSON_ARRAY_SIZE(2) + 60;
    DynamicJsonDocument doc(capacity);
  
    // Parse JSON object
    DeserializationError error = deserializeJson(doc, payload);
    if (error) {
      Serial1.print(F("deserializeJson() failed"));
      http.end();
      return false;
    }  
    // Decode JSON/Extract values
    Serial1.println(F("Response:"));
    Serial1.print(F("require: "));
    Serial1.println(doc["require"].as<bool>());
    Serial1.print(F("recieve: "));
    Serial1.println(doc["recieve"].as<char*>());
    http.end();

    if(doc["require"].as<bool>()){
          Need_register = true;
      }
      
    return true;
  
  }
  else
  {
    
    if(httpCode == -1){
        Need_reconnect = true;
    }
    
    Serial1.println("Error in response");
    http.end();
    return false;
  }
}

bool HTTP_Put(void) {
  
  HTTPClient http; 

  Serial1.println();
  Serial1.println("http://"+String(Gateway_ip)+"/api/uid/"+String(Uid)+"/"); 
  Serial1.println("{\"uid\": \""+String(Uid)+"\",\"version\":\""+String(Version)+"\"}"); 

  http.begin("http://"+String(Gateway_ip)+"/api/uid/"+String(Uid)+"/"); 
  http.addHeader("Content-Type", "application/json");
  
  int httpCode = http.PUT("{\"uid\": \""+String(Uid)+"\",\"version\":\""+String(Version)+"\"}");
  String payload = http.getString();                                     
  
  Serial1.print(F("HTTP PUT Result: "));
  Serial1.println(httpCode);   //Print HTTP return code
  Serial1.println(payload);

  if(httpCode == 201)
  {
    http.end();
    return true;
  }
  else
  {
    
    if(httpCode == -1){
        Need_reconnect = true;
    }
    
    Serial1.println("Error in response");
    http.end();
    return false;
  }
}


uint16_t crc16_update(uint16_t crc, uint8_t a) {
  int i;

  crc ^= (uint16_t)a;
  for (i = 0; i < 8; ++i) {
    if (crc & 1)
      crc = (crc >> 1) ^ 0xA001;
    else
      crc = (crc >> 1);
  }

  return crc;
}

void serialFlush(){
  while(Serial.available() > 0) {
    char t = Serial.read();
  }
}   

bool GetData(void){

    Serial1.println();

    serialFlush();
    memset(payload_buff,0,200);
    
    Serial.write("GetData\r\n");
    //Waits for Data
    delay(5000);
    
    if(Serial.available() > 0){
         int payload_size = Serial.readBytes(payload_buff, 200);
    
         Serial1.println("=======================");
         sprintf(DEBUG_buff, " %X , %X ", payload_buff[payload_size-1],payload_buff[payload_size-2]);
         Serial1.println(DEBUG_buff);
         Serial1.println("=======================");
         Serial1.println(payload_buff);
         Serial1.println("=======================");
         Serial1.println(payload_size);
    
         uint16_t crc = 0xFFFF;
         int i;
         for(i = 0; i < payload_size-2; i++){
            crc = crc16_update(crc, (uint8_t)payload_buff[i]);
          }
          
         Serial1.println("=======================");
         sprintf(DEBUG_buff, "CRC %X", crc);
         Serial1.println(DEBUG_buff);
    
    
         if((uint8_t)crc == (uint8_t)payload_buff[payload_size-1] && (uint8_t)(crc >> 8) == (uint8_t)payload_buff[payload_size-2]){      
              Serial1.println("CRC Check OK !!!!!");
              Serial1.println("=======================");
              payload_buff[payload_size-1] = '\0';
              payload_buff[payload_size-2] = '\0'; 
              return true;
          }
         else{
              Serial1.println("CRC Check Error !!!!!");
              Serial1.println("=======================");
              memset(payload_buff,0,200);
              return false;
          }
    
      }
    else{
        Serial1.println("UART TIME OUT !!!!!");
        Serial1.println("=======================");
        memset(payload_buff,0,200);
        return false;
      }
}


void read_config(void){
    
    Serial1.println("mounting FS...");
  
    if (SPIFFS.begin()) {
      Serial1.println("mounted file system");
      if (SPIFFS.exists("/config.json")) {
        //file exists, reading and loading
        Serial1.println("reading config file");
        File configFile = SPIFFS.open("/config.json", "r");
        if (configFile) {
          Serial1.println("opened config file");
          size_t size = configFile.size();
          
          // Allocate a buffer to store contents of the file.
          std::unique_ptr<char[]> buf(new char[size]);
          configFile.readBytes(buf.get(), size);
  
          const size_t capacity = JSON_OBJECT_SIZE(3) + JSON_ARRAY_SIZE(2) + 60;
          DynamicJsonDocument jsonBuffer(capacity);
                   
          DeserializationError error =deserializeJson(jsonBuffer, buf.get());
          serializeJson(jsonBuffer, Serial1);
  
          if (!error) {         
            Serial1.println("\nparsed json");           
            strcpy(Gateway_ip, jsonBuffer["Gateway_ip"]);  
            Serial1.println(Gateway_ip); 
  
          } 
          else {
            Serial1.println("failed to load json config");
            delay(1000);
            ESP.reset();
          }
          configFile.close();
        }
      }
    } 
    
    else {
      Serial1.println("failed to mount FS");
      delay(1000);
      ESP.reset();
    }
  
}
void SaveConfig(void){
  
      Serial1.println("saving config\n");
  
      const size_t capacity = JSON_OBJECT_SIZE(3) + JSON_ARRAY_SIZE(2) + 60;
      DynamicJsonDocument jsonBuffer(capacity);
          
      jsonBuffer["Gateway_ip"] = Gateway_ip;
  
      File configFile = SPIFFS.open("/config.json", "w");
      if (!configFile) {
        Serial1.println("failed to open config file for writing");
        WiFi.disconnect(true);
        delay(500);
        ESP.reset();
      }
  
      serializeJson(jsonBuffer, Serial1);
      serializeJson(jsonBuffer, configFile);
      configFile.close();
      Serial1.println("DONE!!!");
}
  
void setup() {
  
  //Connect to STM
  Serial.begin(115200);
  //DEBUG
  Serial1.begin(115200);
  //Reset WiFi Config Button
  pinMode(Reset_WiFi, INPUT_PULLUP);  
  //WiFi LED
  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW);  

  //read configuration from FS json
  read_config();

  if (WiFi.SSID() != "") {

      Serial1.println("Using last saved values");   
      WiFi.mode(WIFI_STA);      
      //trying to fix connection in progress hanging
      ETS_UART_INTR_DISABLE();
      wifi_station_disconnect();
      ETS_UART_INTR_ENABLE();
      WiFi.begin();
      
      while( WiFi.status() != WL_CONNECTED ) {
      Serial1.print(".");
      delay(500);
  
      if(digitalRead(Reset_WiFi) == LOW){
            uint8_t i = 10;
            Serial1.println("\n");
            while(digitalRead(Reset_WiFi) == LOW){
              Serial1.print("Reset_WiFiConfig in ");
              Serial1.println(i);
              delay(500);
              i--;
              
              if(i == 0){
              Serial1.print("Reset_WiFiConfig!!"); 
              WiFi.disconnect(true);
              delay(500);

              for (int i = 0; i < 10; i++){              
              digitalWrite(LED, HIGH);
              delay(200);
              digitalWrite(LED, LOW);
              delay(200);
              }
              
              delay(500);
              ESP.reset(); 
              }               
            }             
         } 
      }          
   }

  else{
      Serial1.println("Start Config Portal"); 
      WiFiManager wifiManager;
    
      wifiManager.setSaveConfigCallback(saveConfigCallback);
      WiFiManagerParameter GW_IP("GW_IP", "Gateway ip", Gateway_ip , 32);
      wifiManager.addParameter(&GW_IP);
      
      wifiManager.setConnectTimeout(30);
      wifiManager.setUid(Uid);
      
      wifiManager.autoConnect();
    
      //if you get here you have connected to the WiFi
      Serial1.println("WiFi connected");
         
      strcpy(Gateway_ip , GW_IP.getValue());
      
      Serial1.println("\nWiFi connected");  
      Serial1.println("IP address: ");
      Serial1.println(WiFi.localIP());
      Serial1.println("Gateway_IP : ");
      Serial1.println(Gateway_ip);
  }
  //save the custom parameters to FS
  if (shouldSaveConfig) {
      SaveConfig();
  }
  
  Serial1.println("\nInit OK");
  digitalWrite(LED, HIGH);
}


void loop() {

   if(digitalRead(Reset_WiFi) == LOW){
      uint8_t i = 10;
      while(digitalRead(Reset_WiFi) == LOW){
        Serial1.print("Reset_WiFiConfig in ");
        Serial1.println(i);
        delay(500);
        i--;
        
        if(i == 0){
        Serial1.print("Reset_WiFiConfig!!"); 
        WiFi.disconnect(true);
        delay(500);

        for (int i = 0; i < 10; i++){              
        digitalWrite(LED, HIGH);
        delay(200);
        digitalWrite(LED, LOW);
        delay(200);
        }
        
        delay(500);
        ESP.reset(); 
        }      
      }     
   }

  if (WiFi.status() != WL_CONNECTED || Need_reconnect){ 
    digitalWrite(LED, LOW);
    Reconnect_attempt++;
    Serial1.println("\nLoss connection!!!");
    Serial.write("WiFierr\r\n");
    WIFI_Connect();

    if(Reconnect_attempt == 10){
        Serial1.println("\nLoss connection -> Reset ESP!!!");
        delay(1000);     
        ESP.reset();
      }
  }

  else{

        if(Need_register){
            Serial.write("RegisGW\r\n");
            if( HTTP_Put()){
                Serial1.println("HTTP PUT Success");
                Need_register = false;
              }
            else{
                Serial1.println("HTTP PUT Error");
              }
        }
      
      
        if ((millis() > GetData_timer + (8 * 1000)) || (millis() < GetData_timer )) {
            GetData_timer = millis();
            
                for(uint8_t retry = 0 ; retry <= 5 ; retry++){

                      if (Need_reconnect){                    
                          Serial1.println("\nHTTP POST Error -> Loss connection!!!");
                          Serial.write("WiFierr\r\n");
                          break;
                       }

                      if(GetData()){
                          Serial1.println("GetData Success");                                     
                          if(HTTP_Post()){
                              Serial1.println("HTTP POST Success");                             
                              digitalWrite(LED, LOW);
                              delay(500);             
                              digitalWrite(LED, HIGH);          
                            }
                          else{
                              Serial1.println("HTTP POST Error");
                            }      
                          break;                         
                        }
                      else{
                          Serial1.println("GetData  Error ....");
                          Serial1.print("retry: ");
                          Serial1.println(retry);
                          delay(1000);
                        }
                }
                 
            }
        }
          
  }
  
