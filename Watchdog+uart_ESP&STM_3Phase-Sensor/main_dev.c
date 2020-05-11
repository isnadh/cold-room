#include "stm32f10x.h"
#include "stm32f10x_conf.h"

#include "CircularBuffer.h"
#include "ATparser.h"
#include "3P_rs485.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

void USART1_IRQHandler(void);
void USART2_IRQHandler(void);

void init_usart1(void);
void init_usart2(void);
void init_iwdg(void);

void send_byte1(uint8_t b);
void usart_puts1(char* s);
void send_byte2(uint8_t b);
void usart_puts2(char *s);

void Send_ESP_Payload(void);
uint16_t Payload_crc16_update(uint16_t crc, uint8_t a);

void delay(unsigned long ms);

int readFunc(uint8_t *data);
int writeFunc(uint8_t *buffer, size_t size);
bool readableFunc(void);
void sleepFunc(int us);

void Read_VI(void);
void Read_PQS(void);
void Read_Wh_Varh_PF(void);

//ESP Reset counter
int count = 0;
int reset_cnt = 0;

// Buffer
char Debug_BUF[200];
uint8_t Command_BUF[10] = {'\0'};

// For Usart command 
bool data_ready = false;
uint8_t command_digit = 0;
uint8_t State = 1;
uint8_t data_set_number = 1;

// Char buffer for storing sensors data

char Va[10];
char Vb[10];
char Vc[10];

char Ia[10];
char Ib[10];
char Ic[10];

char P[10];
char Pa[10];
char Pb[10];
char Pc[10];

char Q[10];
char Qa[10];
char Qb[10];
char Qc[10];

char S[10];
char Sa[10];
char Sb[10];
char Sc[10];

char PF[10];
char PFa[10];
char PFb[10];
char PFc[10];

char Wh_p[20];
char Wh_n[20];

char Varh_p[20];
char Varh_n[20];


/////////////////////////////////////////////

circular_buf_t cbuf;
circular_buf_t cbuf_meter;
atparser_t parser;
uint8_t modbus_BUF[13] = {'\0'};

/////////////////////////////////////////////

//rs485
bool use_usart1 = false;
//ESP UART
bool use_usart2 = true;

/////////////////////////////////////////////

 /* ESP_Payload 
   
    "Data": {
      "Ia": 507,
      "Ib": 492,
      "Ic": 546,
      "P": 232,
      "PF": 703,
      "PFa": 700,
      "PFb": 685,
      "PFc": 695,
      "Pa": 76,
      "Pb": 72,
      "Pc": 83,
      "Q": 246,
      "Qa": 80,
      "Qb": 79,
      "Qc": 86,
      "S": 354,
      "Sa": 116,
      "Sb": 112,
      "Sc": 125,
      "Va": 2301,
      "Varh_negative": 0,
      "Varh_positive": 28884,
      "Vb": 2291,
      "Vc": 2298,
      "Wh_negative": 0,
      "Wh_positive": 86982
    }

*/

//////////////////////////////////////////////

void USART1_IRQHandler(void)
{
  //Store Current Status
  bool usart1_status = use_usart1;
  bool usart2_status = use_usart2;


  //switch to UART1
  use_usart1 = true;
  use_usart2 = false;
  
  char b;
  if (USART_GetFlagStatus(USART1, USART_FLAG_RXNE) == SET)
  {

    b = USART_ReceiveData(USART1);
    circular_buf_put(&cbuf_meter, b);
  }

  //Return Current Status
  use_usart1 = usart1_status;
  use_usart2 = usart2_status;

}

void USART2_IRQHandler(void)
{
    //Store Current Status
    bool usart1_status = use_usart1;
    bool usart2_status = use_usart2;


    //switch to UART2
    use_usart1 = false;
    use_usart2 = true;

    char b;
    if(USART_GetFlagStatus(USART2, USART_FLAG_RXNE) == SET) {

        b =  USART_ReceiveData(USART2);
        
        command_digit++;

        //Check maximum size of the command **you can arrange the maximum size of your command here** 
        if(command_digit > 9){
          atparser_flush(&parser);
          // usart_puts2("\nLong-Command -> Clear_RXBuffer\n\n");
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
                  // sprintf(Debug_BUF, "\nCommand size : %d    Command:  %s", command_size,Command_BUF);
                  // usart_puts2(Debug_BUF);

                  atparser_flush(&parser);
                  // usart_puts2("Clear_RXBuffer\n\n");
                  command_digit = 0 ;
                  data_ready = true;
                  
                }
                else{
                  atparser_flush(&parser);
                  // usart_puts2("\nShort-Command -> Clear_RXBuffer\n\n");
                  command_digit = 0 ;
                  State = 1;
                }

                State = 1;
              }

              else{
                atparser_flush(&parser);
                // usart_puts2("\nClear_RXBuffer\n\n");
                command_digit = 0 ;
                State = 1;
              }

              break;
          
          default:
              atparser_flush(&parser);
              // usart_puts2("\nClear_RXBuffer\n\n");
              command_digit = 0 ;
              State = 1;
              break;
      }
	  }

    //Return Current Status
    use_usart1 = usart1_status;
    use_usart2 = usart2_status;
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

