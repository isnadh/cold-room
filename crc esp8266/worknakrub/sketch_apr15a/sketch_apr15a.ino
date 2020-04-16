#include <Crc16.h>
Crc16 crc;

char buffer1[100];
int i =0;
uint8_t start = 0;
uint16_t length;
int system_i;
uint16_t value;
uint8_t payload_save[100];

uint8_t *datain_scan;

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.println("CRC-16 bit test program");
  Serial.println("=======================");
  crc.clearCrc();
  pinMode(LED_BUILTIN, OUTPUT);

}

void toggle_led() {
  digitalWrite(LED_BUILTIN, HIGH);
  delay(200);
  digitalWrite(LED_BUILTIN, LOW);
  i =0;
  memset(payload_save,0,100); 
  }

void loop() {
    
    while(Serial.available() > 0){
      payload_save[i]= Serial.read();
      //sprintf(buffer1, "%c", payload_save[i]);
      //Serial.print(payload_save[i]);
      i++; /////shift to next windows of array
      system_i =i;
      //Serial.println(Serial.readBytes(payload_save, 100));
      }
  
  if(system_i == 86){
    
    for (int i = 0; i == 86; i++) {
       crc.updateCrc(payload_save[i]);
      }
  //Serial.println(i);
  length = 86;
  uint16_t value = crc.getCrc();
  value = crc.Modbus(payload_save,start,length);
  sprintf(buffer1, " MODBUS crc = 0x%x", value);
  Serial.println(buffer1);
  i=0;
  system_i =0;
  toggle_led();
  
  memset(payload_save,0,100);
  crc.clearCrc();
  Serial.println("=======================");
  }
  else{
      i=0;
      system_i =0; 
  }
  //delay(1000);
}
