#include "adc.h"
#include "timer.h"
#include "globaldef.h"
#include "platform_defines.h"
#include "xbee.h"

#define PREF_VDD 0
#define PREF_VREFPIN 1

#define PVREF_PIN 3
#define PVREF_CHANNEL 3

#define FVR_VALUE_AT_MIN_ANALOG_VOLTAGE 1733 // This equals 2.42 volts with
// an FVR of 1.024V

#define FVR_VALUE_AT_MAX_ANALOG_VOLTAGE 1497

extern unsigned char xbee_enabled;

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

unsigned DetectVdd() {
	ADCON1bits.ADPREF = VDD_PVREF;
	// Use FVR to detect the voltage.
	FVRCONbits.FVREN = 1;
	FVRCONbits.ADFVR = 1; // 1.024V

	TmoObj obj = timer1_timeoutObject(1000);
	unsigned short tmr1_start = TMR1;

	while(!FVRCONbits.FVRRDY && !timer1_isTimedOut(&obj));

	if(obj.status == 1 || TMR1 - tmr1_start >= 164) {
		SendErrorReport(FVRRDY_TIMEOUT, TMR1 - tmr1_start);
	}

	unsigned short long result = 0;

	result = ADC_Read(FVR_CHANNEL);
	//result = ADC_Read(FVR_CHANNEL);
	result = (262144 / result) * 1024;
	result >>= 6;

	FVRCONbits.FVREN = 0;
	return result;
}

void ADC_EnableEx(byte pvrefSel) {
	asm("clrwdt");

	if(pvrefSel == PIN_PVREF) {
		ANALOG_POWER_TOGGLE = 1;

		// OK, what we want to do here is poll the VREF+ pin until the value reaches
		// roughly 2.425 (2.5-(3% * 2.5))
		// We enable this pin...
		ADC_EnablePin(SEL_PORTA, PVREF_PIN);
	}

	ADCON1bits.ADCS = FOSC_DIV_8;
	ADCON1bits.ADNREF = 0;
	if(pvrefSel == PIN_PVREF) {
		ADCON1bits.ADPREF = PIN_PVREF;
	}
	
	ADCON1bits.ADFM = 1;
	ADCON0bits.ADRMD = 0;
	ADCON0bits.ADON = 1;

	//timer1_poll_delay(120, DIVISION_1);

	asm("clrwdt");
	
	if(pvrefSel == PIN_PVREF) {
		// Use FVR to detect the voltage.
		FVRCONbits.FVREN = 1;
		FVRCONbits.ADFVR = 1; // 1.024V
		
		TmoObj tmoObj = timer1_timeoutObject(164);
		
		while(!FVRCONbits.FVRRDY && !timer1_isTimedOut(&tmoObj));

		if(tmoObj.status == 1) {
			SendErrorReport(FVRRDY_TIMEOUT, 0);
		}

		unsigned short long result = 0;

		// The most this should take is 50 microseconds.
		// Let's time out after five milliseconds.
		tmoObj = timer1_timeoutObject(164);
		
		asm("clrwdt");
		while(result < 2420 && !timer1_isTimedOut(&tmoObj)) {
			result = ADC_Read(FVR_CHANNEL);
			result = (262144 / result) * 1024;
			result >>= 6; // /= 64;
		}

		if(tmoObj.status == 1) {
			unsigned char do_disable = 0;
			
			SendErrorReport(ADC_PVREF_TOO_LOW, result);
		}

		FVRCONbits.FVREN = 0;
		
		//ADC_DisablePin(SEL_PORTA, PVREF_PIN);
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
	ADC_DisablePin(SEL_PORTA, PVREF_PIN);
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
	} else {
		TRISA &= ~(1 << port_pin);
		ANSELA &= ~(1 << port_pin);
	}
}

uint16 ADC_ReadOne(byte channel) {
	ADCON0bits.CHS = channel;
	
	// Wait (1/(8000000/4))*13 seconds (~6.4us) for the ADC to charge the holding capacitor.
	timer0_poll_delay(13, DIVISION_1);

	//
	TmoObj conversionTmo = timer1_timeoutObject(164);

	ADCON0bits.GO = 1;
	while (ADCON0bits.GO && !timer1_isTimedOut(&conversionTmo)) {
	}

	if(conversionTmo.status == 1) {
		// Timed out.
		long data = ADCON0 | (ADCON1 << 8);
		/*unsigned char* datap = (unsigned char*) &data;
		datap[0] = ADCON0;
		datap[1] = ADCON1;*/

		SendErrorReport(ADC_CONVERSION_TIMEOUT, data);
	}
	
	unsigned short result = ADRES;
	PIR1bits.ADIF = 0; // Clearing ADC Interrupt flag for completeness.

	//ADCON0bits.ADON = 0;

	return result;
}

uint16 ADC_Read(byte channel) {
	TmoObj oscstatTmo = timer1_timeoutObject(2048);
	while (!OSCSTATbits.OSTS && !timer1_isTimedOut(&oscstatTmo)) {
	}

	if(!OSCSTATbits.OSTS && oscstatTmo.status) {
		// We need to figure out how to handle this.
		// DECIDE_FIX
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

	return (unsigned) (sum / (long) ADC_NUM_READS);
}

#define LOWER_BOUND_READ_VALID  20 // about 12mv
// Probably doesn't belong in adc.c

byte probe__;
long GetProbeResistance(byte probe) {
	probe__ = probe;
	long n = ADC_Read(PROBE_CHANNEL(probe));
	if(n < LOWER_BOUND_READ_VALID) {
		return 0L;
	} else {
		return (TOP_RESISTOR_VALUE * 1000L) / ((4096000L/ADC_Read(PROBE_CHANNEL(probe)))-1000);
	}
}

void DAC_Enable() {
	DAC1CON0bits.DAC1EN = 1;
	TRISBbits.TRISB7 = 0;
	DAC1CON0bits.DAC1OE1 = 0;
	DAC1CON0bits.DAC1OE2 = 1;
	DAC1CON0bits.DAC1PSS = 0;
	DAC1CON0bits.DAC1NSS = 0;
}

void DAC_Write(unsigned char w) {
	DAC1CON1bits.DAC1R = w;
}

void DAC_Disable() {
	DAC1CON0bits.DAC1EN = 0;
}