void send_byte1(uint8_t b)
{
	/* Send one byte */
	USART_SendData(USART1, b);

	/* Loop until USART2 DR register is empty */
	while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
}


void usart_puts1(char* s)
{
    while(*s) {
    	send_byte1(*s);
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


//USART1 Modbus
void init_usart1()
{

  USART_InitTypeDef USART_InitStructure;
  GPIO_InitTypeDef GPIO_InitStructure;

  /* Enable peripheral clocks. */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE);
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);

  /* Configure USART1 Rx pin as floating input. */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
  GPIO_Init(GPIOA, &GPIO_InitStructure);

  /* Configure USART1 Tx as alternate function push-pull. */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
  GPIO_Init(GPIOA, &GPIO_InitStructure);

  /* Configure the USART1 */
  USART_InitStructure.USART_BaudRate = 9600;
  USART_InitStructure.USART_WordLength = USART_WordLength_8b;
  USART_InitStructure.USART_StopBits = USART_StopBits_1;
  USART_InitStructure.USART_Parity = USART_Parity_No;
  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
  USART_Init(USART1, &USART_InitStructure);

  NVIC_InitTypeDef NVIC_InitStructure;

  NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);

  /* Enable transmit and receive interrupts for the USART1. */
  USART_ITConfig(USART1, USART_IT_TXE, DISABLE);
  USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);

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

	/* Configure the USART2 */
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


// ATparser callback function
int readFunc(uint8_t *data)
{
  if(use_usart2){
    return circular_buf_get(&cbuf, data);
  }

  if(use_usart1){
    return circular_buf_get(&cbuf_meter, data);
  }
}

int writeFunc(uint8_t *buffer, size_t size)
{
  if(use_usart2){
    size_t i = 0;
    for (i = 0; i < size; i++)
    {
      send_byte2(buffer[i]);
    }
  }
  
  if(use_usart1){
    size_t i = 0;
    for (i = 0; i < size; i++)
    {
      send_byte1(buffer[i]);
    }
  }
  
  return 0;
}

bool readableFunc()
{
  if(use_usart2){
    return !circular_buf_empty(&cbuf);
  }
  if(use_usart1){
    return !circular_buf_empty(&cbuf_meter);
  }
}

void sleepFunc(int us)
{
  Delay_1us(us);
}



void Read_VI(void){

    uint16_t Int_Va;
    uint16_t Int_Vb;
    uint16_t Int_Vc;

    uint16_t Int_Ia;
    uint16_t Int_Ib;
    uint16_t Int_Ic;

    use_usart1 = true;
    use_usart2 = false;
    atparser_flush(&parser);

    if(rs485_3P_read_Volt(&Int_Va,&Int_Vb,&Int_Vc)){
      
      //Convert float to String//
      gcvt((float)(Int_Va/10.0),5,Va);
      gcvt((float)(Int_Vb/10.0),5,Vb);
      gcvt((float)(Int_Vc/10.0),5,Vc);

      }
    else{

      strcpy(Va,"\"N/A\""); 
      strcpy(Vb,"\"N/A\""); 
      strcpy(Vc,"\"N/A\""); 
    
    }

    // sprintf(Debug_BUF, "\nVa : %s Vb : %s Vc : %s \n", Va,Vb,Vc );
    // usart_puts2(Debug_BUF);
    atparser_set_timeout(&parser, 1000000); 
    atparser_flush(&parser);
    atparser_set_timeout(&parser, 8000000); 
    
    Delay_1us(100000);
    if(rs485_3P_read_Amp(&Int_Ia,&Int_Ib,&Int_Ic)){
      
      //Convert float to String//
      gcvt((float)(Int_Ia/1000.0),5,Ia);
      gcvt((float)(Int_Ib/1000.0),5,Ib);
      gcvt((float)(Int_Ic/1000.0),5,Ic);    

      }
    else{
      
      strcpy(Ia,"\"N/A\""); 
      strcpy(Ib,"\"N/A\""); 
      strcpy(Ic,"\"N/A\"");
      
    }

    // sprintf(Debug_BUF, "\nIa : %s Ib : %s Ic : %s \n", Ia,Ib,Ic );
    // usart_puts2(Debug_BUF);
    atparser_set_timeout(&parser, 1000000); 
    atparser_flush(&parser);
    atparser_set_timeout(&parser, 8000000); 


    use_usart1 = false;
    use_usart2 = true;
    atparser_flush(&parser);

    Delay_1us(100000);

}

