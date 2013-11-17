/* 
 * File:   uart.h
 * Author: od
 *
 * Created on June 12, 2013, 11:18 AM
 */

#ifndef UART_H
#define	UART_H

#ifdef	__cplusplus
extern "C" {
#endif

void UART_Init(int baud);

void UART_Transmit(volatile char c);

void UART_TransmitMsg(volatile const unsigned char* msg, int len, const char end_char);
unsigned char UART_Receive();
int UART_ReceiveMsg(char* msg, int len, const char end_char);

void UART_HandleInterrupt();
void UART_ClearBuffer();

#ifdef	__cplusplus
}
#endif

#endif	/* UART_H */

