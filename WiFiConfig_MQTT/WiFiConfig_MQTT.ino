#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#include <FS.h>   

//needed for library
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>        

////////////////////////////////////////////////////////////////////////////////

WiFiClient espClient;
PubSubClient client(espClient);

////////////////////////////////////////////////////////////////////////////////

const char* mqtt_server = "broker.nexpie.io";
const char* Client_ID  = "";
const char* Token   = "";
const char* Secret = "";

////////////////////////////////////////////////////////////////////////////////

#define Reset_WiFi 12 //GPIO 12
#define LED 13 //GPIO 13

////////////////////////////////////////////////////////////////////////////////

#define Uid ""
#define Version "1.0.0"

////////////////////////////////////////////////////////////////////////////////

char MQTT_buffer[300];
char DEBUG_buff[200];
char payload_buff[200];

////////////////////////////////////////////////////////////////////////////////

int timer = 0;
uint32_t GetData_timer = 0;
uint8_t Reconnect_attempt = 0;

////////////////////////////////////////////////////////////////////////////////

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
      Reconnect_attempt = 0;
      Serial1.println("\nWiFi connected");  
      Serial1.println("IP address: ");
      Serial1.println(WiFi.localIP());
      digitalWrite(LED, HIGH);
      return;
    }
  }
}

void nexpie_connect() {
  uint8_t reset_cnt = 0;
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial1.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(Client_ID,Token,Secret)) {
      reset_cnt = 0;
      Serial1.println("Nexpie-Connected");
    } 
    else {
      Serial1.print("failed, rc=");
      Serial1.print(client.state());
      Serial1.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
      reset_cnt++;
         if(reset_cnt >= 36){
         ESP.reset();
        }
    }
      delay(100);
      Serial.write("WiFierr\r\n");
      delay(100);  
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
      
      wifiManager.setConnectTimeout(30);
      wifiManager.setUid(Uid);
      
      wifiManager.autoConnect();
    
      //if you get here you have connected to the WiFi
      Serial1.println("\nWiFi connected");
      
      Serial1.println("IP address: ");
      Serial1.println(WiFi.localIP());
  }

  //Connect to MQTT Broker
  Serial1.println("\nnexpie connecting ......");
  client.setServer(mqtt_server, 1883);
  nexpie_connect();
  Serial1.println("nexpie connected");

  
  Serial1.println("\nInit DONE!");
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

  if (WiFi.status() != WL_CONNECTED){ 
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
        if (client.connected()) {
        
            /* Call this method regularly otherwise the connection may be lost */
            client.loop();
            
            if ((millis() > GetData_timer + (8 * 1000)) || (millis() < GetData_timer )) {
              
              GetData_timer = millis();

              for(uint8_t retry = 0 ; retry <= 5 ; retry++){

                    if(GetData()){  
                         sprintf(MQTT_buffer, "{%s,\"RSSI\": %d}}",payload_buff,WiFi.RSSI());
                         Serial1.println("publish -->  " + String(MQTT_buffer));
                         client.publish("@shadow/data/update",MQTT_buffer);
                         digitalWrite(LED, LOW);
                         delay(500);             
                         digitalWrite(LED, HIGH);
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
        else{
            if (timer >= 8000) {
              Serial1.println("nexpie-reconnect");
              nexpie_connect();
              timer = 0;
            }
            else timer += 100;
        }
        delay(100);    
    }
  delay(100);   
 }
          

  
