#ifndef _3P_RS485_H_
#define _3P_RS485_H_

#include "stm32f10x.h"
#include "ATparser.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

void rs485_3P_init(atparser_t *P, uint8_t *buf);
bool rs485_3P_read_Watt(uint16_t * P , uint16_t * Pa , uint16_t* Pb , uint16_t* Pc);
bool rs485_3P_read_Var(uint16_t * Q , uint16_t * Qa , uint16_t* Qb , uint16_t* Qc);
bool rs485_3P_read_VA(uint16_t * S , uint16_t * Sa , uint16_t* Sb , uint16_t* Sc);
bool rs485_3P_read_Volt(uint16_t * Va , uint16_t* Vb , uint16_t* Vc );
bool rs485_3P_read_Amp(uint16_t * Ia , uint16_t* Ib , uint16_t* Ic );
bool rs485_3P_readPF(uint16_t * PF , uint16_t * PFa , uint16_t* PFb , uint16_t* PFc);
bool rs485_3P_readWh(uint32_t * Wh_p , uint32_t * Wh_n );
bool rs485_3P_readVarh(uint32_t * Varh_p , uint32_t * Varh_n );
uint16_t crc16_update(uint16_t crc, uint8_t a); 


#endif