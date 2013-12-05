#include "adc.h"
#include "timer.h"
#include "globaldef.h"
#include "platform_defines.h"

#define PREF_VDD 0
#define PREF_VREFPIN 1

#define PVREF_PIN 3
#define PVREF_CHANNEL 3

#define MINIMUM_ANALOG_VOLTAGE 3009 // This equals 2.42 volts with
// a VDD of 3.3

const byte PROBE_PORTS[3] = {
	SEL_PORTA, SEL_PORTA, SEL_PORTA
};

const byte PROBE_PINS[3] = {
	0, 1, 5
};

const byte PROBE_CHANNELS[3] = {
	0, 1, 4
};

const byte POT_CHANNELS[3] = {
	POT1_CHANNEL, POT2_CHANNEL, POT3_CHANNEL
};

inline byte PROBE_PORT(byte port) {
	return PROBE_PORTS[port];
}

inline byte PROBE_PIN(byte pin) {
	return PROBE_PINS[pin];
}

inline byte PROBE_CHANNEL(byte probe) {
	return PROBE_CHANNELS[probe];
}

inline byte POT_CHANNEL(byte pot) {
	return POT_CHANNELS[pot];
}

void ADC_EnableEx(byte pvrefSel) {
	if(pvrefSel == PIN_PVREF) {
		ANALOG_POWER_TOGGLE = 1;

		// OK, what we want to do here is poll the VREF+ pin until the value reaches
		// roughly 2.425 (2.5-(3% * 2.5))
		ADC_EnablePin(SEL_PORTA, PVREF_PIN);
	}

	ADCON1bits.ADCS = FOSC_DIV_8;
	ADCON1bits.ADPREF = PREF_VDD; // We have to detect the VREF+ voltage.
	ADCON1bits.ADNREF = 0;
	ADCON1bits.ADFM = 1;
	ADCON0bits.ADRMD = 0;
	ADCON0bits.ADON = 1;

	//timer1_poll_delay(120, DIVISION_1);

	if(pvrefSel == PIN_PVREF) {
		unsigned result = 0;
		while(result < MINIMUM_ANALOG_VOLTAGE) {
			result = ADC_Read(PVREF_CHANNEL);
		}
		ADC_DisablePin(SEL_PORTA, PVREF_PIN);
		ADCON1bits.ADPREF = PREF_VREFPIN;
	} else {
		ADCON1bits.ADPREF = VDD_PVREF;
	}

	// It should be ready to go now.
}

void ADC_Enable() {
	ADC_EnableEx(PIN_PVREF);
}

void ADC_Disable() {
	ANALOG_POWER_TOGGLE = 0;
	ADCON0bits.ADON = 0;
}

void ADC_EnablePin(byte select, byte port_pin) {
	if (select) {
		// Port B
		TRISB |= 1 << port_pin;
		ANSELB |= 1 << port_pin;
	} else {
		TRISA |= 1 << port_pin;
		ANSELA |= 1 << port_pin;
	}


}

void ADC_DisablePin(byte select, byte port_pin) {
	if(select == SEL_PORTB) {
		TRISB &= ~(1 << port_pin);
		ANSELB &= ~(1 << port_pin);
	}
}

uint16 ADC_ReadOne(byte channel) {
	ADCON0bits.CHS = channel;
	
	// Wait (1/8000000)*50 seconds (6.25us) for the ADC to charge the holding capacitor.
	timer1_poll_delay_fast(50, DIVISION_1);

	ADCON0bits.GO = 1;
	while (ADCON0bits.GO) {
	}

	unsigned short result = ADRES;
	PIR1bits.ADIF = 0; // Clearing ADC Interrupt flag for completeness.

	//ADCON0bits.ADON = 0;

	return result;
}

uint16 ADC_Read(byte channel) {
	while (!OSCSTATbits.OSTS) {
	}

	ADC_ReadOne(channel); // discard this in case it's necessary.

	unsigned res[ADC_NUM_READS];
	byte i;
	for (i = 0; i < ADC_NUM_READS; i++) {
		res[i] = ADC_ReadOne(channel);
	}

	long sum = 0;
	for (i = 0; i < ADC_NUM_READS; i++) {
		sum += res[i];
	}

	return (unsigned) 0xFFFF; //(sum / (long) ADC_NUM_READS);
}

// Probably doesn't belong in adc.c
long GetProbeResistance(byte probe) {
	return (TOP_RESISTOR_VALUE * 1000L) / ((4096000L/ADC_Read(PROBE_CHANNEL(probe)))-1000);
}