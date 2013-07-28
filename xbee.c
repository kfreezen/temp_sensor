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

extern unsigned char xbee_reset_flag;

int last_xbee_baud;
// It may be necessary to delay.
void XBee_Enable(int baud) {
    XBEE_POWER = 1;
    XBEE_SLEEP_RQ = 0;
    
    while(!XBEE_ON_nSLEEP) {}
    while(XBEE_nCTS) {}
    
    UART_Init(baud);
    last_xbee_baud = baud;
    
    sleep(1);
}

void XBee_Reset() {
    XBee_Disable();
    timer1_poll_delay_ms(1);
    XBee_Enable(last_xbee_baud);
}

inline void XBee_Sleep() {
    XBEE_SLEEP_RQ = 1;
}

void XBee_Wake() {
    XBEE_SLEEP_RQ = 0;
    // wait for nCTS to go low.
    long i = 0;
    while(XBEE_nCTS && i++ < XTAL_FREQUENCY) {
    } // FIXME: This is causing a freeze glitch

    // This and that "&& i++ < XTAL_FREQUENCY" should fix the problem of freezing.
    if(i >= XTAL_FREQUENCY-1 && XBEE_nCTS) {
        LED2_SIGNAL = 1;
        XBee_Disable();
        // The documentation I found said that the minimum reset pulse needed to be 50ns.
        timer1_poll_delay_ms(1);
        XBee_Enable(XBEE_BAUD);
        LED2_SIGNAL = 0;
        xbee_reset_flag = 1;
    }
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

/*byte checksum(void* addr, int length) {
    unsigned char* address = (unsigned char*) addr;

    byte checksum = 0;
    for(; length!=0; length--) {
        checksum += *address++;
    }

    return 0xFF - checksum;
}*/

unsigned char checksum(void* addr, int length) {
    unsigned char* address = (unsigned char*) addr;
    
    // Calculate checksum
    unsigned char checksum = 0;
    int i;
    for(i=0; i<length; i++) {
        checksum += address[i];
    }

    return 0xFF - checksum;
}

unsigned char doChecksumVerify(unsigned char* address, int length, unsigned char checksum) {
    unsigned char check = 0;
    
    int i;
    for(i=0; i<length; i++) {
        check += address[i];
    }

    check += checksum;

    return check;
}

int XBAPI_Transmit(XBeeAddress* address, const unsigned char* data, int length, int id) {
    //byte frame_length = sizeof(TxFrame); // I highly doubt that the frame will be over 256 bytes
    // long, so for now we'll just use a byte as it saves us space.

    apiFrame.tx.start_delimiter = 0x7e;
    apiFrame.tx.length[0] = 0; // Also we'll put this as a zero. (sizeof(TxFrame)-4) >> 8;
    apiFrame.tx.length[1] = sizeof(TxFrame)-4;
    apiFrame.tx.frame_id = id;
    apiFrame.tx.frame_type = API_TRANSMIT_FRAME;
    memcpy(&apiFrame.tx.destination_address, address, sizeof(XBeeAddress));
    apiFrame.tx.reserved = 0xFEFF;
    apiFrame.tx.transmit_options = 0;
    apiFrame.tx.broadcast_radius = 0;
    memcpy(&apiFrame.tx.packet, data, (length>sizeof(Packet)) ? sizeof(Packet) : length);
    apiFrame.tx.checksum = checksum(apiFrame.buffer+3, sizeof(TxFrame)-4);

    UART_TransmitMsg((byte*)&apiFrame, sizeof(TxFrame), 0);

    int error = 0;
    if(id) {
        error = XBAPI_HandleFrame(API_TRANSMIT_STATUS, TRUE);
        if(error == XBEE_TIMEOUT_OCCURRED) {
            // Reset the xbee.
            XBee_Reset();
        }
    }
    
    return error;
    
}

/*u32 swap_endian_32(u32 n) {
    unsigned char* p_n = (unsigned char*) n;
    unsigned char p_ret[4];
    p_ret[0] = p_n[3];
    p_ret[1] = p_n[2];
    p_ret[2] = p_n[1];
    p_ret[3] = p_n[0];

    return *((u32*)p_ret);
}*/

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
    
    byte atCmdLength = (data_valid) ? sizeof(ATCmdFrame) : sizeof(ATCmdFrame_NoData);

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
        return XBAPI_HandleFrame(API_AT_CMD_RESPONSE, TRUE);
    } else {
        return 0;
    }
    
    // TODO:  Add checksum verification and a way to discard bytes that aren't in a frame.
}

// Returns -2 if a timeout occurs.
int XBAPI_HandleFrame(int expected, int do_tmo) {
    // TODO:  Add verification of frames

    long tmo = (do_tmo) ? XTAL_FREQUENCY>>1 : 0;
    
    while (1) {
        int bytes_read = UART_ReceiveMsgTmo(apiFrame.buffer, 3, 0, tmo);
        if(bytes_read<3) {
            return XBEE_TIMEOUT_OCCURRED;
        }
        
        unsigned short received_length = apiFrame.buffer[2];
        received_length |= (apiFrame.buffer[1] << 8);

        UART_ReceiveMsgTmo(apiFrame.buffer + 3, received_length + 1, 0, tmo);

        if (apiFrame.rx.frame_type != expected && expected) {
            // This is better than before, but I don't know if it's the best way.
            continue; // We blatantly discard all unexpected packets.  It will stay this way until it becomes a problem.
            
        } else {
            break;
        }
    }

    
    
    switch(apiFrame.rx.frame_type) { // It's all the same frame type so it doesn't really matter which struct in the union I choose to access it.
        case API_RX_INDICATOR: {
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