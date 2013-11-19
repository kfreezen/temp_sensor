#include "uart.h"
#include "timer.h"
#include "platform_defines.h"

#include <pic16f1788.h>
#include <string.h>

void UART_Init(int baud) {
    TX1STAbits.SYNC = 0;
    RC1STAbits.SPEN = 1;
    RC1STAbits.CREN = 1;
    TX1STAbits.TXEN = 1;

    int spbrg = ((XTAL_FREQUENCY/baud)>>6) - 1; // >>6 == /64
    SPBRG = spbrg;
}

void UART_Transmit(volatile char c) {
    while(!OSCSTATbits.OSTS) {} // Wait for the external oscillator to turn on.
    
    while(!TXSTAbits.TRMT) {}

    TX1REG = c;
}

// Something is corrupting the value of the "junk" variable

void UART_TransmitMsg(volatile const byte* _msg, int len, const char end_char) {
    //LED1_SIGNAL = 1;
    int i;
    for(i=0; i<len; i++) {
        UART_Transmit(_msg[i]);

        if(end_char && _msg[i] == end_char) {
            break;
        }
    }

    while(!TX1STAbits.TRMT) {}
}

#define UART_TIMEOUT 0x100

byte UART_Receive() {
	while(!OSCSTATbits.OSTS) {}

	asm("clrwdt");
	while(!PIR1bits.RCIF) {}
	asm("clrwdt");

	return RC1REG;
}

#define UART_BUFFER_LEN 128
unsigned char UART_Buffer[UART_BUFFER_LEN];
unsigned char UART_BufferItr = 0;

void UART_HandleInterrupt() {
	if(UART_BufferItr + 1 > UART_BUFFER_LEN) {
		return;
	}
	
	UART_Buffer[UART_BufferItr++] = UART_Receive();
}

void UART_ClearBuffer() {
	memset(UART_Buffer, 0, UART_BUFFER_LEN);
	UART_BufferItr = 0;
}

/*int UART_ReceiveMsgTmo(char* msg, int len, char end_char, long tmo) {
    for(; len!=0; len--) {
        short c;
        c = UART_Receive(tmo);
        if(c!=UART_TIMEOUT) {
            *msg++ = c;
        } else {
            break;
        }

        if(*msg == end_char && end_char) {
            break;
        }
    }

    return len;
}*/

int UART_ReceiveMsg(char* msg, int len, char end_char) {
    int i;

    // Ensure that the sensor doesn't freeze for some stupid non-reply
    for(i=0; i<len; i++) {
        char c;
        c = UART_Receive();
		msg[i] = c;
        
        if(msg[i] == end_char && end_char) {
            break;
        }
    }

	return i;
}