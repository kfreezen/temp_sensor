/* 
 * File:   xbee.h
 * Author: od
 *
 * Created on June 12, 2013, 11:21 AM
 */

#ifndef XBEE_H
#define	XBEE_H

#include "globaldef.h"
#include "platform_defines.h"
#include "packets.h"

#define NO_RESULT 0
#define RETURNS_RESULT 1

#define MAX_XBEE_REPLIES 8

/**
 *
 * @param baud This sets up the UART to the baud rate selected.  Parameter ignored.
 */
void XBee_Enable(int baud);

/**
 * Powers off the XBee
 */
inline void XBee_Disable();
inline void XBee_Sleep();
void XBee_Wake();

/**
 *
 * @param msg Pointer to buffer containing bytes to send
 * @param len Length of message to send
 * @param end_char The character to stop transmission on.<br>If end_char is 0, it is ignored.  The end_char is sent.
 */
void XBee_Send(const char* msg, int len, const char end_char);

/**
 *
 * @param buf A pointer to a buffer to receive the message
 * @param max_len Maximum length of buffer
 * @param end_char The character to terminate reception after.<br>If end_char is 0, it is ignored.  end_char is included in the message.
 * @return
 */
void XBee_Recv(char* buf, int max_len, const char end_char);

// API Mode

#define API_START_DELIM 0x7E

// Frame types
#define API_AT_CMD_FRAME 0x8
#define API_TRANSMIT_FRAME 0x10

#define API_AT_CMD_RESPONSE 0x88
#define API_MODEM_STATUS 0x8A
#define API_TRANSMIT_STATUS 0x8B
#define API_RX_INDICATOR 0x90

// AT commands
#define CMD_ATDB 0x4244
#define CMD_ATBD 0x4442
#define CMD_ATSM 0x4D53
#define CMD_ATAC 0x4341
#define CMD_ATVR 0x5256
#define CMD_ATPD 0x4450
// Command response codes
#define OK 0
#define ERROR 1
#define INVALID_COMMAND 2
#define INVALID_PARAM 3

#define DEFAULT_TO 0x40 // Set to point-multipoint.

void XBAPI_Command(unsigned short command, unsigned long data, byte data_valid);
void XBAPI_Transmit(XBeeAddress* address, const unsigned char* data, int length, byte id);

#define CMD_SUCCESS 0

// These defines are for the Transmit Status's delivery status field in the xbee 900hp pro (frame type 0x8B)
// They are also defines for XBAPI_HandleFrame's return value
#define TRANSMIT_SUCCESS 0x00
#define MAC_ACK_FAIL 0x01
#define NETWORK_ACK_FAIL 0x21
#define ROUTE_NOT_FOUND 0x25
#define PAYLOAD_TOO_LARGE 0x74
#define INDIRECT_MSG_UNREQUESTED 0x75

// Generic XBAPI_HandleFrame return values
#define NOT_HANDLED -1
#define XBEE_TIMEOUT_OCCURRED -2

#define XBEE_BAUD 111111

#define DEFAULT_XBEE_BAUD 9600

#include "packets.h"

typedef struct {
    byte start_delimiter;
    byte length[2];
    byte frame_type;
    byte frame_id;

    union {
        unsigned short command;
        byte command_bytes[2];
    };

    unsigned long data;
    byte checksum;
} ATCmdFrame;

typedef struct {
    byte start_delimiter;
    byte length[2];
    byte frame_type;
    byte frame_id;

    union {
        unsigned short command;
        byte command_bytes[2];
    };

    byte checksum;
} ATCmdFrame_NoData;

typedef struct {
    byte start_delimiter;
    byte length[2]; // Big-endian length of frame not counting first three bytes or checksum
    byte frame_type;
    byte frame_id;
    XBeeAddress_7Bytes source_address;
    unsigned short reserved; // Should equal 0xFEFF
    byte receive_options;
    Packet packet;
    byte checksum;
} RxFrame;

typedef struct {
    byte start_delimiter;
    byte length[2]; // Big-endian length of frame not counting first three bytes or checksum
    byte frame_type;
    byte frame_id;
    XBeeAddress destination_address;
    unsigned short reserved; // Should equal 0xFEFF
    byte broadcast_radius;
    byte transmit_options;
    Packet packet;
    byte checksum;
} TxFrame;

typedef struct {
    byte start_delimiter;
    byte length[2]; // Big-endian length of frame not counting first three bytes or checksum
    byte frame_type;
    byte frame_id;
    unsigned short reserved; // Should equal 0xFEFF
    byte transmit_retry_count;
    byte delivery_status;
    byte discovery_status;
    byte checksum;
} TxStatusFrame;

typedef struct {
    byte start_delimiter;
    byte length[2];
    byte frame_type;
    byte frame_id;
    union {
        byte command_bytes[2];
        unsigned short command;
    };

    byte cmd_status;
    
    byte data[5]; // 4 bytes is the maximum that we can handle + 1 byte for the checksum.
} ATCmdResponse;

typedef union {
    RxFrame rx;
    TxFrame tx;
    TxStatusFrame txStatus;

    ATCmdFrame atCmd;
    ATCmdFrame_NoData atCmdNoData;

    ATCmdResponse atCmdResponse;
    byte buffer[60];
    
} Frame;

void XBeeAddress_From7ByteAddress(XBeeAddress* dest, XBeeAddress_7Bytes* src);

// This will need to handle RX Indicators and transmit statuses.
// And command replies as well.

// expectedFrameType is ignored if frame is not null.
byte XBAPI_HandleFrame(Frame* frame);
int XBAPI_HandleFrameIfValid(Frame* frame, int length);

int XBAPI_WaitTmo(byte expectedFrame, unsigned ticks);
int XBAPI_Wait(byte expectedFrame);

char XBAPI_ReadFrame(Frame* frame);

void XBee_SwitchBaud(long baud);
#endif	/* XBEE_H */

