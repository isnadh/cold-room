#include "stm32f10x.h"
#include "stm32f10x_conf.h"

#include "CircularBuffer.h"
#include "ATparser.h"
#include "one_wire.h"
#include "ds18b20.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

void USART2_IRQHandler(void);

void init_usart1(void);
void init_usart2(void);
void init_iwdg(void);
void NVIC_Configuration(void);

void send_byte(uint8_t b);
void usart_puts(char* s);
void send_byte2(uint8_t b);
void usart_puts2(char *s);

void Send_ESP_Payload(void);
uint16_t crc16_update(uint16_t crc, uint8_t a);

void delay(unsigned long ms);

int readFunc(uint8_t *data);
int writeFunc(uint8_t *buffer, size_t size);
bool readableFunc(void);
void sleepFunc(int us);

uint8_t Scan_Sensors(void);
void Read_Sensors(void);

//ESP Reset counter
int count = 0;
int reset_cnt = 0;

// Buffer
char Debug_BUF[100];
uint8_t Command_BUF[10] = {'\0'};
char ESP_Payload[100];


// For Usart command 
bool data_ready = false;
uint8_t command_digit = 0;
uint8_t State = 1;


// Status of connecting sensors 
uint8_t Number_of_sensor = 0;
bool temp1_connected = false;
bool temp2_connected = false;
bool temp3_connected = false;
bool temp4_connected = false;


// Char buffer for storing sensors data
char temp_1[10];
char temp_2[10];
char temp_3[10];
char temp_4[10];

simple_float temp_data ;
one_wire_device data;

circular_buf_t cbuf;
atparser_t parser;


/*USART Data
"Data": {
      "temp_1": 25.5,
      "temp_2": 24.4,
      "temp_3": "N/A",
      "temp_4": "N/A"
    }
*/

void USART2_IRQHandler(void)
{
    char b;

    if(USART_GetFlagStatus(USART2, USART_FLAG_RXNE) == SET) {

        b =  USART_ReceiveData(USART2);
        
        command_digit++;

        //Check maximum size of the command **you can arrange the maximum size of your command here** 
        if(command_digit > 9){
          atparser_flush(&parser);
          usart_puts("\nLong-Command -> Clear_RXBuffer\n\n");
          command_digit = 0 ;
          State = 1;
        }
      

        circular_buf_put(&cbuf, b);

        switch (State)
      {
          case 1:
              
              if (b == '\r'){
                State++;  
              }
              break;
          
          case 2:
              
              if (b == '\n'){
             
                // Check size of the command **you can arrange the target size of your command here**
                // EX.   if(command_size >= 4) **It mean command can be 4-9 digits(set maximun = 9)** 
                uint8_t command_size = circular_buf_size(&cbuf);
                if(command_size == 9){
                  
                  atparser_read(&parser, Command_BUF, 9);
                  sprintf(Debug_BUF, "\nCommand size : %d    Command:  %s", command_size,Command_BUF);
                  usart_puts(Debug_BUF);

                  atparser_flush(&parser);
                  usart_puts("Clear_RXBuffer\n\n");
                  command_digit = 0 ;
                  data_ready = true;
                  
                }
                else{
                  atparser_flush(&parser);
                  usart_puts("\nShort-Command -> Clear_RXBuffer\n\n");
                  command_digit = 0 ;
                  State = 1;
                }

                State = 1;
              }

              else{
                atparser_flush(&parser);
                usart_puts("\nClear_RXBuffer\n\n");
                command_digit = 0 ;
                State = 1;
              }

              break;
          
          default:
              atparser_flush(&parser);
              usart_puts("\nClear_RXBuffer\n\n");
              command_digit = 0 ;
              State = 1;
              break;
      }
	}
}

static inline void Delay_1us(uint32_t nCnt_1us)
{
  volatile uint32_t nCnt;

  for (; nCnt_1us != 0; nCnt_1us--)
    for (nCnt = 4; nCnt != 0; nCnt--)
      ;
}

void delay(unsigned long ms)
{
  volatile unsigned long i,j;
  for (i = 0; i < ms; i++ )
  for (j = 0; j < 1000; j++ );
}

void send_byte(uint8_t b)
{
	/* Send one byte */
	USART_SendData(USART1, b);

	/* Loop until USART2 DR register is empty */
	while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
}


void usart_puts(char* s)
{
    while(*s) {
    	send_byte(*s);
        s++;
    }
}


void send_byte2(uint8_t b)
{
  /* Send one byte */
  USART_SendData(USART2, b);

  /* Loop until USART2 DR register is empty */
  while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET)
    ;
}

void usart_puts2(char *s)
{
  while (*s)
  {
    send_byte2(*s);
    s++;
  }
}


