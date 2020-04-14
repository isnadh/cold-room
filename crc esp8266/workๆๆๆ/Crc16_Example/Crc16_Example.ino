#include <Crc16.h>

Crc16 crc; 
unsigned short value;
void setup()
{
    Serial.begin(115200); 
    Serial.println("CRC-16 bit test program");
    Serial.println("=======================");
    crc.clearCrc();
}

void loop(){
  
  byte data[] = "ABCDEfghI";
  
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
  memset(data, 0, sizeof(data));
  

  
}
