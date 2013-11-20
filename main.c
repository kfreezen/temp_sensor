/* 
 * File:   main.c
 * Author: Kent
 *
 * Created on May 7, 2013, 4:46 PM
 */

#pragma config WDTE = SWDTEN
#pragma config FOSC = HS
#pragma config PLLEN = 0
#pragma config MCLRE = OFF
#pragma config LVP = OFF

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pic16f1788.h>

#include "timer.h"
#include "xbee.h"
#include "uart.h"
#include "packets.h"
#include "platform_defines.h"
#include "adc.h"
#include "eeprom.h"
#include "crc16.h"
#include "test.h"
#include "interrupts.h"
#include "asm.h"

CalibrationData calibrationData;
IntervalData intervalData;

unsigned char xbee_reset_flag = 0;

void pulseLed(int ticks) {
    LED1_SIGNAL = 1;
    timer1_poll_delay(ticks, DIVISION_8);
    LED1_SIGNAL = 0;
}

int GetOhms(int adc_value) {
    return 16200000/((4096000/adc_value)-1000);
}

extern XBeeAddress dest_address;

extern void __doTestSendPacket();

char error;

int main(int argc, char** argv) {
	TRISA = TRISA_MASK;
	TRISB = TRISB_MASK;
	TRISC = TRISC_MASK;
	
	// These are 0 because the ADC_Enable will enable as necessary.
	ANSELA = 0;
	ANSELB = 0;
	
    //sleep(1);
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

    asm("clrwdt");

	EnableInterrupts();

	INTCONbits.PEIE = 1;
	PIE1bits.RCIE = 1;

    XBee_Enable(9600);

    //LED1_SIGNAL = 1;
    //LED2_SIGNAL = 1;

	/* So basically, the way this will work is
	 * we will register a XBee packet (a command in this case)
	 * in the packet queue.  The interrupt handler will handle
	 * the response and put the relevant information in the response's
	 * corresponding info struct.
	 */

    byte cmdId = XBAPI_Command(CMD_ATSM, 1L, TRUE);
	XBAPI_ReplyStruct* replyStruct = XBAPI_WaitForReplyTmo(cmdId, 32768);
	
	if(replyStruct->status) {
		LED2_SIGNAL = 0;
		LED1_SIGNAL = 1;
		
        XBee_Disable();
        while(1){
            //LED1_SIGNAL = !LED1_SIGNAL;
        }
    }
	XBAPI_FreePacket(cmdId);

    cmdId = XBAPI_Command(CMD_ATAC, 0, FALSE);
    if(replyStruct->status) {
		LED2_SIGNAL = 0;
		LED3_SIGNAL = 1;
        XBee_Disable();
        while(1){
            //LED1_SIGNAL = !LED1_SIGNAL;
        }
    }
	XBAPI_FreePacket(cmdId);
	
    // Initialize crc16 code.
    CRC16_Init();
    
	LED3_SIGNAL = 1;
	timer1_poll_delay(75, DIVISION_8);
	LED3_SIGNAL = 0;

	LED2_SIGNAL = 0;
	
	if(TEST_SIGNAL) {
		XBee_Sleep();
		
		testMain();

		// Do clean-up and shut off stuff here.
		XBee_Sleep();
		XBee_Disable();
		while(1) {
			sleep(1);
		}
	}

    // Send receiver address broadcast request
    SendReceiverBroadcastRequest();
    XBAPI_HandleFrame(NULL, API_RX_INDICATOR);

    LED2_SIGNAL = 0;
    LED1_SIGNAL = 0;
    
    // Core logic
    while(1) {
        asm("clrwdt");
        
        LED1_SIGNAL = 1;
		ADC_Enable();
		
		ADC_EnablePin(PROBE_PORT(0), PROBE_PIN(0));
		
		// Gather data here so that the xbee isn't waiting on it.
        // Temporary fix to determine if this is what's causing it to freeze.
        long thermistorResistance = (TOP_RESISTOR_VALUE * 1000L) / ((4096000L/ADC_Read(THERMISTOR_CHANNEL))-1000);

        XBee_Wake();
        SendReport(thermistorResistance, THERMISTOR_RESISTANCE_25C, THERMISTOR_BETA, TOP_RESISTOR_VALUE);
        XBee_Sleep();

		ADC_DisablePin(PROBE_PORT(0), PROBE_PIN(0));

		ADC_Disable();
		
        LED1_SIGNAL = 0;
        
        sleep(59);
    }
}