/* 
 * File:   main.c
 * Author: Kent
 *
 * Created on May 7, 2013, 4:46 PM
 */

#pragma config WDTE = SWDTEN
#pragma config FOSC = HS
#pragma config PLLEN = OFF
//#pragma config MCLRE = OFF
#pragma config LVP = OFF

#pragma config PWRTE = ON // Power-on reset timer.
#pragma config BOREN = OFF
//#pragma config BORV = HI

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

//#define TEST_NO_XBEE

EEPROM_Structure eepromData;

unsigned char xbee_reset_flag = 0;

void pulseLed(int ticks) {
    LED1_SIGNAL = 1;
    timer1_poll_delay(ticks, DIVISION_8);
    LED1_SIGNAL = 0;
}

extern XBeeAddress dest_address;

extern void __doTestSendPacket();

char error;

unsigned batt_level;
long probeResistance0;

int main(int argc, char** argv) {
	if(PCONbits.STKOVF || PCONbits.STKUNF) {
		LED1_SIGNAL = 1;
		LED2_SIGNAL = 0;
		timer1_poll_delay(16384, DIVISION_1);
		LED1_SIGNAL = 0;
		LED2_SIGNAL = 1;
		timer1_poll_delay(16384, DIVISION_1);
		
	}
	/*if(PCONbits.nBOR == 0) {
		// Brown-out reset.
		// I don't know how to handle these,
		// so for now I'll just do a software reset.
		PCONbits.nBOR = 1;
		asm("reset");
	}*/

	// Initialize
	PIE1 = 0;
	PIE2 = 0;
	INTCON = 0;

	T1CON = 0;
	T1GCON = 0;
	
	TRISA = TRISA_MASK;
	TRISB = TRISB_MASK;
	TRISC = TRISC_MASK;

	// These are 0 because the ADC_Enable will enable as necessary.
	ANSELA = 0;
	ANSELB = 0;

	WPUA = WPUA_MASK;
	WPUB = WPUB_MASK;
	WPUC = WPUC_MASK;
	
	// The ports are in an unknown state at startup, so we need to
	// set them to a known state.
	PORTA = 0;
	PORTB = 0;
	PORTC = 0;

	LATA = 0;
	LATB = 0;
	LATC = 0;

	// Set up low power sleep mode.
	VREGCONbits.VREGPM = 1;
	
	//if(REVID == 0) {
	//	LED1_SIGNAL = 1;
	//}
	Timer1_Init();
	
	// Wait for the oscillator to stabilize.
	while(!OSCSTATbits.OSTS) {}

	unsigned vdd;
	// Here we need to wait till VDD is within 3% of 3.3V
	ADC_EnableEx(VDD_PVREF);
	while(1) {
		vdd = DetectVdd();
		int length;
		if(vdd < 2000) {
			length = 1;
		} else if(vdd < 2500) {
			length = 2000;
		} else if(vdd < 3000) {
			length = 64000;
		} else {
			length = 2;
		}

		if(vdd < 3350 && vdd > 3250) {
			break;
		} else {
			LED1_SIGNAL = 1;
			LED2_SIGNAL = 1;
			LED3_SIGNAL = 1;
			timer1_poll_delay(length/2, DIVISION_1);
			LED1_SIGNAL = 0;
			LED2_SIGNAL = 0;
			LED3_SIGNAL = 0;
			timer1_poll_delay(length/2, DIVISION_1);
		}
	}
	ADC_Disable();
	
	timer1_poll_delay(1, DIVISION_8);
	pulseLed(80);

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

	LED2_SIGNAL = 0;
	
	
	
#ifndef TEST_NO_XBEE
    XBee_Enable(9600);
	XBAPI_Wait(API_MODEM_STATUS);
#endif

    //LED1_SIGNAL = 1;
    //LED2_SIGNAL = 1;

	/* So basically, the way this will work is
	 * we will register a XBee packet (a command in this case)
	 * in the packet queue.  The interrupt handler will handle
	 * the response and put the relevant information in the response's
	 * corresponding info struct.
	 */
	
	//XBee_SwitchBaud(XBEE_BAUD);

	LED3_SIGNAL = 1;
#ifndef TEST_NO_XBEE
    XBAPI_Command(CMD_ATSM, 1L, TRUE);
	int status = XBAPI_Wait(API_AT_CMD_RESPONSE);
	
	if(status != 0) {
		LED2_SIGNAL = 0;
		LED1_SIGNAL = 1;
		
        //XBee_Disable();
        while(1){
			timer1_poll_delay(16384, DIVISION_1);
			LED1_SIGNAL = 0;
			LED3_SIGNAL = 1;
			timer1_poll_delay(16384, DIVISION_1);
			LED3_SIGNAL = 0;
			LED1_SIGNAL = 1;
        }
	}

	XBAPI_Command(CMD_ATPD, 0L, TRUE);
	status = XBAPI_Wait(API_AT_CMD_RESPONSE);
	
    XBAPI_Command(CMD_ATAC, 1L, FALSE);
	status = XBAPI_Wait(API_AT_CMD_RESPONSE);
	
    if(status != 0) {
		LED2_SIGNAL = 0;
        XBee_Disable();
        while(1){
			LED1_SIGNAL = 1;
			timer1_poll_delay(16000, DIVISION_1);
			LED1_SIGNAL = 0;
			timer1_poll_delay(16000, DIVISION_1);
        }
    }
#endif
	
    // Initialize crc16 code.
    CRC16_Init();
    
	LED3_SIGNAL = 1;

	LED2_SIGNAL = 0;
	
	if(TEST_SIGNAL) {
#ifndef TEST_NO_XBEE
		XBee_Sleep();
#endif

		testMain();

		// Do clean-up and shut off stuff here.
#ifndef TEST_NO_XBEE
		XBee_Sleep();
		XBee_Disable();
#endif
		while(1) {
			sleep(2);
		}
	}

#ifndef TEST_NO_XBEE
    // Send receiver address broadcast request
    SendReceiverBroadcastRequest();
    //XBAPI_HandleFrame(NULL, API_RX_INDICATOR);
#endif
	
	LED3_SIGNAL = 0;
    LED2_SIGNAL = 0;
    LED1_SIGNAL = 0;

	int battlevel_itr = 0, i=0;
    // Core logic

    while(1) {
        asm("clrwdt");
        
        LED1_SIGNAL = 1;
		ADC_Enable();
		
		for(i=1; i < 2; i++) {
			ADC_EnablePin(PROBE_PORT(i), PROBE_PIN(i));
		}
		
		// Give the external cap time to charge.
		// The external cap is around 0.047uf, and according to my calculations
		// should be sufficiently charged in whatever 120 ticks is, I guess?
		unsigned short tmr1_start = TMR1;
		
		//XBAPI_Command(CMD_ATSM, 1L, TRUE);
		//XBAPI_Wait(API_AT_CMD_RESPONSE);

		timer1_poll_delay(120-(TMR1 - tmr1_start), DIVISION_1);

		// Gather data here so that the xbee isn't waiting on it.

		long probeResistances[NUM_PROBES];
		memset(probeResistances, 0, sizeof(long)*NUM_PROBES);
		for(i = 1; i < 2; i++) {
			probeResistances[i] = GetProbeResistance(i);
		}
		
		// TODO:  Here, we read battery level.

		battlevel_itr ++;
		long battLevel = 0;
		
		if(battlevel_itr >= 1440) {
			unsigned vdd = DetectVdd();

			timer1_poll_delay(1590, DIVISION_1);

			battLevel = ADC_Read(BATTLEVEL_CHANNEL);

			battLevel = (battLevel * (long) vdd / 4096L) << 8;
			battLevel /= (BATT_LOWER_KOHMS<<8) / (BATT_LOWER_KOHMS + BATT_UPPER_KOHMS);

			ADC_DisablePin(BATTLEVEL_PORTSEL, BATTLEVEL_PIN);

			battlevel_itr = 0;

			FVRCONbits.FVREN = 0;
		}

#ifndef TEST_NO_XBEE
        XBee_Wake();
        SendReport(probeResistances, battLevel, THERMISTOR_RESISTANCE_25C, THERMISTOR_BETA, TOP_RESISTOR_VALUE);
		// Let's just delay 207 ms for this.
		//timer1_poll_delay(6782, DIVISION_1);
		
		XBee_Sleep();
		long sleeptmo = 1000000;
		while(XBEE_ON_nSLEEP && sleeptmo --) {

		}

		// If it timed out, reset sleep mode.
		if(sleeptmo < 1) {
			XBAPI_Command(CMD_ATSM, 1L, TRUE);
			XBAPI_Wait(API_AT_CMD_RESPONSE);
		}
#endif
		
		for(i = 1; i < 2; i++) {
			ADC_DisablePin(PROBE_PORT(i), PROBE_PIN(i));
		}
		
		ADC_Disable();
		
        LED1_SIGNAL = 0;

		// Fill out the last cycle.
		asm("sleep");

        sleep(58);
    }
}