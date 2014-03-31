#include "packets.h"
#include "platform_defines.h"
#include "adc.h"
#include "crc16.h"
#include "xbee.h"
#include "eeprom.h"
#include "timer.h"

#include <string.h>

extern EEPROM_Structure eepromData;

Packet packet_buffer;
XBeeAddress dest_address = {{0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF}};

extern unsigned char xbee_reset_flag;
extern unsigned char tmr1_err;
void SendReport(long* thermistorResistances, uint16 battLevel, long thermRes25C, long thermBeta, long topResValue) {
    memset(&packet_buffer, 0, sizeof(Packet));
    
    packet_buffer.header.command = TEMP_REPORT;
    packet_buffer.header.flags = 0;
    packet_buffer.header.revision = PROGRAM_REVISION;

	memcpy(&packet_buffer.header.sensorId, &eepromData.sensorId, sizeof(SensorId));
	
	packet_buffer.report.probeBeta = thermBeta;

	memcpy(packet_buffer.report.probeResistances, thermistorResistances, sizeof(long)*NUM_PROBES);
    packet_buffer.report.probeResistance25C = thermRes25C;
    packet_buffer.report.topResistorValue = topResValue;
	packet_buffer.report.batteryLevel = battLevel;
	
    CRC16_Generate((byte*)&packet_buffer, sizeof(Packet));

    packet_buffer.header.crc.crc16_bytes[1] = CRC16_GetHigh();
    packet_buffer.header.crc.crc16_bytes[0] = CRC16_GetLow();

    SendPacket(&packet_buffer, 0);
}

unsigned char SendRangeTest() {
	memset(&packet_buffer, 0, sizeof(Packet));
    
    packet_buffer.header.command = RANGE_TEST;
    packet_buffer.header.flags = 0;
    packet_buffer.header.revision = PROGRAM_REVISION;

	// add an if here, when we get around to it.
	memcpy(&packet_buffer.header.sensorId, &eepromData.sensorId, sizeof(SensorId));
	
    CRC16_Generate((byte*)&packet_buffer, sizeof(Packet));

	packet_buffer.header.crc.crc16_bytes[1] = CRC16_GetHigh();
    packet_buffer.header.crc.crc16_bytes[0] = CRC16_GetLow();
    SendPacket(&packet_buffer, 1);

	if(XBAPI_WaitTmo(API_RX_INDICATOR, 32768) == -1) {
		return 0;
	} else {
		return 1;
	}
}

// TODO:  Implement receiver xbee id save.
// If we try ten times to get the reciever xbee.

unsigned char failedReceiverBroadcast = 0;

extern unsigned char lastWDTPlace;

void GenerateCRC(Packet* p) {
	CRC16_Generate((byte*) p, sizeof(Packet));
	p->header.crc.crc16_bytes[1] = CRC16_GetHigh();
	p->header.crc.crc16_bytes[0] = CRC16_GetLow();
}

void SendReceiverBroadcastRequest() {
	unsigned char retries = 2;
	while(retries--) {
		unsigned char receiverContactTries = 3;
		while(receiverContactTries--) {
			memset(&packet_buffer, 0, sizeof(Packet));

			packet_buffer.header.command = REQUEST_RECEIVER;
			packet_buffer.header.flags = 0;
			packet_buffer.header.revision = PROGRAM_REVISION;

			// add an if here, when we get around to it.
			memcpy(&packet_buffer.header.sensorId, &eepromData.sensorId, sizeof(SensorId));

			CRC16_Generate((byte*)&packet_buffer, sizeof(Packet));

			packet_buffer.header.crc.crc16_bytes[1] = CRC16_GetHigh();
			packet_buffer.header.crc.crc16_bytes[0] = CRC16_GetLow();
			SendPacket(&packet_buffer, 1);

			int transmitStatus = XBAPI_WaitTmo(API_TRANSMIT_STATUS, 32768);
			if(transmitStatus == -1) {
				// Should have gotten the transmit status by now.  The xbee may not be
				// up, but let's send an error report.

				// First reset the xbee.
				XBee_Disable();
				sleep(2);
				XBee_Enable(XBEE_BAUD);

				SendErrorReport(REQUEST_RECEIVER_TIMEOUT, 0L);
			}

			// Now we make sure that it has been transmitted.
			if(transmitStatus == TRANSMIT_SUCCESS) {
				break;
			} else if(transmitStatus == MAC_ACK_FAIL) {
				// Bad MAC address, reset it to broadcast.
				SetXBeeBroadcastAddress(&dest_address);
			} else {
				XBee_Sleep();
				sleep(2);
				XBee_Wake();
			}
		}

		if(!receiverContactTries) {
			// It failed, we now need to reset the destination address.
			SetXBeeBroadcastAddress(&dest_address);
		} else {
			// It succeeded, so break from the loop.
			int status = XBAPI_WaitTmo(API_RX_INDICATOR, 32768);
			if(status == -1) {
				// Something went wrong, what shall we do?  DECIDE_FIX
			}

			break;
		}
	}
}

