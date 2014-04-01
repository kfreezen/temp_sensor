/***

Credit for this code goes to:
Ashley Roll
Digital Nemisis Pty Ltd
www.digitalnemesis.com
ash@digitalnemesis.com

***/

#include "crc16.h"
/* 
 * CRC16 Lookup tables (High and Low Byte) for 4 bits per iteration. 
 */
unsigned short CRC16_LookupHigh[16] = {
        0x00, 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70,
        0x81, 0x91, 0xA1, 0xB1, 0xC1, 0xD1, 0xE1, 0xF1
};
unsigned short CRC16_LookupLow[16] = {
        0x00, 0x21, 0x42, 0x63, 0x84, 0xA5, 0xC6, 0xE7,
        0x08, 0x29, 0x4A, 0x6B, 0x8C, 0xAD, 0xCE, 0xEF
};

/*
 * CRC16 "Register". This is implemented as two 8bit values
 */
unsigned char CRC16_High = 0xFF, CRC16_Low = 0xFF;

/*
 * Before each message CRC is generated, the CRC register must be 
 * initialised by calling this function
 */
void CRC16_Init( void )
{
	// Initialise the CRC to 0xFFFF for the CCITT specification
	CRC16_High = 0xFF;
	CRC16_Low = 0xFF;
}

/*
 * Process 4 bits of the message to update the CRC Value.
 *
 * Note that the data must be in the low nibble of val.
 */
void CRC16_Update4Bits( unsigned char val )
{
	unsigned char	t = 0;

	// Step one, extract the Most significant 4 bits of the CRC register
	t = CRC16_High >> 4;

	// XOR in the Message Data into the extracted bits
	t = t ^ val;

	// Shift the CRC Register left 4 bits
	CRC16_High = (CRC16_High << 4) | (CRC16_Low >> 4);
	CRC16_Low = CRC16_Low << 4;

	// Do the table lookups and XOR the result into the CRC Tables
	CRC16_High = CRC16_High ^ CRC16_LookupHigh[t];
	CRC16_Low = CRC16_Low ^ CRC16_LookupLow[t];
}

/*
 * Process one Message Byte to update the current CRC Value
 */
void CRC16_Update( unsigned char val )
{
	CRC16_Update4Bits( val >> 4 );		// High nibble first
	CRC16_Update4Bits( val & 0x0F );	// Low nibble
}

void CRC16_Generate(unsigned char* msg, int len) {
    CRC16_Init();
    int i;
    for(i=0; i<len; i++) {
        CRC16_Update(msg[i]);
    }
}

unsigned char CRC16_GetHigh() {
    return CRC16_High;
}

unsigned char CRC16_GetLow() { // TEEHEE GET LOW!
    return CRC16_Low;
}