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
#define ADC_NUM_READS 4 // This specifies how many reads the program should average.
#define FOSC_DIV_8 1

void ADC_Enable(int select, int port_pin);
unsigned ADC_ReadOne(int channel);
unsigned ADC_Read(int channel);

#endif	/* ADC_H */

