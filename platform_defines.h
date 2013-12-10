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

#define THERMISTOR_BETA 3892
#define THERMISTOR_RESISTANCE_25C 10000

#define THERMISTOR_RESISTANCE_0C_TOP 32690
#define THERMISTOR_RESISTANCE_0C_BOTTOM 32610

#define THERMISTOR_RESISTANCE_0C 32650

#define TOP_RESISTOR_VALUE 31600

#define DEFAULT_INTERVAL 60

#define LED1_SIGNAL PORTBbits.RB0
#define LED2_SIGNAL PORTBbits.RB6
#define LED3_SIGNAL PORTBbits.RB7

#define POT1_PIN 1
#define POT2_PIN 2
#define POT3_PIN 3

#define POT1_CHANNEL 9
#define POT2_CHANNEL 8
#define POT3_CHANNEL 10

#define XBEE_nCTS PORTCbits.RC5
#define XBEE_POWER PORTCbits.RC2
#define XBEE_SLEEP_RQ PORTCbits.RC3
#define XBEE_ON_nSLEEP PORTBbits.RB5

#define ANALOG_POWER_TOGGLE PORTAbits.RA4

#define XTAL_FREQUENCY 8000000

#define RESET_SIGNAL PORTEbits.RE3
#define TEST_SIGNAL PORTCbits.RC4

#define THERMISTOR_CHANNEL 0

#define TEST_SIGNAL_IOCN IOCCNbits.IOCCN4
#define TEST_SIGNAL_IOCF IOCCFbits.IOCCF4

#define RESET_SIGNAL_IOCN IOCENbits.IOCEN3
#define RESET_SIGNAL_IOCF IOCEFbits.IOCEF3

typedef struct {
	short probeValueAdjust[3];
	unsigned char reserved[10];
} CalibrationData;

typedef struct {
    word interval;
} IntervalData;
/////////////////////////////////////////
// KEEP ALL EEPROM MANAGEMENT DEFINES //
// HERE                              //
//////////////////////////////////////
//#define CALIBRATION_DATA_LOCATION 0x00
//#define INTERVAL_LOCATION (CALIBRATION_DATA_LOCATION+sizeof(CalibrationData))

#define PROGRAM_REVISION 1

#endif	/* PLATFORM_DEFINES_H */

