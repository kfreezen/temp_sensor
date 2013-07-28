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
short UART_Receive(long tmo);
int UART_ReceiveMsgTmo(char* msg, int len, const char end_char, long tmo);

inline void UART_ReceiveMsg(char* msg, int len, const char end_char);

#ifdef	__cplusplus
}
#endif

#endif	/* UART_H */

