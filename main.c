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

EEPROM_Structure eepromData;

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

unsigned batt_level;
long probeResistance0;

XBAPI_ReplyStruct* replyStruct;

int main(int argc, char** argv) {
	TRISA = TRISA_MASK;
	TRISB = TRISB_MASK;
	TRISC = TRISC_MASK;
	
	pulseLed(80);
	
	// These are 0 because the ADC_Enable will enable as necessary.
	ANSELA = 0;
	ANSELB = 0;

    //sleep(1);
    LED2_SIGNAL = 1;
    
	EEPROM_Read(0, (byte*)&eepromData, sizeof(EEPROM_Structure));
	if(eepromData.magic != EEPROM_DATA_MAGIC) {
		eepromData.magic = EEPROM_DATA_MAGIC;
		memset(&eepromData.calibration, 0, sizeof(CalibrationData));
		eepromData.interval.interval = DEFAULT_INTERVAL;
		memset(&eepromData.sensorId, 0xFF, sizeof(SensorId));
		EEPROM_Write(0, (byte*)&eepromData, sizeof(EEPROM_Structure));
	}

    asm("clrwdt");

	EnableInterrupts();

	INTCONbits.PEIE = 1;
	PIE1bits.RCIE = 1;

	// Enable interrupt on change for the reset button.
	RESET_SIGNAL_IOCN = 1;
	INTCONbits.IOCIE = 1;
	
    XBee_Enable(9600); // Between here and... (see fixme comment)
    //LED1_SIGNAL = 1;
    //LED2_SIGNAL = 1;

	/* So basically, the way this will work is
	 * we will register a XBee packet (a command in this case)
	 * in the packet queue.  The interrupt handler will handle
	 * the response and put the relevant information in the response's
	 * corresponding info struct.
	 */

	// FIXME:  Somewhere in here, there is a corruption.
	
    byte cmdId = XBAPI_Command(CMD_ATSM, 1L, TRUE);
	replyStruct = XBAPI_WaitForReplyTmo(cmdId, 32768);
	
	if(!replyStruct || replyStruct->status) {
		LED2_SIGNAL = 0;
		LED1_SIGNAL = 1;
		
        XBee_Disable();
        while(1){
            //LED1_SIGNAL = !LED1_SIGNAL;
        }
    }
	XBAPI_FreePacket(cmdId); // here there is corruption in eepromData ?

    cmdId = XBAPI_Command(CMD_ATAC, 1L, FALSE);
	replyStruct = XBAPI_WaitForReplyTmo(cmdId, 32768);
    if(!replyStruct || replyStruct->status) {
		LED2_SIGNAL = 0;
        XBee_Disable();
        while(1){
			LED1_SIGNAL = 1;
			timer1_poll_delay(16000, DIVISION_1);
			LED1_SIGNAL = 0;
			timer1_poll_delay(16000, DIVISION_1);
        }
    }
	XBAPI_FreePacket(cmdId);
	
    // Initialize crc16 code.
    CRC16_Init();
    
	LED3_SIGNAL = 1;

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
    //XBAPI_HandleFrame(NULL, API_RX_INDICATOR);

	LED3_SIGNAL = 0;
    LED2_SIGNAL = 0;
    LED1_SIGNAL = 0;
    
    // Core logic
    while(1) {
        asm("clrwdt");
        
        LED1_SIGNAL = 1;
		ADC_Enable();

		ADC_EnablePin(PROBE_PORT(1), PROBE_PIN(1));
		
		// Give the external cap time to charge.
		// The external cap is around 0.047uf, and according to my calculations
		// should be sufficiently charged in whatever 120 ticks is, I guess?
		timer1_poll_delay(120, DIVISION_1);

		// Gather data here so that the xbee isn't waiting on it.
        // Temporary fix to determine if this is what's causing it to freeze.
		probeResistance0 = GetProbeResistance(1);

        XBee_Wake();
        SendReport(probeResistance0, THERMISTOR_RESISTANCE_25C, THERMISTOR_BETA, TOP_RESISTOR_VALUE);
        XBee_Sleep();

		ADC_DisablePin(PROBE_PORT(1), PROBE_PIN(1));

		ADC_Disable();
		
        LED1_SIGNAL = 0;
        
        sleep(59);
    }
}