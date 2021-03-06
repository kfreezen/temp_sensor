#include "timer.h"
#include <pic16f1789.h>
#include "platform_defines.h"

void Timer1_Start() {
	Timer1_Init(DIVISION_1, TMR1_PINOSC);
	
	TMR1 = 0xFC00;
	PIR1bits.TMR1IF = 0;
	// wait for TMR1 to stabilize.
	while(!PIR1bits.TMR1IF) {}
}

void Timer1_Init(byte t1ckps, byte tmr1cs) {
	
    T1CONbits.TMR1CS = tmr1cs;
    T1CONbits.TMR1ON = 1;
    T1GCONbits.TMR1GE = 0;

    T1CONbits.T1CKPS = t1ckps;
    if(tmr1cs == TMR1_PINOSC) {
		T1CONbits.T1OSCEN = 1;
	}
		
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

extern unsigned char timer1_overflow;
TmoObj timer1_timeoutObject(unsigned short ticks) {
	TmoObj obj;
	obj.tmr1_match = TMR1 + ticks;
	obj.overflow = 0;
	if(obj.tmr1_match < TMR1) {
		obj.overflow++;
	}

	return obj;
}

char timer1_isTimedOut(TmoObj* obj) {
	if(obj->overflow == 1 && TMR1 <= obj->tmr1_match) {
		// We must've looped around, set overflow to 0.
		obj->overflow = 0;
	}

	if(obj->overflow == 0 && TMR1 >= obj->tmr1_match) {
		obj->status = 1;
		return 1;
	} else {
		obj->status = 0;
		return 0;
	}

	/*if(obj->overflow >= timer1_overflow && obj->tmr1_match >= TMR1) {
		obj->status = 1;
		return 1;
	} else {
		obj->status = 0;
		return 0;
	}*/
}

void timer1_poll_delay(unsigned short ticks, byte division) {
    //unsigned char TMR1L_cmp = ticks & 0xFF;
    //unsigned char TMR1H_cmp = ticks >> 8;
	byte savedDivision = T1CONbits.T1CKPS;
	T1CONbits.T1CKPS = division;

	unsigned short expected = TMR1 + ticks;
	if(expected < TMR1) {
		// We want to wait until TMR1 is less than expected.
		while(TMR1 > expected) {
		}
	}

	// Now wait till TMR1 is greater than expected.
	while(TMR1 < expected) {

	}

	T1CONbits.T1CKPS = savedDivision;
}

void timer1_sleep(unsigned short periods) {
	Timer1_Init(DIVISION_1, TMR1_PINOSC);

	// The most we will ever sleep is 16 seconds at a time so give ourselves plenty of margin
	// for error, but not enough that we're going to be stuck somewhere forever.
	// Save our timeout as well.
	unsigned char last_wdtps = WDTCONbits.WDTPS;
	WDTCONbits.WDTPS = WDT_SECONDS_32;
	
	asm("clrwdt");

	PIE1bits.TMR1IE = 1;
	T1CONbits.nT1SYNC = 1;
	//T1GCONbits.TMR1GE = 1;
	
	int div_periods = periods >> 3;
	periods -= div_periods << 3;
	T1CONbits.T1CKPS = DIVISION_8;
	while(div_periods--) {
		asm("sleep");
		asm("clrwdt");
	}

	T1CONbits.T1CKPS = DIVISION_1;

	WDTCONbits.WDTPS = WDT_SECONDS_4;
	while(periods --) {
		asm("sleep");
		asm("clrwdt");
	}

	while(!OSCSTATbits.OSTS) {}
	WDTCONbits.WDTPS = last_wdtps;

	asm("clrwdt");
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
