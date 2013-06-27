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

void Timer1_Init(int t1cs, int t1ckps);

#include <pic16lf1783.h>

void Timer1_Disable();

void timer1_poll_delay(unsigned short ticks, int division);
void sleep(unsigned short seconds);

void timer1_poll_delay_ms(unsigned short ms);

#define INTERVAL_1S 10
#define INTERVAL_2S 11

void wdt_sleep(int setting);

#ifdef	__cplusplus
}
#endif

#endif	/* TIMER_H */

