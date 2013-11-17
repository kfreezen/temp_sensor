#include "interrupts.h"
#include <pic16f1788.h>

inline void EnableInterrupts() {
    INTCONbits.GIE = 1;
}

inline void DisableInterrupts() {
    INTCONbits.GIE = 0;
}

inline int AreInterruptsEnabled() {
    return INTCONbits.GIE;
}