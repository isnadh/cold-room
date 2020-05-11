#include "stm32f10x.h"
#include "stm32f10x_conf.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "CircularBuffer.h"
#include "ATparser.h"
#include "3P_rs485.h"


///////////////////////////////////

uint8_t data_BUF[13] = {'\0'};

uint16_t Va;
uint16_t Vb;
uint16_t Vc;

uint16_t Ia;
uint16_t Ib;
uint16_t Ic;

uint16_t P;
uint16_t Pa;
uint16_t Pb;
uint16_t Pc;

uint16_t Q;
uint16_t Qa;
uint16_t Qb;
uint16_t Qc;

uint16_t S;
uint16_t Sa;
uint16_t Sb;
uint16_t Sc;

uint16_t PF;
uint16_t PFa;
uint16_t PFb;
uint16_t PFc;

uint32_t Wh_p;
uint32_t Wh_n;

uint32_t Varh_p;
uint32_t Varh_n;

///////////////////////////////////

circular_buf_t cbuf;
circular_buf_t cbuf_meter;
atparser_t parser;
//rs485
bool use_usart1 = false;
//ESP UART
bool use_usart2 = true;
//Debug
char Debug_BUF[100];

///////////////////////////////////

void init_usart1(void);
void init_usart2(void);

void send_byte1(uint8_t b);
void send_byte2(uint8_t b);
void usart_puts1(char *s);
void usart_puts2(char *s);

void USART1_IRQHandler(void);
void USART2_IRQHandler(void);

int readFunc(uint8_t *data);
int writeFunc(uint8_t *buffer, size_t size);
bool readableFunc(void);
void sleepFunc(int us);

//////////////////////////////////



static inline void Delay_1us(uint32_t nCnt_1us)
{
  volatile uint32_t nCnt;

  for (; nCnt_1us != 0; nCnt_1us--)
    for (nCnt = 4; nCnt != 0; nCnt--)
      ;
}

void send_byte1(uint8_t b)
{
  /* Send one byte */
  USART_SendData(USART1, b);

  /* Loop until USART2 DR register is empty */
  while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET)
    ;
}

void send_byte2(uint8_t b)
{
  /* Send one byte */
  USART_SendData(USART2, b);

  /* Loop until USART2 DR register is empty */
  while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET)
    ;
}

void usart_puts1(char *s)
{
  while (*s)
  {
    send_byte1(*s);
    s++;
  }
}

void usart_puts2(char *s)
{
  while (*s)
  {
    send_byte2(*s);
    s++;
  }
}


void USART1_IRQHandler(void)
{
  char b;
  if (USART_GetFlagStatus(USART1, USART_FLAG_RXNE) == SET)
  {

    b = USART_ReceiveData(USART1);
    circular_buf_put(&cbuf_meter, b);
  }
}