//USART1 TX Debug
void init_usart1(void)
{

	USART_InitTypeDef USART_InitStructure;
	GPIO_InitTypeDef GPIO_InitStructure;

	/* Enable peripheral clocks. */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);

	/* Configure USART1 Rx pin as floating input. */
	// GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	// GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	// GPIO_Init(GPIOA, &GPIO_InitStructure);

	/* Configure USART1 Tx as alternate function push-pull. */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	/* Configure the USART1 */
	USART_InitStructure.USART_BaudRate = 115200;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Tx;
	USART_Init(USART1, &USART_InitStructure);

	USART_Cmd(USART1, ENABLE);

}


// USART2 using for Communicate with esp8266
void init_usart2(void)
{

	USART_InitTypeDef USART_InitStructure;
	GPIO_InitTypeDef GPIO_InitStructure;

	/* Enable peripheral clocks. */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);

	/* Configure USART1 Rx pin as floating input. */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	/* Configure USART1 Tx as alternate function push-pull. */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	/* Configure the USART1 */
	USART_InitStructure.USART_BaudRate = 115200;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(USART2, &USART_InitStructure);

	NVIC_InitTypeDef NVIC_InitStructure;

	NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	/* Enable transmit and receive interrupts for the USART1. */
	USART_ITConfig(USART2, USART_IT_TXE, DISABLE);
	USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);

	USART_Cmd(USART2, ENABLE);
}

void init_iwdg(void)
{
  /* Enable the LSI OSC */
  RCC_LSICmd(ENABLE);
  /* Wait till LSI is ready */
  while (RCC_GetFlagStatus(RCC_FLAG_LSIRDY) == RESET)
  {}

  /* Enable Watchdog*/
  IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);
  IWDG_SetPrescaler(IWDG_Prescaler_256); // 4, 8, 16 ... 256
  IWDG_SetReload(0x0FFF);//This parameter must be a number between 0 and 0x0FFF.
  IWDG_ReloadCounter();
  IWDG_Enable();

}


void NVIC_Configuration(void) {

    NVIC_SetPriorityGrouping(NVIC_PriorityGroup_4);

    NVIC_SetPriority(USART2_IRQn, NVIC_EncodePriority(4,1,0));

}

// ATparser callback function
int readFunc(uint8_t *data)
{
    return circular_buf_get(&cbuf, data);    
}

int writeFunc(uint8_t *buffer, size_t size)
{

    size_t i = 0;
    for (i = 0; i < size; i++)
    {
      send_byte2(buffer[i]);
    }
    
  return 0;
}

bool readableFunc()
{
    return !circular_buf_empty(&cbuf);
}

void sleepFunc(int us)
{
  Delay_1us(us);
}


// Call when start-up (1-Time)
uint8_t Scan_Sensors(void)
{
  uint8_t retry = 0;
  uint8_t sum = 0;

  //Temp 1
  usart_puts("\n"); 
  ds18b20_init(GPIOA, GPIO_Pin_0, TIM2);
  for(retry = 0 ; retry <= 5 ; retry++){

      usart_puts("Scan Sensor 01 ");      
      if(one_wire_reset_pulse()){
         usart_puts("-> Sensor01 connected!!!\n"); 

         data = one_wire_read_rom();
         sprintf(Debug_BUF, "Address %d:%d:%d:%d:%d:%d:%d:%d", data.address[7],data.address[6],data.address[5],data.address[4],data.address[3],data.address[2],data.address[1],data.address[0]);
         usart_puts(Debug_BUF);
         usart_puts("\n");
         temp1_connected = true;
         sum++;     
         break;
      }
      else{
         usart_puts("-> Sensor01 not connected...\n");
         strcpy(temp_1,"\"N/A\"");    
      }
  }

  //Temp 2
  usart_puts("\n"); 
  ds18b20_init(GPIOA, GPIO_Pin_1, TIM2);
  for(retry = 0 ; retry <= 5 ; retry++){

      usart_puts("Scan Sensor 02 ");
      if(one_wire_reset_pulse()){
         usart_puts("-> Sensor02 connected!!!\n"); 

         data = one_wire_read_rom();
         sprintf(Debug_BUF, "%d:%d:%d:%d:%d:%d:%d:%d", data.address[7],data.address[6],data.address[5],data.address[4],data.address[3],data.address[2],data.address[1],data.address[0]);
         usart_puts(Debug_BUF);
         usart_puts("\n");
         temp2_connected = true;
         sum++;    
         break;
      }
      else{
         usart_puts("-> Sensor02 not connected...\n");
         strcpy(temp_2,"\"N/A\"");     
      }
  }

  //Temp 3
  usart_puts("\n"); 
  ds18b20_init(GPIOB, GPIO_Pin_0, TIM2);
  for(retry = 0 ; retry <= 5 ; retry++){

      usart_puts("Scan Sensor 03 ");      
      if(one_wire_reset_pulse()){
         usart_puts("-> Sensor03 connected!!!\n"); 

         data = one_wire_read_rom();
         sprintf(Debug_BUF, "Address %d:%d:%d:%d:%d:%d:%d:%d", data.address[7],data.address[6],data.address[5],data.address[4],data.address[3],data.address[2],data.address[1],data.address[0]);
         usart_puts(Debug_BUF);
         usart_puts("\n");
         temp3_connected = true;
         sum++; 
         break;
      }
      else{
         usart_puts("-> Sensor03 not connected...\n");
         strcpy(temp_3,"\"N/A\"");    
      }
  }

  //Temp 4
  usart_puts("\n"); 
  ds18b20_init(GPIOB, GPIO_Pin_1, TIM2);
  for(retry = 0 ; retry <= 5 ; retry++){

      usart_puts("Scan Sensor 04 ");
      if(one_wire_reset_pulse()){
         usart_puts("-> Sensor04 connected!!!\n"); 

         data = one_wire_read_rom();
         sprintf(Debug_BUF, "%d:%d:%d:%d:%d:%d:%d:%d", data.address[7],data.address[6],data.address[5],data.address[4],data.address[3],data.address[2],data.address[1],data.address[0]);
         usart_puts(Debug_BUF);
         usart_puts("\n");
         temp4_connected = true;
         sum++;    
         break;
      }
      else{
         usart_puts("-> Sensor04 not connected...\n");
         strcpy(temp_4,"\"N/A\"");     
      }
  }
  return sum;

}