void Read_PQS(void){

    uint16_t Int_P;
    uint16_t Int_Pa;
    uint16_t Int_Pb;
    uint16_t Int_Pc;

    uint16_t Int_Q;
    uint16_t Int_Qa;
    uint16_t Int_Qb;
    uint16_t Int_Qc;

    uint16_t Int_S;
    uint16_t Int_Sa;
    uint16_t Int_Sb;
    uint16_t Int_Sc;

    use_usart1 = true;
    use_usart2 = false;
    atparser_flush(&parser);

    if(rs485_3P_read_Watt(&Int_P,&Int_Pa,&Int_Pb,&Int_Pc)){
      
      //Convert float to String//
      gcvt((float)(Int_P/1000.0),5,P);
      gcvt((float)(Int_Pa/1000.0),5,Pa);
      gcvt((float)(Int_Pb/1000.0),5,Pb);
      gcvt((float)(Int_Pc/1000.0),5,Pc);

      }
    else{

      strcpy(P,"\"N/A\""); 
      strcpy(Pa,"\"N/A\""); 
      strcpy(Pb,"\"N/A\""); 
      strcpy(Pc,"\"N/A\""); 
    
    }

    // sprintf(Debug_BUF, "\nP : %s Pa : %s Pb : %s Pc : %s \n", P,Pa,Pb,Pc );
    // usart_puts2(Debug_BUF);
    atparser_set_timeout(&parser, 1000000); 
    atparser_flush(&parser);
    atparser_set_timeout(&parser, 8000000); 
    
    Delay_1us(100000);
    if(rs485_3P_read_Var(&Int_Q,&Int_Qa,&Int_Qb,&Int_Qc)){
      
      //Convert float to String//
      gcvt((float)(Int_Q/1000.0),5,Q);
      gcvt((float)(Int_Qa/1000.0),5,Qa);
      gcvt((float)(Int_Qb/1000.0),5,Qb);
      gcvt((float)(Int_Qc/1000.0),5,Qc);    

      }
    else{
      
      strcpy(Q,"\"N/A\""); 
      strcpy(Qa,"\"N/A\""); 
      strcpy(Qb,"\"N/A\""); 
      strcpy(Qc,"\"N/A\"");
      
    }

    // sprintf(Debug_BUF, "\nQ : %s Qa : %s Qb : %s Qc : %s \n", Q,Qa,Qb,Qc );
    // usart_puts2(Debug_BUF);
    atparser_set_timeout(&parser, 1000000); 
    atparser_flush(&parser);
    atparser_set_timeout(&parser, 8000000); 

    Delay_1us(100000);
    if(rs485_3P_read_VA(&Int_S,&Int_Sa,&Int_Sb,&Int_Sc)){

      //Convert float to String//
      gcvt((float)(Int_S/1000.0),5,S);
      gcvt((float)(Int_Sa/1000.0),5,Sa);
      gcvt((float)(Int_Sb/1000.0),5,Sb);
      gcvt((float)(Int_Sc/1000.0),5,Sc);   

      }
    else{

      strcpy(S,"\"N/A\""); 
      strcpy(Sa,"\"N/A\""); 
      strcpy(Sb,"\"N/A\""); 
      strcpy(Sc,"\"N/A\"");
    
    }

    // sprintf(Debug_BUF, "\nS : %s Sa : %s Sb : %s Sc : %s \n", S,Sa,Sb,Sc );
    // usart_puts2(Debug_BUF);
    atparser_set_timeout(&parser, 1000000); 
    atparser_flush(&parser);
    atparser_set_timeout(&parser, 8000000); 


    use_usart1 = false;
    use_usart2 = true;
    atparser_flush(&parser);

    Delay_1us(100000);

}

