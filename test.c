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
		if(RESET_SIGNAL == 0 || (TEST_SIGNAL_IOCN == 1 && TEST_SIGNAL_IOCF == 1)) {
			// save stuff and reset.
			EEPROM_Write(CALIBRATION_DATA_LOCATION, (byte*)&calibrationData, sizeof(CalibrationData));
			TEST_SIGNAL_IOCF = 0;
			RESET_SIGNAL_IOCF = 0;
			asm("reset");
		}

		INTCONbits.IOCIF = 0;
	}
}

/*void* XBee_Task(void* param) {
	extern unsigned char UART_Buffer[];
	extern unsigned char UART_BufferItr;
	
	if(UART_Buffer[0] == API_START_DELIM && UART_BufferItr >= 3) {
		// Now, let's see if it's been filled out to the necessary limit.
		Frame* frame = (Frame*) UART_Buffer;
		int length = (frame->rx.length[0] << 8) | frame->rx.length[1];
		// UART_BufferItr needs to be length + 4 (start_delim + length + checksum)
		if(UART_BufferItr == length + 4) {
			XBAPI_HandleFrame(frame, 0);
			UART_ClearBuffer();
			LED3_SIGNAL = 1;
			timer1_poll_delay(750, DIVISION_8);
			LED3_SIGNAL = 0;
		}
	}

	return param;
}*/

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

void* GatherCalibrationDataTask(void* param) {
	extern CalibrationData calibrationData;

	ADC_Enable();

	ADC_EnablePin(SEL_PORTB, POT1_PIN);
	ADC_EnablePin(SEL_PORTB, POT2_PIN);
	ADC_EnablePin(SEL_PORTB, POT3_PIN);

	calibrationData.probe0ValueAdjust = ADC_Read(POT1_CHANNEL);
	calibrationData.probe1ValueAdjust = ADC_Read(POT2_CHANNEL);
	calibrationData.probe2ValueAdjust = ADC_Read(POT3_CHANNEL);

	calibrationData.probe0ValueAdjust = (calibrationData.probe0ValueAdjust >> 4) - 128;
	calibrationData.probe1ValueAdjust = (calibrationData.probe1ValueAdjust >> 4) - 128;
	calibrationData.probe2ValueAdjust = (calibrationData.probe2ValueAdjust >> 4) - 128;

	ADC_DisablePin(SEL_PORTB, POT3_PIN);
	ADC_DisablePin(SEL_PORTB, POT2_PIN);
	ADC_DisablePin(SEL_PORTB, POT1_PIN);

	if(calibrationData.probe0ValueAdjust < 16 && calibrationData.probe0ValueAdjust > -16) {
		LED1_SIGNAL = 1;
	} else {
		LED1_SIGNAL = 0;
	}

	if(calibrationData.probe1ValueAdjust < 16 && calibrationData.probe1ValueAdjust > -16) {
		LED2_SIGNAL = 1;
	} else {
		LED2_SIGNAL = 0;
	}

	if(calibrationData.probe2ValueAdjust < 16 && calibrationData.probe2ValueAdjust > -16) {
		LED3_SIGNAL = 1;
	} else {
		LED3_SIGNAL = 0;
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