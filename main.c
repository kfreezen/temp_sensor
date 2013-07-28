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

unsigned char xbee_reset_flag = 0;

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

extern void __doTestSendPacket();

int error;
int cmd_itr = 0;

int main(int argc, char** argv) {
    TRISC = TRISC_MASK;
    TRISB = TRISB_MASK;
    TRISA = TRISA_MASK;
    
    pulseLed(5000);
    //LED1_SIGNAL = 1;
    LED2_SIGNAL = 1;
    
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
    
    error = XBAPI_Command(CMD_ATSM, 1L, 0xc0, TRUE);
    if(error) {
        XBee_Disable();
        while(1){
            //LED1_SIGNAL = !LED1_SIGNAL;
        }
    }

    cmd_itr++;

    error = XBAPI_Command(CMD_ATAC, 0, 0xc1, FALSE);
    if(error) {
        XBee_Disable();
        while(1){
            //LED1_SIGNAL = !LED1_SIGNAL;
        }
    }

    cmd_itr++;
    
    ADC_Enable(SEL_PORTB, 1);
    
    // Initialize crc16 code.
    CRC16_Init();

    //LED2_SIGNAL = 1;
    
    // Send receiver address broadcast request
    SendReceiverBroadcastRequest();
    XBAPI_HandleFrame(API_RX_INDICATOR, FALSE);

    LED2_SIGNAL = 0;
    
    // Core logic
    while(1) {
        LED1_SIGNAL = 1;
        // Gather data here so that the xbee isn't waiting on it.
        long thermistorResistance = (TOP_RESISTOR_VALUE * 1000L) / ((4096000L/ADC_Read(THERMISTOR_CHANNEL))-1000);

        XBee_Wake(); // It's freezing here anyway so...
        //sleep(1); // This shouldn't be here, figure out whether the xbee keeps having the same problem with this here.
        SendReport(thermistorResistance, THERMISTOR_RESISTANCE_25C, THERMISTOR_BETA, TOP_RESISTOR_VALUE);
        XBee_Sleep();
        LED1_SIGNAL = 0;
        
        sleep(59);
    }
}