/* 
 * File:   adc.h
 * Author: od
 *
 * Created on June 13, 2013, 9:39 AM
 */

#ifndef ADC_H
#define	ADC_H

#define SEL_PORTA 0
#define SEL_PORTB 1
#define ADC_NUM_READS 8 // This specifies how many reads the program should average.
#define FOSC_DIV_8 1
#include "globaldef.h"

void ADC_EnablePin(byte select, byte port_pin);
void ADC_Enable(void);

void ADC_DisablePin(byte select, byte port_pin);
void ADC_Disable(void);

unsigned ADC_ReadOne(byte channel);
unsigned ADC_Read(byte channel);

inline byte PROBE_PIN(byte pin);
inline byte PROBE_PORT(byte port);

#endif	/* ADC_H */

