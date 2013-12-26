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
void SendReport(int thermistorResistance, int thermRes25C, int thermBeta, int topResValue) {
    memset(&packet_buffer, 0, sizeof(Packet));
    
    packet_buffer.header.command = TEMP_REPORT;
    packet_buffer.header.flags = 0;
    packet_buffer.header.revision = PROGRAM_REVISION;

	memcpy(&packet_buffer.header.sensorId, &eepromData.sensorId, sizeof(SensorId));
	
	packet_buffer.report.probeBeta = thermBeta;
    packet_buffer.report.probeResistance[0] = thermistorResistance;
    packet_buffer.report.probeResistance25C = thermRes25C;
    packet_buffer.report.topResistorValue = topResValue;

    CRC16_Generate((byte*)&packet_buffer, sizeof(Packet));

    packet_buffer.header.crc.crc16_bytes[1] = CRC16_GetHigh();
    packet_buffer.header.crc.crc16_bytes[0] = CRC16_GetLow();

    char id = SendPacket(&packet_buffer);
	
	XBAPI_ReplyStruct* reply;
	do {
		reply = XBAPI_WaitForReplyTmo(id, 256);
	} while(!reply || (reply->frameType == API_TRANSMIT_STATUS && reply->status != TRANSMIT_SUCCESS));
	XBAPI_FreePacket(id);
}

void SendReceiverBroadcastRequest() {
restartSend:
	memset(&packet_buffer, 0, sizeof(Packet));
    
    packet_buffer.header.command = REQUEST_RECEIVER;
    packet_buffer.header.flags = 0;
    packet_buffer.header.revision = PROGRAM_REVISION;

	// add an if here, when we get around to it.
	memcpy(&packet_buffer.header.sensorId, &eepromData.sensorId, sizeof(SensorId));
	
    CRC16_Generate((byte*)&packet_buffer, sizeof(Packet));

	packet_buffer.header.crc.crc16_bytes[1] = CRC16_GetHigh();
    packet_buffer.header.crc.crc16_bytes[0] = CRC16_GetLow();
    char id = SendPacket(&packet_buffer);

	XBAPI_ReplyStruct* reply;
	byte retries = 0;

	while(1) {
		reply = XBAPI_WaitForReply(id);
		if(reply->frameType == API_RX_INDICATOR) {
			break;
		} else if(reply->frameType == API_TRANSMIT_STATUS) {
			if(reply->status != TRANSMIT_SUCCESS) {
				if(retries < 3) {
					retries++;
					goto restartSend;
				}
				
				LED3_SIGNAL = 0;
				while(1) {
					LED1_SIGNAL = 1;
					timer1_poll_delay(8192, DIVISION_1);
					LED1_SIGNAL = 0;
					timer1_poll_delay(8192, DIVISION_1);
				}
			}
		}
		// XBAPI_AckReply(id);
		reply->frameType = 0;
		reply->status = 0;
	}

	XBAPI_FreePacket(id);
}

unsigned char frame_id_itr = 0;

void __doTestSendPacket() {
    SendPacket(&packet_buffer);
}

unsigned char SendPacket(Packet* packet) {
    if(frame_id_itr == 0) {
        frame_id_itr++;
    }

    char id = XBAPI_Transmit(&dest_address, (byte*) packet, sizeof(Packet)); // FIXME:  Some time I may want to know what the transmit status is.
	if(id == 0xFF) {
		return 0xFF;
	}

	return id;
}
