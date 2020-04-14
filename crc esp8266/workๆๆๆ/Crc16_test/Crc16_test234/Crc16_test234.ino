#include <Crc16.h>

Crc16 crc; 
uint16_t value;
char buffer1[100];

void setup(){
    Serial.begin(115200); 
    Serial.println("CRC-16 bit test program");
    Serial.println("=======================");
    crc.clearCrc();
    pinMode(LED_BUILTIN, OUTPUT);
  }
void toggle_led(){
  digitalWrite(LED_BUILTIN,HIGH);
  delay(500);
  digitalWrite(LED_BUILTIN,LOW);
}

void loop(){
  
  uint8_t data[] = "ABCDEfghI";

  for(int i=0;i<9;i++){
     Serial.print("byte ");
     Serial.print(i);
     Serial.print(" = ");
     Serial.println(data[i]);
     crc.updateCrc(data[i]);
  }
  value = crc.getCrc();
  Serial.print("crc = 0x");
  Serial.println(value, HEX);
  
  Serial.println("Calculating crc in a single call");
  
  //Modbus
  value = crc.Modbus(data,0,9);
  Serial.print("Modbus crc = 0x");    
  Serial.println(value, HEX);
  
  //clear array in ////
  Serial.println("---------------------");
  Serial.println(sizeof(data)-1);
  Serial.println("---------------------");
  memset(data, 0, sizeof(data));
  
  for(int i=0;i<9;i++){
     sprintf(buffer1, "array data %d",data[i]);
     Serial.println(buffer1);
  }
  toggle_led(); 
  delay(2000); 
}
