#include "timer.h"
#include <pic16lf1783.h>
#include "platform_defines.h"

void Timer1_Init(int tmr1cs, int t1ckps) {
    T1CONbits.TMR1CS = tmr1cs;
    T1CONbits.TMR1ON = 1;
    T1GCONbits.TMR1GE = 0;

    T1CONbits.T1CKPS = t1ckps;
}

#define TIMER1_INIT_TRIES 3

unsigned char tmr1_err = 0;

void timer1_poll_delay(unsigned short ticks, int division) {
    //unsigned char TMR1L_cmp = ticks & 0xFF;
    //unsigned char TMR1H_cmp = ticks >> 8;

    Timer1_Disable();

    TMR1 = 0;

    Timer1_Init(SYS_CLOCK, division);

    unsigned char tmr1_turned_on = 0;

    while(tmr1_turned_on < TIMER1_INIT_TRIES) {
        // For some reason TMR1ON is 0.

        if(T1CONbits.TMR1ON == 0) {
            // I shouldn't have to have this "if" here.
            Timer1_Init(SYS_CLOCK, division);
            TMR1 = 0;
            tmr1_turned_on ++;
        }

        if(TMR1 >= ticks) {
            break;
        }
    }

    if(tmr1_turned_on >= TIMER1_INIT_TRIES) {
        tmr1_err = 1;
    }
    
    Timer1_Disable();
}

void timer1_poll_delay_ms(unsigned short ms) {
    // How many ticks in one ms.

    timer1_poll_delay(ms*(XTAL_FREQUENCY/1000), DIVISION_1); // This /1000 is causing freezing for some reason.
}

void sleep(unsigned short seconds) {
    int i;
    for(i=0; i<seconds; i++) {
        WDTCONbits.SWDTEN = 1;
        WDTCONbits.WDTPS = INTERVAL_1S;
        asm("sleep");
        WDTCONbits.SWDTEN = 0;
    }
}

void Timer1_Disable()  {
    T1CONbits.TMR1ON = 0;
}