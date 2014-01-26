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

void ADC_EnableEx(byte pvrefSel);
void ADC_Enable(void);

void ADC_DisablePin(byte select, byte port_pin);
void ADC_Disable(void);

void DAC_Enable(void);
void DAC_Write(unsigned char w);

uint16 ADC_ReadOne(byte channel);
uint16 ADC_Read(byte channel);

inline byte PROBE_PIN(byte probe);
inline byte PROBE_PORT(byte probe);

inline byte PROBE_CHANNEL(byte probe);
inline byte POT_CHANNEL(byte pot);

long GetProbeResistance(byte probe);

unsigned DetectVdd();

#define PIN_PVREF 1
#define VDD_PVREF 0

#define FVR_CHANNEL 0x1F

#endif	/* ADC_H */

