/* 
 * File:   packets.h
 * Author: od
 *
 * Created on June 12, 2013, 3:44 PM
 */

#ifndef PACKETS_H
#define	PACKETS_H

#define REPORT 0x00
#define CALIBRATE 0x01
#define REQUEST_RECEIVER 0x04

#include "globaldef.h"

// This is an application-defined, arbitrary format.
typedef struct {
    unsigned short magic; // 0xAA55
    unsigned short crc16;
    byte revision;
    byte command;
    unsigned short reserved;
} PacketHeader;

// This union allows me to select between using a byte buffer and a myriad of structures for my packets
typedef union {

     struct {
        PacketHeader header;

        union {

            struct {
                word thermistorResistance;
                word thermistorResistance25C;
                word thermistorBeta;
                word topResistorValue;
            } report;

            struct {
                short thermistorResistance25CAdjust;
                short topResistorValueAdjust;
            } calibration;

            struct {
                word interval;
            } interval;

            struct {
                word networkID;
                byte preambleID;
            } set_id;

            struct {
                byte address[8];
            } receiver_address;
        };
    };

    byte packet_data[32];
} Packet;

void SendReport();
void SendReceiverBroadcastRequest();

void SendPacket(Packet* packet);

#endif	/* PACKETS_H */

