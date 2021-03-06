#include "test.h"
#include "platform_defines.h"
#include "uart.h"
#include "timer.h"
#include "xbee.h"
#include "adc.h"
#include "eeprom.h"

#include <pic16f1789.h>
#include <stddef.h>

#define MAX_TASKS 4

extern EEPROM_Structure eepromData;

typedef void* (*TaskFunc)(void*);

byte curTask = 0;
byte taskBits = 0; // This allows up to 8 tasks.
TaskFunc tasks[MAX_TASKS];
void* savedParams[MAX_TASKS];

void tasking_init();

byte tasking_initTask(TaskFunc start, void* param) {
	byte taskItr = 0;
	byte taskMask = 1;
	for(; taskItr < MAX_TASKS; taskItr++) {
		if(taskBits & taskMask) {
		} else {
			taskBits |= taskMask;
			tasks[taskItr] = start;
			savedParams[taskItr] = param;
			return taskItr;
		}

		// PIC16's shift instructions only shift by one,
		// so it's more efficient to do this when possible.
		taskMask <<= 1;
	}
	return MAX_TASKS;
}

void tasking_scheduler() {
	while(1) {
		if(++curTask >= MAX_TASKS) {
			curTask = 0;
		}

		byte taskMask = 1<<curTask;
		
		if(taskBits & taskMask) {
			asm("clrwdt");
			savedParams[curTask] = tasks[curTask](savedParams[curTask]);
		}

	}
}

/*void tasking_yield(void* paramToSave) {
	savedParams[curTask] = paramToSave;
	
	if(++curTask >= MAX_TASKS) {
		curTask = 0;
	}

	byte taskMask = 1 << curTask
	if(taskBits & taskMask) {
		tasks[curTask](savedParams[curTask]);
	}
}*/

unsigned char timer1_flag;
unsigned char timer1_overflow = 0;
void interrupt isr() {
	extern unsigned char UART_Buffer[];
	extern unsigned char UART_BufferItr;

	if (PIR1bits.TMR1IF) {
		PIR1bits.TMR1IF = 0;
		timer1_flag = 1;
		timer1_overflow++;

		// clrwdt in the isr requires everything to time out.
		asm("clrwdt");
		
	} else if (PIR1bits.RCIF) {
		UART_HandleInterrupt();
		PIR1bits.RCIF = 0;

		char valid = XBAPI_HandleFrameIfValid((Frame*) & UART_Buffer, UART_BufferItr);
		if (valid == 0) {
			UART_ClearBuffer();
		}
	} else if(INTCONbits.IOCIF) {

		if((RESET_SIGNAL == 0 && TEST_SIGNAL == 1) || TEST_SIGNAL_IOCF) {
			// either RESET btn being set, or TEST_SIGNAL going low
			// constitute a reset.

			// save stuff and reset.
			EEPROM_Write(0, (byte*)&eepromData, sizeof(EEPROM_Structure));
			TEST_SIGNAL_IOCF = 0;
			RESET_SIGNAL_IOCF = 0;
			asm("reset");
		}

		INTCONbits.IOCIF = 0;
	} else {
		LED1_SIGNAL = 1;
		timer1_poll_delay(16384, DIVISION_1);
		LED1_SIGNAL = 0;
		timer1_poll_delay(16384, DIVISION_1);
		//Error here
	}
	
}

unsigned char mainTaskDone = 0;
void* MainTask(void* param) {
	if(param == 0) {
		XBee_Wake();
		UART_Init(9600);
		SendReceiverBroadcastRequest();

		/*DAC_Enable();

		// Enable CCP1 in PWM mode.
		TRISBbits.TRISB0 = 1; // Enable output driver.
		APFCONbits.CCP1SEL = 1; // Set to RB0
		CCP1CONbits.CCP1M = 0xC; // Set to PWM mode.

		PIE1bits.TMR2IE = 0;

		CCP1CONbits.DC1B = 3; // 511 & 3 = 3
		CCPR1L = 127;
		PIR1bits.TMR2IF = 0;
		T2CONbits.T2CKPS = 0; // Timer 2 prescaler 1:1.
		T2CONbits.TMR2ON = 1;

		TRISBbits.TRISB0 = 1; // Put it to input.*/

		LED1_SIGNAL = 0;
		LED2_SIGNAL = 0;
		LED3_SIGNAL = 0;
		
		mainTaskDone = 1;
		return (void*) 1;
	} else {
		return (void*) 2;
	}
}

