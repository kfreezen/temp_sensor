#include "xbee.h"
#include "uart.h"
#include "timer.h"
#include "packets.h"
#include "globaldef.h"
#include "platform_defines.h"
#include "eeprom.h"

#include <pic16lf1783.h>
#include <string.h>
#include <stddef.h>

extern XBeeAddress dest_address;

int xbee_set_baud_rate = 9600;

// It may be necessary to delay.
void XBee_Enable(int baud) {
    XBEE_POWER = 1;
    XBEE_SLEEP_RQ = 0;
    
    while(!PORTCbits.RC2) {}

    UART_Init(xbee_set_baud_rate);
}

inline void XBee_Sleep() {
    XBEE_SLEEP_RQ = 1;
}

inline void XBee_Wake() {
    XBEE_SLEEP_RQ = 0;
    // wait for nCTS to go low.
    while(XBEE_nCTS) {}
}

inline void XBee_Disable() {
    XBEE_POWER = 0;
}

void XBee_Send(const char* msg, int len, const char end_char) {
    //UART_Init(xbee_set_baud_rate);

    UART_TransmitMsg((byte*)msg, len, end_char);
}

void XBee_Recv(char* buf, int max_len, const char end_char) {
    //UART_Init(xbee_set_baud_rate);

    UART_ReceiveMsg(buf, max_len, end_char);
}

Frame apiFrame;

unsigned char checksum_debug = 0;
void* __addr;

unsigned char checksum(void* addr, int length) {
    unsigned char* address = (unsigned char*) addr;
    __addr = addr;
    
    // Calculate checksum
    unsigned char checksum = 0;
    int i;
    for(i=0; i<length; i++) {
        checksum += address[i];
        checksum_debug = address[i];
    }

    return 0xFF - checksum;
}

unsigned char doChecksumVerify(unsigned char* address, int length, unsigned char checksum) {
    unsigned char check = 0;
    __addr = address;
    
    int i;
    for(i=0; i<length; i++) {
        check += address[i];
    }

    check += checksum;

    return check;
}

void XBAPI_Transmit(XBeeAddress* address, const unsigned char* data, int length, int id) {
    int frame_length = sizeof(TxFrame);
    
    apiFrame.tx.start_delimiter = 0x7e;
    apiFrame.tx.length[0] = (sizeof(TxFrame)-4) >> 8;
    apiFrame.tx.length[1] = (sizeof(TxFrame)-4) & 0xFF;
    apiFrame.tx.frame_id = id;
    apiFrame.tx.frame_type = API_TRANSMIT_FRAME;
    memcpy(&apiFrame.tx.destination_address, address, sizeof(XBeeAddress));
    apiFrame.tx.reserved = 0xFEFF;
    apiFrame.tx.transmit_options = 0;
    apiFrame.tx.broadcast_radius = 0;
    memcpy(&apiFrame.tx.packet, data, (length>sizeof(Packet)) ? sizeof(Packet) : length);
    apiFrame.tx.checksum = checksum(apiFrame.buffer+3, frame_length-4);

    UART_TransmitMsg((byte*)&apiFrame, sizeof(TxFrame), 0);

    int error = 0;
    if(id) {
        error = XBAPI_HandleFrame(API_TRANSMIT_STATUS);
    }
    
    if(error) {
        LED1_SIGNAL = !LED1_SIGNAL;
    }
    
}

u32 swap_endian_32(u32 n) {
    unsigned long r = 0;
    r |= (n&0xFF) << 24;
    r |= ((n>>8)&0xFF) << 16;
    r |= ((n>>16)&0xFF) << 8;
    r |= ((n>>24)&0xFF);

    return r;
}

byte calc_checksum;
int XBAPI_Command(unsigned short command, unsigned long data, int id, int data_valid) {
    
    /*int total_packet_length = 4 + length;

    apiFrame.buffer[0] = 0x7e;
    apiFrame.buffer[1] = total_packet_length >> 8;
    apiFrame.buffer[2] = total_packet_length & 0xFF;
    apiFrame.buffer[3] = API_AT_CMD_FRAME;
    apiFrame.buffer[4] = id;
    memcpy(&apiFrame.buffer[5], &command, 2);
    memcpy(&apiFrame.buffer[7], data, length);
    apiFrame.buffer[total_packet_length] = checksum(apiFrame.buffer+3, total_packet_length);
    */
    
    int atCmdLength = (data_valid) ? sizeof(ATCmdFrame) : sizeof(ATCmdFrame_NoData);

    apiFrame.atCmd.start_delimiter = 0x7e;
    apiFrame.atCmd.length[0] = (atCmdLength-4) >> 8;
    apiFrame.atCmd.length[1] = (atCmdLength-4) & 0xFF;
    apiFrame.atCmd.frame_type = API_AT_CMD_FRAME;
    apiFrame.atCmd.frame_id = id;
    apiFrame.atCmd.command = command;
    apiFrame.atCmd.data = swap_endian_32(data);
   
    calc_checksum = checksum(apiFrame.buffer+3, atCmdLength-4);
    byte check = doChecksumVerify(apiFrame.buffer+3, atCmdLength-4, calc_checksum);
    
    if(data_valid) {
        apiFrame.atCmd.checksum = calc_checksum;
    } else {
        apiFrame.atCmdNoData.checksum = calc_checksum;
    }

    UART_TransmitMsg(apiFrame.buffer, atCmdLength, 0);

    if(id) {
        return XBAPI_HandleFrame(API_AT_CMD_RESPONSE);
    } else {
        return 0;
    }
    
    
    // TODO:  Add checksum verification and a way to discard bytes that aren't in a frame.
}

int XBAPI_HandleFrame(int expected) {
    // TODO:  Add verification of frames

    while (1) {
        UART_ReceiveMsg(apiFrame.buffer, 3, 0);

        unsigned short received_length = apiFrame.buffer[2];
        received_length |= (apiFrame.buffer[1] << 8);

        UART_ReceiveMsg(apiFrame.buffer + 3, received_length + 1, 0);

        if (apiFrame.rx.frame_type != expected && expected) {
            // This is better than before, but I don't know if it's the best way.
            continue;
        } else {
            break;
        }
    }

    
    
    switch(apiFrame.rx.frame_type) { // It's all the same frame type so it doesn't really matter which struct in the union I choose to access it.
        case API_RX_INDICATOR: {
            // TODO:  Implement
            Packet* packet = &apiFrame.rx.packet;
            switch(packet->header.command) {
                case REQUEST_RECEIVER:
                {
                    memcpy(&dest_address, &apiFrame.rx.source_address, sizeof(XBeeAddress));
                    // These nodes will only need to ever reply to one address,
                    // allowing us to just use dest_address for this purpose.
                    // TODO:  change dest_address to receiver_address.
                } break;
            }
            
        } break;

        case API_TRANSMIT_STATUS: {
            // TODO:  Add error handling code here.
            return apiFrame.txStatus.delivery_status;
        }

        case API_AT_CMD_RESPONSE: {
            return apiFrame.atCmdResponse.cmd_status;
        }

        default:
            break;
    }

    return NOT_HANDLED;
}