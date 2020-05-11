#include "stm32f10x.h"
#include "stm32f10x_conf.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

void USART2_IRQHandler(void);

void init_usart1(void);
void init_usart2(void);
void init_iwdg(void);
void NVIC_Configuration(void);

void send_byte(uint8_t b);
void usart_puts(char* s);
void clear_RXBuffer(void);
void clear_dataBuffer(void);
void copy_string(char *target, char *source); 
void delay(unsigned long ms);

int count = 0;
int reset_cnt = 0;
char out[20];
uint8_t RXC = 0;
char RX_BUF[3] = {'\0'};
char data_BUF[3] = {'\0'};
bool data_ready = false;

void clear_RXBuffer(void) {
	uint8_t RXi = 0;
	usart_puts("clear\n");
	usart_puts(RX_BUF);
	send_byte('\n');
	for (RXi=0; RXi<3; RXi++)
		RX_BUF[RXi] = '\0';

	RXC = 0;
}

void clear_dataBuffer(void) {
	uint8_t RXi = 0;
	usart_puts("clear\n");
	usart_puts(data_BUF);
	send_byte('\n');
	for (RXi=0; RXi<3; RXi++)
		data_BUF[RXi] = '\0';

	data_ready = false;
}

void copy_string(char *target, char *source) {
    while (*source) {
        *target = *source;
        source++;
        target++;
    }
    *target = '\0';
    data_ready = true;
}

void USART2_IRQHandler(void)
{
    char b;

    if(USART_GetFlagStatus(USART2, USART_FLAG_RXNE) == SET) {

        b =  USART_ReceiveData(USART2);
      	RX_BUF[RXC] = b;
    	  RXC++;
        /* Uncomment this to loopback */
        send_byte(b);
        send_byte('\n');
        usart_puts(RX_BUF);
        send_byte('\n');

        if (RXC >= 2) {
        	copy_string(data_BUF,RX_BUF);
        	clear_RXBuffer();
        }

	}
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
  IWDG_SetPrescaler(IWDG_Prescaler_128); // 4, 8, 16 ... 256
  IWDG_SetReload(0x0FFF);//This parameter must be a number between 0 and 0x0FFF.
  IWDG_ReloadCounter();
  IWDG_Enable();

}


void NVIC_Configuration(void) {

    NVIC_SetPriorityGrouping(NVIC_PriorityGroup_4);

    NVIC_SetPriority(USART2_IRQn, NVIC_EncodePriority(4,1,0));

}


int main(void)
{
  /* Initialize Leds mounted on STM32 board */
  GPIO_InitTypeDef  GPIO_InitStructure;
  /* Initialize Reset which connected to PA8, Enable the Clock*/
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
  /* Configure the GPIO_LED pin */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOA, &GPIO_InitStructure);

  /* Disable reset */
  GPIO_SetBits(GPIOA, GPIO_Pin_8);

  init_iwdg();
  init_usart1();
  init_usart2();
  NVIC_Configuration();

  usart_puts("\r\nSTART\r\n");

  /* Reset ESP*/
  GPIO_ResetBits(GPIOA, GPIO_Pin_8);
  delay(5000);
  GPIO_SetBits(GPIOA, GPIO_Pin_8);

   /* Wait ESP ready*/
  IWDG_ReloadCounter();
  delay(5000);


  while (1)
  { 

   IWDG_ReloadCounter(); 
   sprintf(out, "count is : %d", count);
   usart_puts(out);
   usart_puts("\n");
   sprintf(out, "reset cnt is : %d", reset_cnt);
   usart_puts(out);
   usart_puts("\n");

   if (data_ready) {
   	
   		if(data_BUF[0] == 'O' && data_BUF[1] == 'K'){
   			count = 0;
   			clear_dataBuffer();
   		}
   		else{
   			usart_puts("\r\nNOT OK\r\n");
   			count++;
   			clear_RXBuffer();
   			clear_dataBuffer();
   		}
   }

   else{

   	count++;
   
   }

   if(count >= 300){

   IWDG_ReloadCounter();
   clear_RXBuffer();
   clear_dataBuffer(); 
   GPIO_ResetBits(GPIOA, GPIO_Pin_8);
   delay(5000);
   GPIO_SetBits(GPIOA, GPIO_Pin_8);
   reset_cnt++;
   count = 0;

   }

   delay(1000);

  }
}