void Read_Sensors(void){
 float temp; 

  //Read Sensor1
  if (temp1_connected){
      ds18b20_init(GPIOA, GPIO_Pin_0, TIM2);
      ds18b20_convert_temperature_simple();
      Delay_1us(200000);
      temp_data = ds18b20_read_temperature_simple();
        
        if(temp_data.is_valid){
          temp = temp_data.raw_temp;   
          //Convert float to String//
          gcvt(temp,5,temp_1);  
        }
        else{
          strcpy(temp_1,"\"N/A\""); 
        }

      Delay_1us(10000);
  }

  //Read Sensor2
  if (temp2_connected){
      ds18b20_init(GPIOA, GPIO_Pin_1, TIM2);
      ds18b20_convert_temperature_simple();
      Delay_1us(200000);
      temp_data = ds18b20_read_temperature_simple();
        
        if(temp_data.is_valid){
          temp = temp_data.raw_temp;
          //Convert float to String//
          gcvt(temp,5,temp_2); 
        }
        else{
          strcpy(temp_2,"\"N/A\""); 
        }

      Delay_1us(10000);
  }

   //Read Sensor3
   if (temp3_connected){
      ds18b20_init(GPIOB, GPIO_Pin_0, TIM2);
      ds18b20_convert_temperature_simple();
      Delay_1us(200000);
      temp_data = ds18b20_read_temperature_simple();
        
        if(temp_data.is_valid){
          temp = temp_data.raw_temp;
          //Convert float to String//
          gcvt(temp,5,temp_3); 
        }
        else{
          strcpy(temp_3,"\"N/A\""); 
        }

      Delay_1us(10000);
  }

   //Read Sensor4
   if (temp4_connected){
      ds18b20_init(GPIOB, GPIO_Pin_1, TIM2);
      ds18b20_convert_temperature_simple();
      Delay_1us(200000);
      temp_data = ds18b20_read_temperature_simple();
        
        if(temp_data.is_valid){
          temp = temp_data.raw_temp;
          //Convert float to String//
          gcvt(temp,5,temp_4);
        }
        else{
          strcpy(temp_4,"\"N/A\""); 
        }
 
      Delay_1us(10000);
  }

}

void Send_ESP_Payload(void)
{
  char Payload [100];

  sprintf(Payload, "{\"Data\": {\"temp_1\": %s,\"temp_2\": %s,\"temp_3\": %s,\"temp_4\": %s}}", temp_1,temp_2,temp_3,temp_4 );
  usart_puts(Payload);
  sprintf(Debug_BUF, "\nPayload size :  %d \n", strlen(Payload) );
  usart_puts(Debug_BUF);


  uint16_t crc = 0xFFFF;
  uint8_t i;
    for(i = 0; i < strlen(Payload); i++){
        crc = crc16_update(crc, (uint8_t)Payload[i]);
      }

  sprintf(Debug_BUF, "\nPayload CRC :  %X \n",crc );
  usart_puts(Debug_BUF);

  // convert CRC16 to 2byte 
  uint8_t  crc8[2];
  crc8[0]= (uint8_t)crc;
  crc8[1]=(crc >> 8);

  sprintf(Debug_BUF, "\nESP_Payload CRC :  %X , %X\n",crc8[1],crc8[0] );
  usart_puts(Debug_BUF);

  //Send data to ESP
  usart_puts2(Payload);
  send_byte2(crc8[1]);
  send_byte2(crc8[0]);

}

