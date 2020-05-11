#include "3P_rs485.h"
#include "ATparser.h"

atparser_t *Parser;
uint8_t *data_buf;

uint8_t read_Volt[8] = {0x01, 0x03, 0x00, 0x00, 0x00, 0x03, 0x05, 0xCB};
uint8_t read_Amp[8] = {0x01, 0x03, 0x00, 0x03, 0x00, 0x03, 0xF5, 0xCB};

uint8_t read_Watt[8] = {0x01, 0x03, 0x00, 0x07, 0x00, 0x04, 0xF5, 0xC8};
uint8_t read_Var[8] = {0x01, 0x03, 0x00, 0x0B, 0x00, 0x04, 0x35, 0xCB};
uint8_t read_VA[8] = {0x01, 0x03, 0x00, 0x0F, 0x00, 0x04, 0x74, 0x0A};

uint8_t read_PF[8] = {0x01, 0x03, 0x00, 0x13, 0x00, 0x04, 0xB5, 0xCC};

uint8_t read_Wh[8] = {0x01, 0x03, 0x00, 0x1D, 0x00, 0x04, 0xD4, 0x0F};
uint8_t read_Varh[8] = {0x01, 0x03, 0x00, 0x21, 0x00, 0x04, 0x14, 0x03};



static void delay(unsigned long ms)
{
  volatile unsigned long i,j;
  for (i = 0; i < ms; i++ )
  for (j = 0; j < 1000; j++ );
}


void rs485_3P_init(atparser_t *P, uint8_t *buf) {
	Parser = P;
	data_buf = buf;
}



bool rs485_3P_read_Watt(uint16_t * P , uint16_t * Pa , uint16_t* Pb , uint16_t* Pc){

  //send command  
  atparser_write(Parser, &read_Watt[0], 8);
  delay(100);
  //received data
  atparser_set_timeout(Parser, 1000000); 
  int ret = atparser_read(Parser, data_buf, 13);
  atparser_set_timeout(Parser, 8000000); 

  if (ret != -1){
      
    uint16_t crc = 0xFFFF;
    int i;
    for(i = 0; i < 11; i++){
        crc = crc16_update(crc, data_buf[i]);
  	  }


    if( crc == ( ( (uint16_t)data_buf[12] )<<8 | (uint16_t)data_buf[11] ) ){
    	
    	*P = (((uint16_t)data_buf[3])<<8 | (uint16_t)data_buf[4] );
      *Pa = (((uint16_t)data_buf[5])<<8 | (uint16_t)data_buf[6] );
      *Pb = (((uint16_t)data_buf[7])<<8 | (uint16_t)data_buf[8] );
      *Pc = (((uint16_t)data_buf[9])<<8 | (uint16_t)data_buf[10] );

    	return true;
  	  
      }
  	else{	
  		
  		return false;
  	  }


  	}

  	else{
  		
  		return false;
  	}

}

bool rs485_3P_read_Var(uint16_t * Q , uint16_t * Qa , uint16_t* Qb , uint16_t* Qc){

  //send command  
  atparser_write(Parser, &read_Var[0], 8);
  delay(100);
  //received data
  atparser_set_timeout(Parser, 1000000); 
  int ret = atparser_read(Parser, data_buf, 13);
  atparser_set_timeout(Parser, 8000000); 

  if (ret != -1){
      
    uint16_t crc = 0xFFFF;
    int i;
    for(i = 0; i < 11; i++){
        crc = crc16_update(crc, data_buf[i]);
      }


    if( crc == ( ( (uint16_t)data_buf[12] )<<8 | (uint16_t)data_buf[11] ) ){
      
      *Q = (((uint16_t)data_buf[3])<<8 | (uint16_t)data_buf[4] );
      *Qa = (((uint16_t)data_buf[5])<<8 | (uint16_t)data_buf[6] );
      *Qb = (((uint16_t)data_buf[7])<<8 | (uint16_t)data_buf[8] );
      *Qc = (((uint16_t)data_buf[9])<<8 | (uint16_t)data_buf[10] );

      return true;
      
      }
    else{ 
      
      return false;
      }


    }

    else{
      
      return false;
    }

}

bool rs485_3P_read_VA(uint16_t * S , uint16_t * Sa , uint16_t* Sb , uint16_t* Sc){

  //send command  
  atparser_write(Parser, &read_VA[0], 8);
  delay(100);
  //received data
  atparser_set_timeout(Parser, 1000000); 
  int ret = atparser_read(Parser, data_buf, 13);
  atparser_set_timeout(Parser, 8000000); 

  if (ret != -1){
      
    uint16_t crc = 0xFFFF;
    int i;
    for(i = 0; i < 11; i++){
        crc = crc16_update(crc, data_buf[i]);
      }


    if( crc == ( ( (uint16_t)data_buf[12] )<<8 | (uint16_t)data_buf[11] ) ){
      
      *S = (((uint16_t)data_buf[3])<<8 | (uint16_t)data_buf[4] );
      *Sa = (((uint16_t)data_buf[5])<<8 | (uint16_t)data_buf[6] );
      *Sb = (((uint16_t)data_buf[7])<<8 | (uint16_t)data_buf[8] );
      *Sc = (((uint16_t)data_buf[9])<<8 | (uint16_t)data_buf[10] );

      return true;
      
      }
    else{ 
      
      return false;
      }


    }

    else{
      
      return false;
    }

}

