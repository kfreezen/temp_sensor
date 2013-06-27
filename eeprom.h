/* 
 * File:   eeprom.h
 * Author: od
 *
 * Created on June 14, 2013, 2:19 PM
 */

#ifndef EEPROM_H
#define	EEPROM_H

#include "globaldef.h"

void EEPROM_Read(byte address, byte* data, int length);
void EEPROM_Write(byte address, byte* data, int length);

#endif	/* EEPROM_H */

