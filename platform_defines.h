/* 
 * File:   platform_defines.h
 * Author: od
 *
 * Created on June 12, 2013, 3:52 PM
 */

#ifndef PLATFORM_DEFINES_H
#define	PLATFORM_DEFINES_H

#include "globaldef.h"

#define CURRENT_COMMANDS_REVISION 0

#define TRISC_MASK 0x80 | (0x14);
#define TRISB_MASK 0x0
#define TRISA_MASK 0x0

#define FOSC_DIV_8 1

#define THERMISTOR_BETA 3948
#define THERMISTOR_RESISTANCE_25C 5000
#define TOP_RESISTOR_VALUE 16200

#define DEFAULT_INTERVAL 60

#define LED1_SIGNAL PORTCbits.RC3
#define LED2_SIGNAL PORTBbits.RB0

#define XBEE_nCTS PORTCbits.RC4

#define XTAL_FREQUENCY 8000000

#define THERMISTOR_CHANNEL 10

#define CALIBRATION_DATA_MAGIC 0xF00D

typedef struct {
    unsigned short magic;
    short thermistorValue25CAdjust;
    short topResistorValueAdjust;
} CalibrationData;

#define INTERVAL_MAGIC 0xFACE
typedef struct {
    word magic;
    word interval;
} IntervalData;
/////////////////////////////////////////
// KEEP ALL EEPROM MANAGEMENT DEFINES //
// HERE                              //
//////////////////////////////////////
#define CALIBRATION_DATA_LOCATION 0x00
#define INTERVAL_LOCATION (CALIBRATION_DATA_LOCATION+sizeof(CalibrationData))

#define PROGRAM_REVISION 0

#endif	/* PLATFORM_DEFINES_H */

