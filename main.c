/* 
 * File:   main.c
 * Author: Kent
 *
 * Created on May 7, 2013, 4:46 PM
 */

#pragma config WDTE = SWDTEN
#pragma config FOSC = HS
#pragma config PLLEN = 0
#pragma config LVP = OFF

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pic16lf1783.h>

#include "timer.h"
#include "xbee.h"
#include "uart.h"
#include "packets.h"
#include "platform_defines.h"
#include "adc.h"
#include "eeprom.h"
#include "crc16.h"

CalibrationData calibrationData;
IntervalData intervalData;

void pulseLed(int ticks) {
    LED1_SIGNAL = 1;
    timer1_poll_delay(ticks, DIVISION_8);
    LED1_SIGNAL = 0;
}

void interrupt isr() {
    if(PIR1bits.TMR1IF==1) {
        PIR1bits.TMR1IF = 0;
    }

    if(PIR1bits.RCIF==1) {
        PIR1bits.RCIF = 0;
        // Do UART read.
    }
}

int GetOhms(int adc_value) {
    return 16200000/((4096000/adc_value)-1000);
}

extern XBeeAddress dest_address;

int therm_debug = 0;

extern void __doTestSendPacket();

int error;

// TODO:  Enable ADC pins
int main(int argc, char** argv) {
    TRISC = TRISC_MASK;
    TRISB = TRISB_MASK;
    TRISA = TRISA_MASK;

    unsigned long command_data = 0x1;
    
    pulseLed(5000);
    timer1_poll_delay(5000, DIVISION_8);
    
    // Load Calibration
    EEPROM_Read(CALIBRATION_DATA_LOCATION, (byte*)&calibrationData, sizeof(CalibrationData));
    if(calibrationData.magic != CALIBRATION_DATA_MAGIC) { // Calibration data is invalid, write default to EEPROM.
        memset(&calibrationData, 0, sizeof(CalibrationData));
        calibrationData.magic = CALIBRATION_DATA_MAGIC;
        EEPROM_Write(CALIBRATION_DATA_LOCATION, (byte*)&calibrationData, sizeof(CalibrationData));
    }

    // Load Interval
    EEPROM_Read(INTERVAL_LOCATION, (byte*)&intervalData, sizeof(IntervalData));
    if(intervalData.magic != INTERVAL_MAGIC) {
        intervalData.magic = INTERVAL_MAGIC;
        intervalData.interval = DEFAULT_INTERVAL;
        EEPROM_Write(INTERVAL_LOCATION, (byte*)&intervalData, sizeof(IntervalData));
    }

    XBee_Enable(9600);
    XBee_Wake();
    
    error = XBAPI_Command(CMD_ATSM, command_data, 0xc0, TRUE);
    if(error) {
        XBee_Disable();
        while(1){}
    }

    error = XBAPI_Command(CMD_ATAC, 0, 0xc1, FALSE);
    if(error) {
        XBee_Disable();
        while(1){}
    }
    
    ADC_Enable(SEL_PORTB, 1);
    
    // Initialize crc16 code.
    CRC16_Init();

    // Send receiver address broadcast request
    SendReceiverBroadcastRequest();
    XBAPI_HandleFrame(API_RX_INDICATOR); // Freezing here because it never gets any stuff.
    
    //LED2_SIGNAL = 1;

    // Core logic
    while(1) {
        // Gather data here so that the xbee isn't waiting on it.
        long thermistorResistance = (TOP_RESISTOR_VALUE * 1000L) / ((4096000L/ADC_Read(THERMISTOR_CHANNEL))-1000);
        therm_debug = thermistorResistance;
        
        XBee_Wake();
        //sleep(1); // FIXME: This is an extremely bad way to do this.  Figure out a
        //timer1_poll_delay_ms(100);
        SendReport(thermistorResistance, THERMISTOR_RESISTANCE_25C, THERMISTOR_BETA, TOP_RESISTOR_VALUE);
        XBee_Sleep();

        sleep(59);
    }
}