extern unsigned long userData;
void* RangeTestTask(void* param) {
	if(mainTaskDone == 0) {
		return (void*) 0;
	}

	// Do range test.
	if(SendRangeTest() == 1) {
		XBAPI_Command(CMD_ATDB, 0, FALSE);
		XBAPI_WaitTmo(API_AT_CMD_RESPONSE, 32768);
		unsigned toWrite = userData;
		// 0 to -255dBm
		// 0 to -40 is very good signal.
		// -41 to -60 is good signal
		// -61 to -70 is ok signal.
		// -71 to -80 is bad signal.
		// -81 to -90 is very bad
		// -91 to -101 is super uber bad.
		// -40 is the upper bound for 3.3V (DAC_Write(255))
		// -101 should be 0V
		if(toWrite < 40) {
			toWrite = 0;
		} else {
			toWrite -= 40;
		}
		toWrite = 255 * toWrite;
		toWrite /= 61;
		// Now we have properly scaled value.
		// invert it.
		toWrite = ~toWrite;
		
		toWrite >>= 5;
		LED1_SIGNAL = toWrite & 1;
		LED2_SIGNAL = (toWrite & 2) >> 1;
		LED3_SIGNAL = (toWrite & 4) >> 2;
	} else {
		LED1_SIGNAL = 0;
		LED2_SIGNAL = 0;
		LED3_SIGNAL = 0;
	}
}

inline void SET_LED(byte i, byte n) {
	switch(i) {
		case 0: LED1_SIGNAL = n; break;
		case 1: LED2_SIGNAL = n; break;
		case 2: LED3_SIGNAL = n; break;
	}
}

long probeValues[3];

void* GatherCalibrationDataTask(void* param) {

	ADC_Enable();

	ADC_EnablePin(POT1_PORT, POT1_PIN);
	ADC_EnablePin(POT2_PORT, POT2_PIN);
	ADC_EnablePin(POT3_PORT, POT3_PIN);

	byte i;
	for(i=0; i<3; i++) {
		ADC_EnablePin(PROBE_PORT(i), PROBE_PIN(i));
	}

	// wait about 3.75ms
	timer1_poll_delay(120, DIVISION_1);
	
	//long probeValues[3];
	byte calibrating[3];

	for(i=0; i<3; i++) {
		probeValues[i] = GetProbeResistance(i);
		if(probeValues[i] <= THERMISTOR_RESISTANCE_0C + (THERMISTOR_RESISTANCE_0C/100) && probeValues[i] >= THERMISTOR_RESISTANCE_0C - (THERMISTOR_RESISTANCE_0C/100)) {
			calibrating[i] = 1;
			eepromData.calibration.probeValueAdjust[i] = ADC_Read(POT_CHANNEL(i));
			eepromData.calibration.probeValueAdjust[i] = (eepromData.calibration.probeValueAdjust[i] >> 6) - 32;
			SET_LED(i, 1);
		} else {
			calibrating[i] = 0;
			SET_LED(i, 0);
		}
	}
	
	
	eepromData.calibration.probeValueAdjust[1] = ADC_Read(POT2_CHANNEL);
	eepromData.calibration.probeValueAdjust[2] = ADC_Read(POT3_CHANNEL);

	eepromData.calibration.probeValueAdjust[1] = (eepromData.calibration.probeValueAdjust[1] >> 6) - 32;
	eepromData.calibration.probeValueAdjust[2] = (eepromData.calibration.probeValueAdjust[2] >> 6) - 32;
	
	ADC_DisablePin(POT3_PORT, POT3_PIN);
	ADC_DisablePin(POT2_PORT, POT2_PIN);
	ADC_DisablePin(POT1_PORT, POT1_PIN);

	for(i=0; i<3; i++) {
		ADC_DisablePin(PROBE_PORT(i), PROBE_PIN(i));
	}

	timer1_poll_delay(100, DIVISION_1);

	// Now we want to go through and calculate the calibrated resistance, and
	// turn off the LEDs corresponding to the calibration pots if the resistance
	// is within 0.1% (or some other arbitrary value) of the correct value.  That effect will
	// be that the LEDs blink if they are within 1% of 0 degrees celsius, and will
	// stay on if they are within 0.1% (or some other arbitrary value between
	// 0% and 1%
	for(i=0; i < 3; i++) {
		if(calibrating[i]) {
			long calibratedValue = probeValues[i] + (probeValues[i]*eepromData.calibration.probeValueAdjust[i]) / 3200;
			if(calibratedValue >= THERMISTOR_RESISTANCE_0C_TOP || calibratedValue <= THERMISTOR_RESISTANCE_0C_BOTTOM) {
				SET_LED(i, 0);
			}
		}
	}

	ADC_Disable();
	
	return param;
}

int testMain() {
	// Enable interrupt on change, falling edge, on TEST_SIGNAL_IOCN
	TEST_SIGNAL_IOCN = 1;
	
	//tasking_initTask(GatherCalibrationDataTask, 0);
	tasking_initTask(RangeTestTask, 0);
	tasking_initTask(MainTask, 0);

	// And start it.
	tasking_scheduler();
}