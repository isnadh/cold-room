#include <Crc16.h>
Crc16 crc;

char buffer1[100];
int i =0;
uint8_t start = 0;
uint16_t length;
int system_i;
uint16_t value;
uint8_t payload_save[100];

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
}

void loop() {
  while(Serial.available() > 0){
    payload_save[i] = Serial.read();
    sprintf(buffer1, "%c", payload_save[i]);
    Serial.print(buffer1);
    i++;
    system_i = i;
  }
  
  if(system_i == 86){
    for (int i = 0; i < system_i - 1; i++) {
      crc.updateCrc(payload_save[i]);
      }
  Serial.println(i);
  length = system_i - 1;
  uint16_t value = crc.getCrc();
  value = crc.Modbus(payload_save,start,length);
  sprintf(buffer1, " MODBUS crc = 0x%x", value);
  Serial.println(buffer1);
  i=0;
  system_i =0;
  toggle_led();
  
  memset(payload_save,0,100);
  ///crc.clearCrc();
  Serial.println("=======================");
  } 
  //delay(1000);
}
