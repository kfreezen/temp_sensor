/* 
 * File:   xbee.h
 * Author: od
 *
 * Created on June 12, 2013, 11:21 AM
 */

#ifndef XBEE_H
#define	XBEE_H

#include "globaldef.h"

// Pin name defines
#define XBEE_POWER PORTCbits.RC0
#define XBEE_SLEEP_RQ PORTCbits.RC1
#define XBEE_ON_nSLEEP PORTCbits.RC2

#define NO_RESULT 0
#define RETURNS_RESULT 1

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
inline void XBee_Wake();

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
// Frame types
#define API_AT_CMD_FRAME 0x8
#define API_TRANSMIT_FRAME 0x10

#define API_AT_CMD_RESPONSE 0x88
#define API_MODEM_STATUS 0x8A
#define API_TRANSMIT_STATUS 0x8B
#define API_RX_INDICATOR 0x90

// AT commands
#define CMD_ATBD 0x4442
#define CMD_ATSM 0x4D53
#define CMD_ATAC 0x4341

// Command response codes
#define OK 0
#define ERROR 1
#define INVALID_COMMAND 2
#define INVALID_PARAM 3

// This is for the 64-bit XBee address.
typedef struct __XBeeAddress {
    unsigned char addr[8];
} XBeeAddress;

int XBAPI_Command(unsigned short command, unsigned long data, int id, int data_valid);
void XBAPI_Transmit(XBeeAddress* address, const unsigned char* data, int length, int id);

// These defines are for the Transmit Status's delivery status field in the xbee 900hp pro (frame type 0x8B)
// They are also defines for XBAPI_HandleFrame's return value
#define SUCCESS 0x00
#define MAC_ACK_FAIL 0x01
#define NETWORK_ACK_FAIL 0x21
#define ROUTE_NOT_FOUND 0x25
#define PAYLOAD_TOO_LARGE 0x74
#define INDIRECT_MSG_UNREQUESTED 0x75

// Generic XBAPI_HandleFrame return values
#define NOT_HANDLED -1

// This will need to handle RX Indicators and transmit statuses.
int XBAPI_HandleFrame(int expected);

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
    XBeeAddress source_address;
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

typedef union {
    RxFrame rx;
    TxFrame tx;
    TxStatusFrame txStatus;

    ATCmdFrame atCmd;
    ATCmdFrame_NoData atCmdNoData;
    
    byte buffer[60];
    
} Frame;

#endif	/* XBEE_H */