/*Ex. code To use this crc16
    uint16_t crc = 0xFFFF;
    int i;
    for(i = 0; i < 5; i++){
        crc = crc16_update(crc, data_buf[i]);
      }
*/
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

int main(void)
{
  /* Initialize Leds mounted on STM32 board */
  GPIO_InitTypeDef  GPIO_InitStructure;
  /* Initialize ESP-Reset which is connected to PA8, Enable the Clock*/
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
  /* Configure the GPIO_LED pin */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOA, &GPIO_InitStructure);

  /* Disable ESP-reset */
  GPIO_SetBits(GPIOA, GPIO_Pin_8);

  init_iwdg();
  init_usart1();
  init_usart2();
  NVIC_Configuration();


  circular_buf_init(&cbuf);
  atparser_init(&parser, readFunc, writeFunc, readableFunc, sleepFunc);


  usart_puts("\r\nSTART\r\n");
  
  Number_of_sensor = Scan_Sensors();
  sprintf(Debug_BUF, "\n--- %d Sensors connected ---\n", Number_of_sensor );
  usart_puts(Debug_BUF);
  Read_Sensors();
  sprintf(Debug_BUF, "\"Data\": {\"temp_1\": %s,\"temp_2\": %s,\"temp_3\": %s,\"temp_4\": %s}", temp_1,temp_2,temp_3,temp_4 );
  usart_puts(Debug_BUF);

  /* Reset ESP*/
  usart_puts("\r\nReset ESP\r\n");
  GPIO_ResetBits(GPIOA, GPIO_Pin_8);
  Delay_1us(2000000);
  GPIO_SetBits(GPIOA, GPIO_Pin_8);

   /* Wait ESP ready*/
  IWDG_ReloadCounter();
  Delay_1us(6000000);

  memset(Command_BUF, 0, sizeof(Command_BUF));
  atparser_flush(&parser);
  usart_puts("\r\nStart\r\n");

  while (1)
  { 

   IWDG_ReloadCounter(); 
   sprintf(Debug_BUF, "count is : %d", count);
   usart_puts(Debug_BUF);
   usart_puts("\n");
   sprintf(Debug_BUF, "reset cnt is : %d", reset_cnt);
   usart_puts(Debug_BUF);
   usart_puts("\n");


   if (data_ready) {

      usart_puts("\n");

      if (!strcmp((char*)Command_BUF, "GetData\r\n")) {
        usart_puts("GET_DATA: OK\n");

        memset(Command_BUF, 0, sizeof(Command_BUF));
        atparser_flush(&parser);
        ///////////////////////////////////////////////////////
        Read_Sensors();
        Send_ESP_Payload();
        //////////////////////////////////////////////////////
        data_ready = false;
        count = 0;
      }

      else if (!strcmp((char*)Command_BUF, "WiFierr\r\n")) {
        usart_puts("WiFi_Error: OK\n");

        memset(Command_BUF, 0, sizeof(Command_BUF));
        atparser_flush(&parser);
        data_ready = false;
        count = 0;
      }

      else if (!strcmp((char*)Command_BUF, "RegisGW\r\n")) {
        usart_puts("Regis_GW: OK\n");

        memset(Command_BUF, 0, sizeof(Command_BUF));
        atparser_flush(&parser);
        data_ready = false;
        count = 0;
      }

      else{
        usart_puts("Unknown Command\n");        
        memset(Command_BUF, 0, sizeof(Command_BUF));
        atparser_flush(&parser);
        data_ready = false;
        count++;
      }

      usart_puts("\n");

   }

   else {
     count++;
   } 



   if(count >= 300){

   IWDG_ReloadCounter();

   usart_puts("\r\nReset ESP\r\n");
   GPIO_ResetBits(GPIOA, GPIO_Pin_8);
   Delay_1us(2000000);
   GPIO_SetBits(GPIOA, GPIO_Pin_8);
   Delay_1us(6000000);
   usart_puts("\r\nStart\r\n");

   memset(Command_BUF, 0, sizeof(Command_BUF));
   atparser_flush(&parser);

   reset_cnt++;
   count = 0;

   }

   Delay_1us(500000);

  }
}
