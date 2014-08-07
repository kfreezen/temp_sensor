/* 
 * File:   timer.h
 * Author: Kent
 *
 * Created on May 11, 2013, 2:33 PM
 */

#ifndef TIMER_H
#define	TIMER_H

#ifdef	__cplusplus
extern "C" {
#endif

// Timer1 Prescaler stuff
#define DIVISION_1 0
#define DIVISION_2 1
#define DIVISION_4 2
#define DIVISION_8 3

#define TMR0_DIV_2 1
#define TMR0_DIV_4 2
#define TMR0_DIV_8 3
	
// Timer1 clock source select
#define INST_CLOCK 0
#define SYS_CLOCK 1
#define TMR1_PINOSC 2
#define LFINTOSC 3
    
// Userdefined stuff for sleep
#define SLEEP_8MHZ 0
#define SLEEP_4MHZ 1
#define SLEEP_2MHZ 2
#define SLEEP_1MHZ 3
#define SLEEP_500KHZ 4
#define SLEEP_250KHZ 5

#define WDT_SECONDS_4 12
#define WDT_SECONDS_8 13
#define WDT_SECONDS_16 14
#define WDT_SECONDS_32 15
	
#include "globaldef.h"

void Timer1_Start();
void Timer1_Init(byte t1ckps, byte tmr1cs);

#include <pic16f1789.h>

void Timer1_Disable();

void timer0_poll_delay(unsigned char ticks, byte division);
void timer1_poll_delay(unsigned short ticks, byte division);

void sleep(unsigned short seconds);

void timer1_poll_delay_ms(unsigned short ms);

inline unsigned int timer1_getValue();
inline void timer1_setValue(unsigned int val);

#define INTERVAL_1S 10
#define INTERVAL_2S 11

void wdt_sleep(int setting);

typedef struct {
	unsigned short tmr1_match;
	byte overflow;
	byte status;
} TmoObj;

TmoObj timer1_timeoutObject(unsigned short ticks);
char timer1_isTimedOut(TmoObj* obj);
#ifdef	__cplusplus
}
#endif

#endif	/* TIMER_H */