void Read_Wh_Varh_PF(void){
    uint16_t Int_PF;
    uint16_t Int_PFa;
    uint16_t Int_PFb;
    uint16_t Int_PFc;

    uint32_t Int_Wh_p;
    uint32_t Int_Wh_n;

    uint32_t Int_Varh_p;
    uint32_t Int_Varh_n;

    use_usart1 = true;
    use_usart2 = false;
    atparser_flush(&parser);

    if(rs485_3P_readPF(&Int_PF,&Int_PFa,&Int_PFb,&Int_PFc)){

      //Convert float to String//
      gcvt((float)(Int_PF/1000.0),5,PF);
      gcvt((float)(Int_PFa/1000.0),5,PFa);
      gcvt((float)(Int_PFb/1000.0),5,PFb);
      gcvt((float)(Int_PFc/1000.0),5,PFc);    

      }
    else{

      strcpy(PF,"\"N/A\""); 
      strcpy(PFa,"\"N/A\""); 
      strcpy(PFb,"\"N/A\""); 
      strcpy(PFc,"\"N/A\"");

    }

    // sprintf(Debug_BUF, "\nPF : %s PFa : %s PFb : %s PFc : %s \n", PF,PFa,PFb,PFc );
    // usart_puts2(Debug_BUF);
    atparser_set_timeout(&parser, 1000000); 
    atparser_flush(&parser);
    atparser_set_timeout(&parser, 8000000); 
    
    Delay_1us(100000);
    if(rs485_3P_readWh(&Int_Wh_p,&Int_Wh_n)){
    
      //Convert float to String//
      gcvt((float)(Int_Wh_p/1.0),10,Wh_p);
      gcvt((float)(Int_Wh_n/1.0),10,Wh_n); 

      }
    else{

      strcpy(Wh_p,"\"N/A\""); 
      strcpy(Wh_n,"\"N/A\""); 

    }
   
    // sprintf(Debug_BUF, "\nWh_p : %s Wh_n : %s \n", Wh_p,Wh_n );
    // usart_puts2(Debug_BUF);
    atparser_set_timeout(&parser, 1000000); 
    atparser_flush(&parser);
    atparser_set_timeout(&parser, 8000000); 

    Delay_1us(100000);
    if(rs485_3P_readVarh(&Int_Varh_p,&Int_Varh_n)){

      //Convert float to String//
      gcvt((float)(Int_Varh_p/1.0),10,Varh_p);
      gcvt((float)(Int_Varh_n/1.0),10,Varh_n); 
   

      }
    else{

      strcpy(Varh_p,"\"N/A\""); 
      strcpy(Varh_n,"\"N/A\""); 

    }

    //sprintf(Debug_BUF, "\nVarh_p : %s Varh_n : %s \n", Varh_p,Varh_n );
    //usart_puts2(Debug_BUF);
    atparser_set_timeout(&parser, 1000000); 
    atparser_flush(&parser);
    atparser_set_timeout(&parser, 8000000);  

    use_usart1 = false;
    use_usart2 = true;
    atparser_flush(&parser);

}

