#include <Crc16.h>

Crc16 crc; 
uint16_t *parameter;
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
  crc.clearCrc();
  uint8_t data[] = " \"Data\": {\"temp_1\": 25.5, \"temp_2\": 24.4, \"temp_3\": \"N/A\",\"temp_4\": \"N/A\" } ";
  char data1[] = " \"Data\": {\"temp_1\": 25.5, \"temp_2\": 24.4, \"temp_3\": \"N/A\",\"temp_4\": \"N/A\" } ";
  
  delay(1000);
  ///cheack array size //////
  Serial.println("--------------------------");
  uint8_t sizeofdata = sizeof(data);
  sprintf(buffer1,"sizeofdata == %d",sizeofdata);
  Serial.println(buffer1);
  Serial.println("--------------------------");

  for(int i=0;i < (sizeof(data) -1);i++){
      Serial.print(data1[i]);
      crc.updateCrc(data[i]);
  }
  /////////crc total
  value = crc.getCrc();
  sprintf(buffer1," \n crc = 0x%x",value);
  Serial.println(buffer1);
  
  //Modbus
  value = crc.Modbus(data,0,sizeof(data));
  sprintf(buffer1," MODBUS crc = 0x%x",value);
  Serial.println(buffer1);
 
  memset(data, 0, sizeof(data));
  
  //data clear array ////
  for(int i=0;i < (sizeof(data) -1);i++){
     sprintf(buffer1, "array data %d",data[i]);
     Serial.println(buffer1);
  }
  
  toggle_led(); 
  //delay(1000); 
}
