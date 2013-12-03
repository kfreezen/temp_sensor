#include "test.h"
#include "platform_defines.h"
#include "uart.h"
#include "timer.h"
#include "xbee.h"
#include "adc.h"
#include "eeprom.h"

#include <pic16f1788.h>

#define MAX_TASKS 4

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
			savedParams[curTask] = tasks[curTask](savedParams[curTask]);
		}

	}
}

/*void tasking_yield(void* paramToSave) {
	savedParams[curTask] = paramToSave;
	
	if(++curTask >= MAX_TASKS) {
		curTask = 0;
	}

	byte taskMask = 1 << curTask;main.c:121: error: too few function arguments
	if(taskBits & taskMask) {
		tasks[curTask](savedParams[curTask]);
	}
}*/

void interrupt isr() {
	extern unsigned char UART_Buffer[];
	extern unsigned char UART_BufferItr;

	extern CalibrationData calibrationData;
	
	if(PIR1bits.TMR1IF == 1) {
		PIR1bits.TMR1IF = 0;
	}

	if(PIR1bits.RCIF==1) {
		UART_HandleInterrupt();
		PIR1bits.RCIF = 0;

		char valid = XBAPI_HandleFrameIfValid((Frame*) &UART_Buffer, 0, UART_BufferItr);
		if(valid == 0) {
			UART_ClearBuffer();
		}
	}

	if(INTCONbits.IOCIF == 1) {
		if(RESET_SIGNAL == 0 && TEST_SIGNAL == 1) {
			// save stuff and reset.
			EEPROM_Write(CALIBRATION_DATA_LOCATION, (byte*)&calibrationData, sizeof(CalibrationData));
			TEST_SIGNAL_IOCF = 0;
			RESET_SIGNAL_IOCF = 0;
			asm("reset");
		}

		INTCONbits.IOCIF = 0;
	}
}

void* MainTask(void* param) {
	if(param == 0) {
		XBee_Wake();
		UART_Init(9600);
		SendReceiverBroadcastRequest();
		return 1;
	} else {
		return 2;
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
	extern CalibrationData calibrationData;

	ADC_Enable();

	ADC_EnablePin(SEL_PORTB, POT1_PIN);
	ADC_EnablePin(SEL_PORTB, POT2_PIN);
	ADC_EnablePin(SEL_PORTB, POT3_PIN);

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
			calibrationData.probeValueAdjust[i] = ADC_Read(POT_CHANNEL(i));
			calibrationData.probeValueAdjust[i] = (calibrationData.probeValueAdjust[i] >> 6) - 32;
			SET_LED(i, 1);
		} else {
			calibrating[i] = 0;
			SET_LED(i, 0);
		}
	}
	
	
	/*calibrationData.probeValueAdjust[1] = ADC_Read(POT2_CHANNEL);
	calibrationData.probeValueAdjust[2] = ADC_Read(POT3_CHANNEL);

	calibrationData.probeValueAdjust[1] = (calibrationData.probeValueAdjust[1] >> 6) - 32;
	calibrationData.probeValueAdjust[2] = (calibrationData.probeValueAdjust[2] >> 6) - 32;
	*/
	
	ADC_DisablePin(SEL_PORTB, POT3_PIN);
	ADC_DisablePin(SEL_PORTB, POT2_PIN);
	ADC_DisablePin(SEL_PORTB, POT1_PIN);

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
			long calibratedValue = probeValues[i] + (probeValues[i]*calibrationData.probeValueAdjust[i]) / 3200;
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
	
	tasking_initTask(GatherCalibrationDataTask, 0);
	tasking_initTask(MainTask, 0);

	// And start it.
	tasking_scheduler();
}