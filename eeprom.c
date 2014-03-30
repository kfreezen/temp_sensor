#include "eeprom.h"
#include "globaldef.h"
#include "timer.h"

#include <pic16f1788.h>

void EEPROM_Read(byte address, byte* data, int length) {
    unsigned char i;
    for(i=0; i<length; i++) {
        EEADRL = address+i;
        EECON1bits.EEPGD = 0;
        EECON1bits.CFGS = 0;
        EECON1bits.RD = 1;
        data[i] = EEDATL;
    }
    
}

void EEPROM_Write(byte address, byte* data, int length) {
    unsigned char i;
    for(i=0; i<length; i++) {
        EEADRL = address+i;
        EEDATL = data[i];

        EECON1bits.WREN = 1;
        EECON2 = 0x55;
        EECON2 = 0xAA;
        EECON1bits.WR = 1;

        // Now wait for completion of the write
        while(!PIR2bits.EEIF) {} // DECIDE_FIX
        PIR2bits.EEIF = 0;
        EECON1bits.WREN = 0;
    }

    EECON1bits.WREN = 0;
}