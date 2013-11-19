/* 
 * File:   asm.h
 * Author: kent
 *
 * Created on November 19, 2013, 11:51 AM
 */

#ifndef ASM_H
#define	ASM_H

#define STR_HELPER(x) #x

#define __TRISA 5
#define __TRISB 6
#define __TRISC 7

#define SET_TRIS(p, v) asm("movlw " STR_HELPER(v) "; tris " STR_HELPER(p))

#endif	/* ASM_H */