/*
typedef struct DiagReport {
	unsigned char PORTA, PORTB, PORTC, PORTD, PORTE;
	unsigned char TRISA, TRISB, TRISC, TRISD, TRISE;
	unsigned char ANSELA, ANSELB, ANSELC, ANSELD, ANSELE;
	unsigned char APFCON1;
	unsigned char BAUD1CON;
	unsigned char SP1BRGL, SP1BRGH;
	unsigned char RC1STA, TX1STA;
	unsigned char TMR1L, TMR1H;
	unsigned char T1CON, T1GCON;
	unsigned char OPTION_REG;
	unsigned char INTCON;
	unsigned char PIE1, PIR1;
	unsigned char STATUS;
	unsigned char reserved;
	unsigned char reservedForExtendedDiagReport;
} DiagReport;
*/

void SendDiagReport() {
	memset(&packet_buffer, 0, sizeof(packet_buffer));

	packet_buffer.header.command = DIAG_REPORT;
	packet_buffer.header.flags = 0;
	packet_buffer.header.revision = PROGRAM_REVISION;

	packet_buffer.diagReport.PORTA = PORTA; 
	packet_buffer.diagReport.PORTB = PORTB; 
	packet_buffer.diagReport.PORTC = PORTC; 
	packet_buffer.diagReport.PORTE = PORTE;

	packet_buffer.diagReport.TRISA = TRISA; 
	packet_buffer.diagReport.TRISB = TRISB; 
	packet_buffer.diagReport.TRISC = TRISC; 
	packet_buffer.diagReport.TRISE = TRISE;

	packet_buffer.diagReport.ANSELA = ANSELA; 
	packet_buffer.diagReport.ANSELB = ANSELB; 
	packet_buffer.diagReport.ANSELC = ANSELC; 

	packet_buffer.diagReport.APFCON1 = APFCON1;
	packet_buffer.diagReport.BAUD1CON = BAUD1CON;
	packet_buffer.diagReport.SP1BRGL = SP1BRGL;
	packet_buffer.diagReport.SP1BRGH = SP1BRGH;

	packet_buffer.diagReport.RC1STA = RC1STA;
	packet_buffer.diagReport.TX1STA = TX1STA;

	packet_buffer.diagReport.TMR1L = TMR1L;
	packet_buffer.diagReport.TMR1H = TMR1H;

	packet_buffer.diagReport.T1CON = T1CON;
	packet_buffer.diagReport.T1GCON = T1GCON;
	packet_buffer.diagReport.OPTION_REG = OPTION_REG;
	packet_buffer.diagReport.INTCON = INTCON;
	packet_buffer.diagReport.PIE1 = PIE1;
	packet_buffer.diagReport.PIR1 = PIR1;
	packet_buffer.diagReport.STATUS = STATUS;
	packet_buffer.diagReport.reserved = 0;
	packet_buffer.diagReport.reservedForExtendedDiagReport = 0;
	GenerateCRC(&packet_buffer);

	SendPacket(&packet_buffer, 0);
}

void SendErrorReport(unsigned short error, unsigned long data) {
	unsigned char do_disable = 0;
	if(XBEE_ON_nSLEEP == 0) {
		XBee_Wake();
		do_disable = 1;
	}


	memset(&packet_buffer, 0, sizeof(packet_buffer));
	
	packet_buffer.header.command = ERROR_REPORT;
	packet_buffer.header.flags = 0;
	packet_buffer.header.revision = PROGRAM_REVISION;

	packet_buffer.errReport.data = data;
	packet_buffer.errReport.error = error;

	GenerateCRC(&packet_buffer);

	SendPacket(&packet_buffer, 0);

	SendDiagReport();
	
	if(do_disable) {
		XBee_Sleep();
	}
}

unsigned char frame_id_itr = 0;

void __doTestSendPacket() {
    SendPacket(&packet_buffer, 0);
}

void SendPacket(Packet* packet, byte id) {
    if(frame_id_itr == 0) {
        frame_id_itr++;
    }

    XBAPI_Transmit(&dest_address, (byte*) packet, sizeof(Packet), id); // FIXME:  Some time I may want to know what the transmit status is.
	
}
