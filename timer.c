#include "timer.h"
#include <pic16f1788.h>
#include "platform_defines.h"

void Timer1_Init() {
	byte tmr1cs, t1ckps;
	t1ckps = DIVISION_1;
	tmr1cs = TMR1_PINOSC;
	
    T1CONbits.TMR1CS = tmr1cs;
    T1CONbits.TMR1ON = 1;
    T1GCONbits.TMR1GE = 0;

    T1CONbits.T1CKPS = t1ckps;
    if(tmr1cs == TMR1_PINOSC) {
		T1CONbits.T1OSCEN = 1;
	}

	TMR1 = 0xFC00;
	PIR1bits.TMR1IF = 0;
	// wait for TMR1 to stabilize.
	while(!PIR1bits.TMR1IF) {}
}

#define TIMER1_INIT_TRIES 3

unsigned char tmr1_err = 0;

#define TMR0_INST_CLOCK 0
void timer0_poll_delay(byte ticks, byte division) {
	OPTION_REGbits.TMR0CS = TMR0_INST_CLOCK;
	OPTION_REGbits.PS = division - 1;
	if(division) {
		OPTION_REGbits.PSA = 1;
	}

	INTCONbits.TMR0IF = 0;
	TMR0 = 255 - ticks;
	while(!INTCONbits.TMR0IF) {}
	
}

void timer1_poll_delay(unsigned short ticks, byte division) {
    //unsigned char TMR1L_cmp = ticks & 0xFF;
    //unsigned char TMR1H_cmp = ticks >> 8;
	byte savedDivision = T1CONbits.T1CKPS;
	T1CONbits.T1CKPS = division;

	TMR1 = 0;

    unsigned char tmr1_turned_on = 0;

    while(tmr1_turned_on < TIMER1_INIT_TRIES) {
        // For some reason TMR1ON is 0.

        /*if(T1CONbits.TMR1ON == 0) {
            // I shouldn't have to have this "if" here.
            Timer1_Init(TMR1_PINOSC, division);
            //TMR1 = 0;
            tmr1_turned_on ++;
        }*/

        if(TMR1 >= ticks) {
            break;
        }
    }

    if(tmr1_turned_on >= TIMER1_INIT_TRIES) {
        tmr1_err = 1;
    }

	T1CONbits.T1CKPS = savedDivision;
}

void timer1_sleep(unsigned short periods) {
	PIE1bits.TMR1IE = 1;
	T1CONbits.nT1SYNC = 1;
	//T1GCONbits.TMR1GE = 1;
	
	while(periods --) {
		asm("sleep");
	}

	while(!OSCSTATbits.OSTS) {}
}

void sleep(unsigned short seconds) {
    /*int i;
    for(i=0; i<seconds; i++) {
        WDTCONbits.SWDTEN = 1;
        WDTCONbits.WDTPS = INTERVAL_1S;
        asm("sleep");
        WDTCONbits.SWDTEN = 0;
    }*/

	timer1_sleep(seconds >> 1);
}

void Timer1_Disable()  {
    T1CONbits.TMR1ON = 0;
}

inline unsigned int timer1_getValue() {
	return TMR1;
}

inline void timer1_setValue(unsigned int val) {
	TMR1 = val;
}