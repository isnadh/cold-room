
char DEBUG_buff[100];
char payload_buff[100];


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

void setup() {
  Serial.begin(115200);
  Serial.println("CRC-16 bit test program");
  Serial.println("=======================");     
}


void loop() {
  
    //Next I will try Serial.available() > 70
    if(Serial.available() > 0){
    delay(1000);
    int payload_size = Serial.readBytes(payload_buff, 100);

     Serial.println("=======================");
     sprintf(DEBUG_buff, " %X , %X ", payload_buff[payload_size-1],payload_buff[payload_size-2]);
     Serial.println(DEBUG_buff);
     Serial.println("=======================");
     Serial.println(payload_buff);
     Serial.println("=======================");
     Serial.println(payload_size);

     uint16_t crc = 0xFFFF;
     int i;
     for(i = 0; i < payload_size-2; i++){
        crc = crc16_update(crc, (uint8_t)payload_buff[i]);
      }
      
     Serial.println("=======================");
     sprintf(DEBUG_buff, "CRC %X", crc);
     Serial.println(DEBUG_buff);


     if((uint8_t)crc == (uint8_t)payload_buff[payload_size-1] && (uint8_t)(crc >> 8) == (uint8_t)payload_buff[payload_size-2]){      
          Serial.println("CRC Check OK !!!!!");
      }
     else{
          Serial.println("CRC Check Error !!!!!");
      }


     memset(payload_buff,0,100);
  }

}