bool rs485_3P_read_Volt(uint16_t * Va , uint16_t* Vb , uint16_t* Vc ){

  //send command  
  atparser_write(Parser, &read_Volt[0], 8);
  delay(100);
  //received data
  atparser_set_timeout(Parser, 1000000); 
  int ret = atparser_read(Parser, data_buf, 11);
  atparser_set_timeout(Parser, 8000000); 

  if (ret != -1){   
    uint16_t crc = 0xFFFF;
    int i;
    for(i = 0; i < 9; i++){
        crc = crc16_update(crc, data_buf[i]);
      }


    if( crc == ( ( (uint16_t)data_buf[10] )<<8 | (uint16_t)data_buf[9] ) ){
      
      *Va = (((uint16_t)data_buf[3])<<8 | (uint16_t)data_buf[4] );
      *Vb = (((uint16_t)data_buf[5])<<8 | (uint16_t)data_buf[6] );
      *Vc = (((uint16_t)data_buf[7])<<8 | (uint16_t)data_buf[8] );

      return true;
      
      }
    else{ 
      
      return false;
      }


    }

    else{
      
      return false;
    }

}

bool rs485_3P_read_Amp(uint16_t * Ia , uint16_t* Ib , uint16_t* Ic ){

 //send command  
  atparser_write(Parser, &read_Amp[0], 8);
  delay(100);
  //received data
  atparser_set_timeout(Parser, 1000000); 
  int ret = atparser_read(Parser, data_buf, 11);
  atparser_set_timeout(Parser, 8000000); 

  if (ret != -1){
      
    uint16_t crc = 0xFFFF;
    int i;
    for(i = 0; i < 9; i++){
        crc = crc16_update(crc, data_buf[i]);
      }


    if( crc == ( ( (uint16_t)data_buf[10] )<<8 | (uint16_t)data_buf[9] ) ){
      
      *Ia = (((uint16_t)data_buf[3])<<8 | (uint16_t)data_buf[4] );
      *Ib = (((uint16_t)data_buf[5])<<8 | (uint16_t)data_buf[6] );
      *Ic = (((uint16_t)data_buf[7])<<8 | (uint16_t)data_buf[8] );

      return true;
      
      }
    else{ 
      
      return false;
      }


    }

    else{
      
      return false;
    }

}

bool rs485_3P_readPF(uint16_t * PF , uint16_t * PFa , uint16_t* PFb , uint16_t* PFc){

  //send command  
  atparser_write(Parser, &read_PF[0], 8);
  delay(100);
  //received data
  atparser_set_timeout(Parser, 1000000); 
  int ret = atparser_read(Parser, data_buf, 13);
  atparser_set_timeout(Parser, 8000000); 

  if (ret != -1){
      
    uint16_t crc = 0xFFFF;
    int i;
    for(i = 0; i < 11; i++){
        crc = crc16_update(crc, data_buf[i]);
      }


    if( crc == ( ( (uint16_t)data_buf[12] )<<8 | (uint16_t)data_buf[11] ) ){
      
      *PF = (((uint16_t)data_buf[3])<<8 | (uint16_t)data_buf[4] );
      *PFa = (((uint16_t)data_buf[5])<<8 | (uint16_t)data_buf[6] );
      *PFb = (((uint16_t)data_buf[7])<<8 | (uint16_t)data_buf[8] );
      *PFc = (((uint16_t)data_buf[9])<<8 | (uint16_t)data_buf[10] );

      return true;
      
      }
    else{ 
      
      return false;
      }


    }

    else{
      
      return false;
    }

}

bool rs485_3P_readWh(uint32_t * Wh_p , uint32_t * Wh_n ){

  //send command  
  atparser_write(Parser, &read_Wh[0], 8);
  delay(100);
  //received data
  atparser_set_timeout(Parser, 1000000); 
  int ret = atparser_read(Parser, data_buf, 13);
  atparser_set_timeout(Parser, 8000000); 

  if (ret != -1){
      
    uint16_t crc = 0xFFFF;
    int i;
    for(i = 0; i < 11; i++){
        crc = crc16_update(crc, data_buf[i]);
      }


    if( crc == ( ( (uint16_t)data_buf[12] )<<8 | (uint16_t)data_buf[11] ) ){
      
      *Wh_p = (((uint32_t)data_buf[3])<<24 | ((uint32_t)data_buf[4])<<16 | ((uint32_t)data_buf[5])<<8 | (uint32_t)data_buf[6] );
      *Wh_n = (((uint32_t)data_buf[7])<<24 | ((uint32_t)data_buf[8])<<16 | ((uint32_t)data_buf[9])<<8 | (uint32_t)data_buf[10] );

      return true;
      
      }
    else{ 
      
      return false;
      }


    }

    else{
      
      return false;
    }

}

bool rs485_3P_readVarh(uint32_t * Varh_p , uint32_t * Varh_n ){

  //send command  
  atparser_write(Parser, &read_Varh[0], 8);
  delay(100);
  //received data
  atparser_set_timeout(Parser, 1000000); 
  int ret = atparser_read(Parser, data_buf, 13);
  atparser_set_timeout(Parser, 8000000); 

  if (ret != -1){
      
    uint16_t crc = 0xFFFF;
    int i;
    for(i = 0; i < 11; i++){
        crc = crc16_update(crc, data_buf[i]);
      }


    if( crc == ( ( (uint16_t)data_buf[12] )<<8 | (uint16_t)data_buf[11] ) ){
      
      *Varh_p = (((uint32_t)data_buf[3])<<24 | ((uint32_t)data_buf[4])<<16 | ((uint32_t)data_buf[5])<<8 | (uint32_t)data_buf[6] );
      *Varh_n = (((uint32_t)data_buf[7])<<24 | ((uint32_t)data_buf[8])<<16 | ((uint32_t)data_buf[9])<<8 | (uint32_t)data_buf[10] );

      return true;
      
      }
    else{ 
      
      return false;
      }


    }

    else{
      
      return false;
    }

}


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






































