#include <Crc16.h>

Crc16 crc;
uint16_t *parameter;
uint16_t value;
char buffer1[100];
uint8_t start = 0;
uint16_t length;

void setup() {
  Serial.begin(115200);
  Serial.println("CRC-16 bit test program");
  Serial.println("=======================");
  crc.clearCrc();
  pinMode(LED_BUILTIN, OUTPUT);
}

void toggle_led() {
  digitalWrite(LED_BUILTIN, HIGH);
  delay(500);
  digitalWrite(LED_BUILTIN, LOW);
}

bool update_data(){
  
}

void loop() {
  crc.clearCrc();
  uint8_t data[] = "\"Data\": {\"temp_1\": 25.5,\"temp_2\": 24.4,\"temp_3\": \"N/A\",\"temp_4\": \"N/A\"}";

  delay(1000);
  ///cheack array size //////
  Serial.println("--------------------------");
  sprintf(buffer1, "sizeofdata == %d", sizeof(data));
  Serial.println(buffer1);
  Serial.println("--------------------------");

  for (int i = 0; i < (sizeof(data) - 1); i++) {
    Serial.print(data[i]);
    crc.updateCrc(data[i]);
  }
  length = sizeof(data) - 1;
  /////////crc total
  value = crc.getCrc();
  sprintf(buffer1, " \n crc = %x", value);
  Serial.println(buffer1);

  //Modbus
  value = crc.Modbus(data,start,length);
  sprintf(buffer1, " MODBUS crc = %x", value);
  Serial.println(buffer1);
  
  //data clear array ////
  memset(data, 0, sizeof(data));
  for (int i = 0; i < (sizeof(data) - 1); i++) {
    sprintf(buffer1, "array data %d", data[i]);
    Serial.println(buffer1);
  }

  toggle_led();
}
