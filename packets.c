#include "packets.h"
#include "platform_defines.h"
#include "adc.h"
#include "crc16.h"
#include "xbee.h"

Packet packet_buffer;
XBeeAddress dest_address = {{0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF}};

#define THERMISTOR_CHANNEL 10

void SendReport() {
    packet_buffer.header.command = REPORT;
    packet_buffer.header.magic = 0xAA55;
    packet_buffer.header.revision = PROGRAM_REVISION;
    packet_buffer.report.thermistorBeta = THERMISTOR_BETA;
    packet_buffer.report.thermistorResistance = ADC_ReadOne(THERMISTOR_CHANNEL);
    packet_buffer.report.thermistorResistance25C = THERMISTOR_RESISTANCE_25C;
    packet_buffer.report.topResistorValue = TOP_RESISTOR_VALUE;
    packet_buffer.header.crc16 = CRC16_Generate((byte*)&packet_buffer, sizeof(Packet));
    
    SendPacket(&packet_buffer);
}

void SendReceiverBroadcastRequest() {
    packet_buffer.header.command = REQUEST_RECEIVER;
    packet_buffer.header.magic = 0xAA55;
    packet_buffer.header.revision = PROGRAM_REVISION;
    packet_buffer.header.crc16 = CRC16_Generate((byte*)&packet_buffer, sizeof(Packet));

    SendPacket(&packet_buffer);
}

unsigned char frame_id_itr = 0;

void SendPacket(Packet* packet) {
    XBAPI_Transmit(&dest_address, (byte*) packet, sizeof(Packet), ++frame_id_itr);
}