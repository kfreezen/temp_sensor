#include "uart.h"
#include "timer.h"
#include "platform_defines.h"

#include <pic16lf1783.h>

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

short UART_Receive(long tmo) {
    while(!OSCSTATbits.OSTS) {} // Wait for external oscillator to start running.
    long i=0;
    //asm("clrwdt"); // Probably not necessary to do this.
    while(!PIR1bits.RCIF) {i++; if(tmo && i>=tmo) {break;} }
    //asm("clrwdt");
    return (i<tmo || !tmo) ? RC1REG : 0x100;
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

int UART_ReceiveMsgTmo(char* msg, int len, char end_char, long tmo) {
    int i;

    // Ensure that the sensor doesn't freeze for some stupid non-reply
    //SWDTEN = 1; // FIXME Using a timeout would really be better than this.
    for(i=0; i<len; i++) {
        short c;
        c = UART_Receive(tmo);
        if(c!=UART_TIMEOUT) {
            msg[i] = c;
        } else {
            return i;
        }
        
        if(msg[i] == end_char && end_char) {
            break;
        }
    }
    //asm("clrwdt");
    SWDTEN = 0;
}

inline void UART_ReceiveMsg(char* msg, int len, const char end_char) {
    UART_ReceiveMsgTmo(msg, len, end_char, 0);
}