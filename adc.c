#include "adc.h"
#include "timer.h"
#include "globaldef.h"
void ADC_Enable(byte select, byte port_pin) {
    ADCON1bits.ADCS = FOSC_DIV_8;
    if(select) {
        // Port B
        TRISB |= 1<<port_pin;
        ANSELB |= 1<<port_pin;
    } else {
        TRISA |= 1<<port_pin;
        ANSELA |= 1<<port_pin;
    }

    ADCON1bits.ADPREF = 0;
    ADCON1bits.ADNREF = 0;
    ADCON1bits.ADFM = 1;
    ADCON0bits.ADRMD = 0;
}

unsigned ADC_ReadOne(byte channel) {
    ADCON0bits.CHS = channel;
    ADCON0bits.ADON = 1;

    // Wait (1/8000000)*50 seconds (6.25us) for the ADC to charge the holding capacitor.
    timer1_poll_delay(50, DIVISION_1);

    ADCON0bits.GO = 1;
    while(ADCON0bits.GO) {}

    unsigned short result = ADRES;
    PIR1bits.ADIF = 0; // Clearing ADC Interrupt flag for completeness.

    ADCON0bits.ADON = 0;
    
    return result;
}

unsigned ADC_Read(byte channel) {
    while(!OSCSTATbits.OSTS) {}
    
    ADC_ReadOne(channel); // discard this in case it's necessary.

    int res[ADC_NUM_READS];
    byte i;
    for(i=0; i<ADC_NUM_READS; i++) {
        res[i] = ADC_ReadOne(channel);
    }

    int sum = 0;
    for(i=0; i<ADC_NUM_READS; i++) {
        sum += res[i];
    }

    return sum/ADC_NUM_READS;
}