void USART2_IRQHandler(void)
{
  char b;
  if (USART_GetFlagStatus(USART2, USART_FLAG_RXNE) == SET)
  {

    b = USART_ReceiveData(USART2);
    circular_buf_put(&cbuf, b);
  }
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
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
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


int main(void)
{
    
    circular_buf_init(&cbuf_meter);
    atparser_init(&parser, readFunc, writeFunc, readableFunc, sleepFunc);
    rs485_3P_init(&parser, &data_BUF[0]);

    init_usart1();
    init_usart2();

    while(1)
    {
        
    use_usart1 = true;
    use_usart2 = false;
    atparser_flush(&parser);

    if(rs485_3P_read_Volt(&Va,&Vb,&Vc)){
      
      Va = Va;
      Vb = Vb;
      Vc = Vc;

      }
    else{

      Va = 0;
      Vb = 0;
      Vc = 0;
      
      atparser_set_timeout(&parser, 1000000); 
      atparser_flush(&parser);
      atparser_set_timeout(&parser, 8000000); 
    
    }

    sprintf(Debug_BUF, "\nVa : %d Vb : %d Vc : %d \n", Va,Vb,Vc );
    usart_puts2(Debug_BUF);


    Delay_1us(100000);

    if(rs485_3P_read_Amp(&Ia,&Ib,&Ic)){
    
      Ia = Ia;
      Ib = Ib;
      Ic = Ic;

      }
    else{

      Ia = 0;
      Ib = 0;
      Ic = 0;
      
      atparser_set_timeout(&parser, 1000000); 
      atparser_flush(&parser);
      atparser_set_timeout(&parser, 8000000); 
    }

    sprintf(Debug_BUF, "\nIa : %d Ib : %d Ic : %d \n", Ia,Ib,Ic );
    usart_puts2(Debug_BUF);

    Delay_1us(100000);

    if(rs485_3P_read_Watt(&P,&Pa,&Pb,&Pc)){

      P = P;
      Pa = Pa;
      Pb = Pb;
      Pc = Pc;
      
      }
    else{

      P = 0;
      Pa = 0;
      Pb = 0;
      Pc = 0;
    
      atparser_set_timeout(&parser, 1000000); 
      atparser_flush(&parser);
      atparser_set_timeout(&parser, 8000000); 
     
    }

    sprintf(Debug_BUF, "\nP : %d Pa : %d Pb : %d Pc : %d \n", P,Pa,Pb,Pc );
    usart_puts2(Debug_BUF);

    Delay_1us(100000);

    if(rs485_3P_read_Var(&Q,&Qa,&Qb,&Qc)){

      Q = Q;
      Qa = Qa;
      Qb = Qb;
      Qc = Qc;

      }
    else{

      Q = 0;
      Qa = 0;
      Qb = 0;
      Qc = 0;
      
      atparser_set_timeout(&parser, 1000000); 
      atparser_flush(&parser);
      atparser_set_timeout(&parser, 8000000); 
    
    }

    sprintf(Debug_BUF, "\nQ : %d Qa : %d Qb : %d Qc : %d \n", Q,Qa,Qb,Qc );
    usart_puts2(Debug_BUF);

    Delay_1us(100000);

    if(rs485_3P_read_VA(&S,&Sa,&Sb,&Sc)){

      S = S;
      Sa = Sa;
      Sb = Sb;
      Sc = Sc;

      }
    else{

      S = 0;
      Sa = 0;
      Sb = 0;
      Sc = 0;
     
      atparser_set_timeout(&parser, 1000000); 
      atparser_flush(&parser);
      atparser_set_timeout(&parser, 8000000); 
    
    }

    sprintf(Debug_BUF, "\nS : %d Sa : %d Sb : %d Sc : %d \n", S,Sa,Sb,Sc );
    usart_puts2(Debug_BUF);

    Delay_1us(100000);

    if(rs485_3P_readPF(&PF,&PFa,&PFb,&PFc)){

      PF = PF;
      PFa = PFa;
      PFb = PFb;
      PFc = PFc;    

      }
    else{

      PF = 0;
      PFa = 0;
      PFb = 0;
      PFc = 0;

      atparser_set_timeout(&parser, 1000000); 
      atparser_flush(&parser);
      atparser_set_timeout(&parser, 8000000); 

    }

    sprintf(Debug_BUF, "\nPF : %d PFa : %d PFb : %d PFc : %d \n", PF,PFa,PFb,PFc );
    usart_puts2(Debug_BUF);

    Delay_1us(100000);

    if(rs485_3P_readWh(&Wh_p,&Wh_n)){
    
      Wh_p = Wh_p;
      Wh_n = Wh_n; 

      }
    else{

      Wh_p = 0;
      Wh_n = 0;  

      atparser_set_timeout(&parser, 1000000); 
      atparser_flush(&parser);
      atparser_set_timeout(&parser, 8000000); 

    }
   
    sprintf(Debug_BUF, "\nWh_p : %ld Wh_n : %ld \n", Wh_p,Wh_n );
    usart_puts2(Debug_BUF);

    Delay_1us(100000);
 
    if(rs485_3P_readVarh(&Varh_p,&Varh_n)){

      Varh_p = Varh_p;
      Varh_n = Varh_n;   

      }
    else{

      Varh_p = 0;
      Varh_n = 0;
      
      atparser_set_timeout(&parser, 1000000); 
      atparser_flush(&parser);
      atparser_set_timeout(&parser, 8000000); 

    }

    sprintf(Debug_BUF, "\nVarh_p : %ld Varh_n : %ld \n", Varh_p,Varh_n );
    usart_puts2(Debug_BUF);

    Delay_1us(100000);
   

    use_usart1 = false;
    use_usart2 = true;
    atparser_flush(&parser);


    }
}