void Send_ESP_Payload(void)
{
  char Payload [200];

  switch (data_set_number)
  {
    case 1:
        sprintf(Payload, "{\"Data\": {\"Va\": %s,\"Vb\": %s,\"Vc\": %s,\"Ia\": %s,\"Ib\": %s,\"Ic\": %s}}", Va,Vb,Vc,Ia,Ib,Ic );            
        break;

    case 2:
        sprintf(Payload, "{\"Data\": {\"P\": %s,\"Pa\": %s,\"Pb\": %s,\"Pc\": %s,\"Q\": %s,\"Qa\": %s,\"Qb\": %s,\"Qc\": %s,\"S\": %s,\"Sa\": %s,\"Sb\": %s,\"Sc\": %s}}", P,Pa,Pb,Pc,Q,Qa,Qb,Qc,S,Sa,Sb,Sc );
        break;

    case 3:
        sprintf(Payload, "{\"Data\": {\"PF\": %s,\"PFa\": %s,\"PFb\": %s,\"PFc\": %s,\"Wh_positive\": %s,\"Wh_negative\": %s,\"Varh_positive\": %s,\"Varh_negative\": %s}}", PF,PFa,PFb,PFc,Wh_p,Wh_n,Varh_p,Varh_n );
        break;

    default:              
        return;
  }

  // sprintf(Debug_BUF, "\nPayload size :  %d \n", strlen(Payload) );
  // usart_puts2(Debug_BUF);

  uint16_t crc = 0xFFFF;
  uint8_t i;
    for(i = 0; i < strlen(Payload); i++){
        crc = Payload_crc16_update(crc, (uint8_t)Payload[i]);
      }

  // sprintf(Debug_BUF, "\nPayload CRC :  %X \n",crc );
  // usart_puts2(Debug_BUF);

  // convert CRC16 to 2byte 
  uint8_t  crc8[2];
  crc8[0]= (uint8_t)crc;
  crc8[1]=(crc >> 8);

  // sprintf(Debug_BUF, "\nESP_Payload CRC :  %X , %X\n",crc8[1],crc8[0] );
  // usart_puts2(Debug_BUF);

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
uint16_t Payload_crc16_update(uint16_t crc, uint8_t a) {
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


  circular_buf_init(&cbuf);
  circular_buf_init(&cbuf_meter);
  atparser_init(&parser, readFunc, writeFunc, readableFunc, sleepFunc);
  rs485_3P_init(&parser, &modbus_BUF[0]);


  // usart_puts2("\r\nSTART\r\n");
  
 
  /* Reset ESP*/
  // usart_puts2("\r\nReset ESP\r\n");
  GPIO_ResetBits(GPIOA, GPIO_Pin_8);
  Delay_1us(2000000);
  GPIO_SetBits(GPIOA, GPIO_Pin_8);

   /* Wait ESP ready*/
  IWDG_ReloadCounter();
  Delay_1us(6000000);

  memset(Command_BUF, 0, sizeof(Command_BUF));
  atparser_flush(&parser);
  // usart_puts2("\r\nStart\r\n");



///////////////////////////////////////////////////////////////////////////////////////////
/*   TEST  MODBUS READ   */  
  //   while (1)
  // { 
  //   Read_VI();
  //   Send_ESP_Payload();
  //   usart_puts2("\n");
  //   data_set_number++;  
  //   Delay_1us(2000000);

  //   Read_PQS();
  //   Send_ESP_Payload();
  //   usart_puts2("\n");
  //   data_set_number++;
  //   Delay_1us(2000000);

  //   Read_Wh_Varh_PF();
  //   Send_ESP_Payload();
  //   usart_puts2("\n");
  //   data_set_number = 1;
  //   Delay_1us(2000000);
  // }
///////////////////////////////////////////////////////////////////////////////////////////

  while (1)
  { 

   IWDG_ReloadCounter(); 
   // sprintf(Debug_BUF, "count is : %d", count);
   // usart_puts2(Debug_BUF);
   // usart_puts2("\n");
   // sprintf(Debug_BUF, "reset cnt is : %d", reset_cnt);
   // usart_puts2(Debug_BUF);
   // usart_puts2("\n");

   if (data_ready) {

      if (!strcmp((char*)Command_BUF, "GetData\r\n")) {
        // usart_puts2("GET_DATA: OK\n");

        memset(Command_BUF, 0, sizeof(Command_BUF));
        atparser_flush(&parser);
        ///////////////////////////////////////////////////////
        switch (data_set_number)
        {
          case 1:
              Read_VI();
              Send_ESP_Payload();
              data_set_number++;              
              break;

          case 2:
              Read_PQS();
              Send_ESP_Payload();
              data_set_number++;
              break;

          case 3:
              Read_Wh_Varh_PF();
              Send_ESP_Payload();
              data_set_number = 1;
              break;

          default:              
              data_set_number = 1;
              break;
        }
        //////////////////////////////////////////////////////
        data_ready = false;
        count = 0;
      }

      else if (!strcmp((char*)Command_BUF, "WiFierr\r\n")) {
        // usart_puts2("WiFi_Error: OK\n");

        memset(Command_BUF, 0, sizeof(Command_BUF));
        atparser_flush(&parser);
        data_ready = false;
        count = 0;
      }

      else if (!strcmp((char*)Command_BUF, "RegisGW\r\n")) {
        // usart_puts2("Regis_GW: OK\n");

        memset(Command_BUF, 0, sizeof(Command_BUF));
        atparser_flush(&parser);
        data_ready = false;
        count = 0;
      }

      else{
        // usart_puts2("Unknown Command\n");        
        memset(Command_BUF, 0, sizeof(Command_BUF));
        atparser_flush(&parser);
        data_ready = false;
        count++;
      }

      // usart_puts2("\n");

   }

   else {
     count++;
   } 



   if(count >= 600){

   IWDG_ReloadCounter();

   // usart_puts2("\r\nReset ESP\r\n");
   GPIO_ResetBits(GPIOA, GPIO_Pin_8);
   Delay_1us(2000000);
   GPIO_SetBits(GPIOA, GPIO_Pin_8);
   Delay_1us(6000000);
   // usart_puts2("\r\nStart\r\n");

   memset(Command_BUF, 0, sizeof(Command_BUF));
   atparser_flush(&parser);

   reset_cnt++;
   count = 0;

   }

   Delay_1us(500000);

  }
}
