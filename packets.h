/* 
 * File:   packets.h
 * Author: od
 *
 * Created on June 12, 2013, 3:44 PM
 */

#ifndef PACKETS_H
#define	PACKETS_H

#define TEMP_REPORT 0x00
#define REPORT 0x00
#define ERROR_REPORT 0x01

#define RECEIVER_ACK 0x04
#define REQUEST_RECEIVER 0x04

#define RANGE_TEST 0xFF

#include "globaldef.h"

typedef union SensorId {
	byte id[8];
	#ifdef __GNUC__
	uint64 uId;
	#endif
} SensorId;

// This is for the 64-bit XBee address.
typedef struct __XBeeAddress {
    unsigned char addr[8];
} XBeeAddress;

typedef struct __XBeeAddress_7Bytes {
    unsigned char addr[7];
} XBeeAddress_7Bytes;

// This is an application-defined, arbitrary format.
/*struct PacketHeaderRev0 {
    unsigned short magic; // 0xAA55
    union {
        unsigned short crc16;
        unsigned char crc16_bytes[2];
    } crc;
    byte revision;
    byte command;
    unsigned short reserved;
} PACKED_STRUCT;*/

struct PacketHeaderRev1 {
	uint8 flags;
	uint8 reserved0;

	union {
		uint16 crc16;
		uint8 crc16_bytes[2];
	} crc;

	uint8 revision;
	uint8 command;
	uint16 reserved1;
	SensorId sensorId;
} PACKED_STRUCT;

typedef struct PacketHeaderRev1 PacketHeader;

struct ReportRev0 {
	word thermistorResistance;
	word thermistorResistance25C;
	word thermistorBeta;
	word topResistorValue;
	byte xbee_reset;
} PACKED_STRUCT;

struct ReportRev1 {
	int32 probeResistances[3];
	int32 probeResistance25C;
	uint16 probeBeta;
	uint16 batteryLevel;
	uint32 topResistorValue;
} PACKED_STRUCT;

typedef struct ReportRev1 Report;

/*struct RequestReceiverRev0 {
	// empty.
} PACKED_STRUCT;*/

struct RequestReceiverRev1 {
	byte wdt_place;
	byte reserved[31];
} PACKED_STRUCT;

typedef struct RequestReceiverRev1 RequestReceiver;

struct ReceiverAckRev0 {
	byte receiverAddress[8];
} PACKED_STRUCT;

struct ReceiverAckRev1 {
	byte reserved[32];
} PACKED_STRUCT;

typedef struct ReceiverAckRev1 ReceiverAck;

typedef struct ErrorReport {
	unsigned short error;
	unsigned long data;
} ErrorReport;

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
};
// This union allows me to select between using a byte buffer and a myriad of structures for my packets
typedef union {

     struct {
        PacketHeader header; // 16B

        union {
			ErrorReport errReport;
			Report report;
			RequestReceiver requestReceiver;
			ReceiverAck receiverAck;
        };
    };

    byte packet_data[48];
} Packet;

void SendReport(long* thermistorResistances, uint16 battLevel, long thermRes25C, long thermBeta, long topResValue);

void SendReceiverBroadcastRequest();

void SendPacket(Packet* packet, byte id);

unsigned char SendRangeTest();

#define ERR_SUCCESS 0x0000
#define ADC_PVREF_TOO_LOW 0x0001
#define ADC_CONVERSION_TIMEOUT 0x0002
#define FVRRDY_TIMEOUT 0x0003
#define REQUEST_RECEIVER_TIMEOUT 0x0004

void SendErrorReport(unsigned short error, unsigned long data);

#endif	/* PACKETS_H */

