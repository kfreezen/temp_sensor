/* 
 * File:   interrupts.h
 * Author: Kent
 *
 * Created on May 7, 2013, 10:11 PM
 */

#ifndef INTERRUPTS_H
#define	INTERRUPTS_H

#ifdef	__cplusplus
extern "C" {
#endif

    inline void EnableInterrupts();
    inline void DisableInterrupts();
    inline int AreInterruptsEnabled();
    
#ifdef	__cplusplus
}
#endif

#endif	/* INTERRUPTS_H */

