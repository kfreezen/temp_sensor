#include "test.h"
#include "platform_defines.h"
#include "uart.h"
#include "timer.h"
#include "xbee.h"

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
	if(PIR1bits.RCIF==1) {
		UART_HandleInterrupt();
		PIR1bits.RCIF = 0;
	}
}

void* XBee_Task(void* param) {
	extern unsigned char UART_Buffer[];
	extern unsigned char UART_BufferItr;

	if(UART_Buffer[0] == API_START_DELIM && UART_BufferItr >= 3) {
		// Now, let's see if it's been filled out to the necessary limit.
		Frame* frame = (Frame*) UART_Buffer;
		int length = (frame->rx.length[0] << 8) | frame->rx.length[1];
		// UART_BufferItr needs to be length + 4 (start_delim + length + checksum)
		if(UART_BufferItr == length + 4) {
			XBAPI_HandleFrame(0, 0);
			UART_ClearBuffer();
			//LED3_SIGNAL = 1;
			timer1_poll_delay(750, DIVISION_8);
			LED3_SIGNAL = 0;
		}
	}

	return param;
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
inline void EnableInterrupts() {
	INTCONbits.GIE = 1;
}

int testMain() {
	EnableInterrupts();

	INTCONbits.PEIE = 1;
	PIE1bits.RCIE = 1;

	tasking_initTask(XBee_Task, 0);
	tasking_initTask(MainTask, 0);

	// And start it.
	tasking_scheduler();
}