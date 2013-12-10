/* 
 * File:   eeprom.h
 * Author: od
 *
 * Created on June 14, 2013, 2:19 PM
 */

#ifndef EEPROM_H
#define	EEPROM_H

#include "globaldef.h"
#include "platform_defines.h"
#include "packets.h"

void EEPROM_Read(byte address, byte* data, int length);
void EEPROM_Write(byte address, byte* data, int length);

typedef struct EEPROM_Structure {
	uint16 magic;
	CalibrationData calibration;
	IntervalData interval;
	SensorId sensorId;
} EEPROM_Structure;

#define EEPROM_DATA_MAGIC 0x1234
#endif	/* EEPROM_H */

