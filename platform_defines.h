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

#define TRISC_MASK 0xB0
#define TRISB_MASK 0x20
#define TRISA_MASK 0x00

#define FOSC_DIV_8 1

#define THERMISTOR_BETA 3948
#define THERMISTOR_RESISTANCE_25C 10000
#define TOP_RESISTOR_VALUE 31600

#define DEFAULT_INTERVAL 60

#define LED1_SIGNAL PORTBbits.RB0
#define LED2_SIGNAL PORTBbits.RB6
#define LED3_SIGNAL PORTBbits.RB7

#define XBEE_nCTS PORTCbits.RC5
#define XBEE_POWER PORTCbits.RC2
#define XBEE_SLEEP_RQ PORTCbits.RC3
#define XBEE_ON_nSLEEP PORTBbits.RB5

#define ANALOG_POWER_TOGGLE PORTAbits.RA4

#define XTAL_FREQUENCY 8000000

#define TEST_SIGNAL PORTCbits.RC4

#define THERMISTOR_CHANNEL 0

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

#define PROGRAM_REVISION 1

#endif	/* PLATFORM_DEFINES_H */

