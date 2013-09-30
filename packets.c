#include "packets.h"
#include "platform_defines.h"
#include "adc.h"
#include "crc16.h"
#include "xbee.h"

#include <string.h>

Packet packet_buffer;
XBeeAddress dest_address = {{0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF}};

extern unsigned char xbee_reset_flag;
extern unsigned char tmr1_err;
void SendReport(int thermistorResistance, int thermRes25C, int thermBeta, int topResValue) {
    memset(&packet_buffer, 0, sizeof(Packet));
    
    packet_buffer.header.command = REPORT;
    packet_buffer.header.magic = 0xAA55;
    packet_buffer.header.revision = PROGRAM_REVISION;
    packet_buffer.report.thermistorBeta = thermBeta;
    packet_buffer.report.thermistorResistance = thermistorResistance;
    packet_buffer.report.thermistorResistance25C = thermRes25C;
    packet_buffer.report.topResistorValue = topResValue;
    packet_buffer.report.xbee_reset = xbee_reset_flag | (tmr1_err<<1);
    CRC16_Generate((byte*)&packet_buffer, sizeof(Packet));
    packet_buffer.header.crc.crc16_bytes[1] = CRC16_GetHigh();
    packet_buffer.header.crc.crc16_bytes[0] = CRC16_GetLow();

    xbee_reset_flag = 0;
    
    SendPacket(&packet_buffer);
}

void SendReceiverBroadcastRequest() {
    memset(&packet_buffer, 0, sizeof(Packet));
    
    packet_buffer.header.command = REQUEST_RECEIVER;
    packet_buffer.header.magic = 0xAA55;
    packet_buffer.header.revision = PROGRAM_REVISION;
    CRC16_Generate((byte*)&packet_buffer, sizeof(Packet));
    packet_buffer.header.crc.crc16_bytes[1] = CRC16_GetHigh();
    packet_buffer.header.crc.crc16_bytes[0] = CRC16_GetLow();
    SendPacket(&packet_buffer);
}

unsigned char frame_id_itr = 0;

void __doTestSendPacket() {
    SendPacket(&packet_buffer);
}

void SendPacket(Packet* packet) {
    if(frame_id_itr == 0) {
        frame_id_itr++;
    }
    byte status = 1;
    while(status) {
        status = XBAPI_Transmit(&dest_address, (byte*) packet, sizeof(Packet), 0); // FIXME:  Some time I may want to know what the transmit status is.

    }

}
