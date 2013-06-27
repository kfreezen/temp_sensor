#include "uart.h"
#include "timer.h"
#include "platform_defines.h"

#include <pic16lf1783.h>

void UART_Init(int baud) {
    TX1STAbits.SYNC = 0;
    RC1STAbits.SPEN = 1;
    RC1STAbits.CREN = 1;
    TX1STAbits.TXEN = 1;

    int spbrg = (XTAL_FREQUENCY/baud) / 64 - 1;
    SPBRG = spbrg;
}

void UART_Transmit(volatile char c) {

    if(c==0x16) {
        LED1_SIGNAL = 0;
        while(1) {}
    }
    
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

short UART_Receive(long tmo) {
    long i=0;
    while(!PIR1bits.RCIF) {i++; if(tmo && i>=tmo) {break;} }
    return (i<tmo || !tmo) ? RC1REG : 0x100;
}

void UART_ReceiveMsgTmo(char* msg, int len, char end_char, long tmo) {
    int i;

    for(i=0; i<len; i++) {
        msg[i] = UART_Receive(tmo);
        
        if(msg[i] == end_char && end_char) {
            break;
        }
    }
}

inline void UART_ReceiveMsg(char* msg, int len, const char end_char) {
    UART_ReceiveMsgTmo(msg, len, end_char, 0);
}