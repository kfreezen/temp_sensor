/* 
 * File:   crc16.h
 * Author: od
 *
 * Created on June 15, 2013, 5:35 PM
 */

#ifndef CRC16_H
#define	CRC16_H

/*
 * CRC16 "Register". This is implemented as two 8bit values
 */
//unsigned char CRC16_High, CRC16_Low;

void CRC16_Init( void );
void CRC16_Update4Bits( unsigned char val );
void CRC16_Update( unsigned char val );
void CRC16_Generate(unsigned char* msg, int len);
unsigned char CRC16_GetHigh();
unsigned char CRC16_GetLow();

#endif	/* CRC16_H */

