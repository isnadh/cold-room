#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>

////////////////////////////////////////////////////////////////////////////////

char DEBUG_buff[100];
char payload_buff[100];
uint8_t sensor_id = 1;
String sensor_type = "Sensor_Temp";

////////////////////////////////////////////////////////////////////////////////

const char* ssid     = "";
const char* password = "";
const char* mqtt_server = "broker.netpie.io";

////////////////////////////////////////////////////////////////////////////////

//have to change to uint32_t
uint8_t GetData_timer = 0;

////////////////////////////////////////////////////////////////////////////////

void WIFI_Connect()
{
  WiFi.disconnect();
  delay(1000);
  WiFi.mode(WIFI_STA);
  
  Serial1.print("WiFi connecting ......"); 
  WiFi.begin(ssid, password);
  
  // Wait for connection
  for (int i = 0; i < 25; i++)
  {
    if ( WiFi.status() != WL_CONNECTED ) {
      Serial1.print(".");
      delay(500);
    }
    else{
      Serial1.println("\nWiFi connected");  
      Serial1.println("IP address: ");
      Serial1.println(WiFi.localIP());
      return;
    }
  }
}

bool HTTP_Post(void) {
  
  HTTPClient http; 

  Serial1.println();
  Serial1.println("http://203.151.152.59:1880/api/sensor/"+sensor_type+"/"+String(sensor_id)+"/"); 
  Serial1.println(String(payload_buff)); 

  http.begin("http://203.151.152.59:1880/api/sensor/"+sensor_type+"/"+String(sensor_id)+"/"); 
  http.addHeader("Content-Type", "application/json");
  
  int httpCode = http.POST(String(payload_buff));
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
    //Serial.println(doc["status"].as<bool>());
    Serial1.println(doc["recieve"].as<char*>());
    http.end();
    //return doc["status"].as<bool>();
    return true;
  }
  else
  {
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

    serialFlush();
    memset(payload_buff,0,100);
    
    Serial.write("GetData\r\n");
    //Waits for Data
    delay(2500);
    
    if(Serial.available() > 0){
         int payload_size = Serial.readBytes(payload_buff, 100);
    
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
              memset(payload_buff,0,100);
              return false;
          }
    
      }
    else{
        Serial1.println("UART TIME OUT !!!!!");
        Serial1.println("=======================");
        memset(payload_buff,0,100);
        return false;
      }
}

  
void setup() {
  //Connect to STM
  Serial.begin(115200);
  //DEBUG
  Serial1.begin(115200);
  
  Serial1.println("CRC-16 bit test program");
  Serial1.println("=======================");
  
  
  WiFi.mode(WIFI_STA);  
  Serial1.println("WiFi connecting ......"); 
  if (WiFi.begin(ssid, password)) {
        while (WiFi.status() != WL_CONNECTED) {
            delay(500);
            Serial1.print(".");
        }
  }
  Serial1.println("\nWiFi connected");  
  Serial1.println("IP address: ");
  Serial1.println(WiFi.localIP());
  Serial1.println("Init OK");
 
}


void loop() {

  if (WiFi.status() != WL_CONNECTED){ 
    Serial1.println("\nLoss connection!!!");
    WIFI_Connect();
  }


   if ((millis() > GetData_timer + (5 * 1000)) || (millis() < GetData_timer )) {
      GetData_timer = millis();
      if (WiFi.status() == WL_CONNECTED){ 
          if(GetData()){
                if(HTTP_Post()){
                    Serial1.println("DONE");
                  } 
            }
      }
      
  }
  


}
