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

void SendReceiverBroadcastRequest() {
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

	while(1) {
		unsigned char receiverContactTries = 10;
		while(receiverContactTries--) {
			int transmitStatus = XBAPI_Wait(API_TRANSMIT_STATUS);
			// Now we make sure that it has been transmitted.
			if(transmitStatus == TRANSMIT_SUCCESS) {
				break;
			} else {
				XBee_Sleep();
				sleep(10);
				XBee_Wake();
			}
		}

		if(receiverContactTries) {
			failedReceiverBroadcast = 0;
			XBAPI_Wait(API_RX_INDICATOR);
		} else {
			if(failedReceiverBroadcast) {
				// We already did this, so we should just return, and go into a
				// lowpower mode.
				return;
			}
			// Set the broadcast destination, which is 0x000000000000FFFF
			memset(&eepromData.sensorId, 0, sizeof(SensorId));
			eepromData.sensorId.id[7] = eepromData.sensorId.id[6] = 0xFF;
			failedReceiverBroadcast = 1;
			// We should now go back and try the transmitting again.
		}
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
