#include "xbee.h"
#include "uart.h"
#include "timer.h"
#include "packets.h"
#include "globaldef.h"
#include "platform_defines.h"
#include "eeprom.h"
#include "bits.h"
#include "timer.h"
#include "interrupts.h"

#include <pic16f1788.h>
#include <string.h>
#include <stddef.h>

byte nCTS_Valid = 0;

extern XBeeAddress dest_address;
extern EEPROM_Structure eepromData;

extern unsigned char xbee_reset_flag;

long last_xbee_baud;
// It may be necessary to delay.

XBeeAddress* debug_dest;
XBeeAddress_7Bytes* debug_src;

void XBeeAddress_From7ByteAddress(XBeeAddress* dest, XBeeAddress_7Bytes* src) {
    debug_dest = dest;
    debug_src = src;
    memset(dest->addr, 0, sizeof(XBeeAddress));
    memcpy(dest->addr+1, src, sizeof(XBeeAddress_7Bytes));
}

void XBee_StopReset() {
	XBEE_nRESET_TRIS = INPUT;
	XBEE_nRESET_LATCH = 0;
}

void XBee_StartReset() {
	XBEE_nRESET_TRIS = OUTPUT;
	XBEE_nRESET_LATCH = 0;
}

/*void XBee_SwitchBaud(long baud) {
	XBAPI_Command(CMD_ATBD, baud, TRUE);
	
	// There's no guarantee we'll receive something here, so
	// lets just switch UART speed and wait for reply.
	// If none received, revert to default in the MCU side.
	UART_Init(baud);

	int status = XBAPI_Wait(API_AT_CMD_RESPONSE);
	if(status != 0) {
		UART_Init(DEFAULT_XBEE_BAUD);
	}
}*/

unsigned char xbee_enabled = 0;

void XBee_Enable(int baud) {
	if(xbee_enabled) {
		return;
	}
	
	UART_Init(baud);
    last_xbee_baud = baud;
	
	XBee_StartReset();
	timer1_poll_delay(40, DIVISION_1);
	XBee_StopReset();
	
    XBEE_SLEEP_RQ = 0;
    
    while(!XBEE_ON_nSLEEP) {}
    while(XBEE_nCTS) {
		//timer1_poll_delay(40, DIVISION_1);
		/*byte id = XBAPI_Command(CMD_ATVR, 0L, 0);
		___reply = XBAPI_WaitForReplyTmo(id, 32768);
		if(___reply) {
			if(XBEE_nCTS) {
				nCTS_Valid = 0;
			}

			working = 1;
			break;
		} else {
			XBee_StartReset();
			timer1_poll_delay(40, DIVISION_1);
			XBee_StopReset();
		}*/
	}

	int err = XBAPI_WaitTmo(API_MODEM_STATUS, 32768); // timeout in 1 second.
	if(err == -1) {
		// Something went wrong, what shall we do?
		// TODO:  Decide.

		// For now we're just going to sleep for 30 seconds and then issue a reset command.
		XBee_Disable();
		sleep(30);
		asm("reset");
	}

	xbee_enabled = 1;
}

void XBee_Reset() {
    XBee_Disable();
	// about 1.2 ms.
    timer1_poll_delay(40, DIVISION_1);
    XBee_Enable(last_xbee_baud);
}

inline void XBee_Sleep() {
	// Wait about 5ms. for things to settle.
	timer1_poll_delay(163, DIVISION_1);
	XBEE_SLEEP_RQ = 1;
}

void XBee_Wake() {
    XBEE_SLEEP_RQ = 0;
    // wait for nCTS to go low.
    long i = 0;
    while(XBEE_nCTS && i++ < XTAL_FREQUENCY) {
    }

    // This and that "&& i++ < XTAL_FREQUENCY" should fix the problem of freezing.
    if(i >= XTAL_FREQUENCY-1 && XBEE_nCTS) {
        XBee_Disable();
        // The documentation I found said that the minimum reset pulse needed to be 50ns.
        timer1_poll_delay(40, DIVISION_8);
        XBee_Enable(DEFAULT_XBEE_BAUD);
		
        xbee_reset_flag = 1;
    }
}

inline void XBee_Disable() {
    XBEE_nRESET_LATCH = 0;
	XBEE_nRESET_TRIS = OUTPUT;
	xbee_enabled = 0;
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

byte lastFrameType = 0;
int status = 0;
uint32 userData = 0;

// This function basically waits for a certain frame type to be received.
extern unsigned char timer1_flag;
int XBAPI_WaitTmo(byte expectedFrameType, unsigned tmo) {
	unsigned expected = TMR1 + tmo;
	if(expected < tmo) { // TMR1 must overflow before we call timeout.
		timer1_flag = 0;
	}

	while(1) {
		if(TMR1 > expected && timer1_flag == 1 && tmo) {
			break; // Timed out.
		}
		
		if(lastFrameType == expectedFrameType) {
			lastFrameType = 0;
			return status;
		}
	}

	return -1;
}

/*int XBAPI_Wait(byte expectedFrame) {
	return XBAPI_WaitTmo(expectedFrame, 0);
}*/

void XBAPI_Transmit(XBeeAddress* address, const unsigned char* data, int length, byte id) {
    //byte frame_length = sizeof(TxFrame); // I highly doubt that the frame will be over 256 bytes
    // long, so for now we'll just use a byte as it saves us space.
	
    apiFrame.tx.start_delimiter = 0x7e;
    apiFrame.tx.length[0] = 0; // Also we'll put this as a zero. (sizeof(TxFrame)-4) >> 8;
    apiFrame.tx.length[1] = sizeof(TxFrame)-4;
    apiFrame.tx.frame_id = id;
    apiFrame.tx.frame_type = API_TRANSMIT_FRAME;
    memcpy(&apiFrame.tx.destination_address, address, sizeof(XBeeAddress));
    apiFrame.tx.reserved = 0xFEFF;
    apiFrame.tx.transmit_options = DEFAULT_TO;
    apiFrame.tx.broadcast_radius = 0;
    memcpy(&apiFrame.tx.packet, data, ((unsigned)length>sizeof(Packet)) ? sizeof(Packet) : length);
    apiFrame.tx.checksum = checksum(apiFrame.buffer+3, sizeof(TxFrame)-4);

    UART_TransmitMsg((byte*)&apiFrame, sizeof(TxFrame), 0);

	/*
    int error = 0;
    if(id) {
        error = XBAPI_HandleFrame(NULL, API_TRANSMIT_STATUS);
        if(error == XBEE_TIMEOUT_OCCURRED) {
            // Reset the xbee.
            XBee_Reset();
        }
    }
    
    return error;*/
}

uint32 swap_endian_32(uint32 n) {
    unsigned char* p_n = (unsigned char*) &n;
    unsigned char p_ret[4];
	//unsigned char tmp;
    p_ret[0] = p_n[3];
    p_ret[1] = p_n[2];
    p_ret[2] = p_n[1];
    p_ret[3] = p_n[0];

    return *((uint32*)p_ret);
}

/*u32 swap_endian_32(u32 n) {
    unsigned long r = 0;
    r |= (n&0xFF) << 24;
    r |= ((n>>8)&0xFF) << 16;
    r |= ((n>>16)&0xFF) << 8;
    r |= ((n>>24)&0xFF);

    return r;
}*/

byte calc_checksum;
void XBAPI_Command(unsigned short command, unsigned long data, byte data_valid) {

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

	// We should only handle one at a time.
	//byte id = XBAPI_RegisterPacket();
	byte id = 0x1;

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

    /*if(id) {
        return XBAPI_HandleFrame(NULL, API_AT_CMD_RESPONSE);
    } else {
        return 0;
    }*/
	
    // TODO:  Add checksum verification and a way to discard bytes that aren't in a frame.
}

char XBAPI_ReadFrame(Frame* frame) {

	int bytes_read = UART_ReceiveMsg(frame->buffer, 3, 0);
	if(bytes_read<3) {
		return XBEE_TIMEOUT_OCCURRED;
	}

	unsigned short received_length = frame->rx.length[1];
	received_length |= (frame->rx.length[1] << 8);

	UART_ReceiveMsg(frame->buffer + 3, received_length + 1, 0);

	return 0;
}

int XBAPI_HandleFrameIfValid(Frame* frame, int length) {
	int frameLength = frame->rx.length[1] | (frame->rx.length[0] << 8);
	if(frame->rx.start_delimiter == API_START_DELIM && frameLength + 4 <= length) {
		return XBAPI_HandleFrame(frame);
	} else {
		return -2;
	}
}
// Returns the frame type of the handled frame.
byte XBAPI_HandleFrame(Frame* frame) {
	// Non-reentrant code, because *frame may be global.
	// This is horribly, horribly messy, but that's what happens
	// in MCUs that have 28KB of program space and 2KB of data space.
	//DisableInterrupts();

	if(frame == NULL) {
		frame = &apiFrame;

		while(1) {
			XBAPI_ReadFrame(frame);

			XBAPI_HandleFrame(frame);
		}
	}
	
	// TODO:  Add verification of frames

	switch(frame->rx.frame_type) { // It's all the same frame type so it doesn't really matter which struct in the union I choose to access it.
		case API_RX_INDICATOR: {
			Packet* packet = &frame->rx.packet;
			
			switch(packet->header.command) {
				case RECEIVER_ACK: {
					memset(dest_address.addr, 0, sizeof(XBeeAddress));
					memcpy(dest_address.addr+1, &(frame->rx.source_address), sizeof(XBeeAddress_7Bytes));
					memcpy(&eepromData.receiverAddress, &dest_address, sizeof(XBeeAddress));

					byte i;
					for(i=0; i<sizeof(SensorId); i++) {
					    if(eepromData.sensorId.id[i] != 0xFF) {
					    	break;
					    }
					}

					// This means that sensorId is invalid, so we have to load the proper one, and save it to EEPROM.
					if(i >= 8) {
						memcpy(&eepromData.sensorId, &packet->header.sensorId, sizeof(SensorId));
					}

					// Write everything to eeprom.
					EEPROM_Write(0, (byte*)&eepromData, sizeof(EEPROM_Structure));

						// These nodes will only need to ever reply to one address,
						// allowing us to just use dest_address for this purpose.
						// TODO:  change dest_address to receiver_address.
				} break;
			}

			status = 0;
			lastFrameType = API_RX_INDICATOR;
		} break;

        case API_TRANSMIT_STATUS: {
            // TODO:  Add error handling code here.
			status = frame->txStatus.delivery_status;
			lastFrameType = frame->txStatus.frame_type;
        } break;

        case API_AT_CMD_RESPONSE: {
			status = frame->atCmdResponse.cmd_status;
			lastFrameType = frame->atCmdResponse.frame_type;

			int length = (frame->atCmdResponse.length[0] << 8) | frame->atCmdResponse.length[1];
			
			unsigned char dataSize = length - (sizeof(ATCmdResponse) - 8);
			switch(dataSize) {
				case 1:
					userData = frame->atCmdResponse.data[0];
					break;
				case 2:
					userData = (frame->atCmdResponse.data[0] << 8) | frame->atCmdResponse.length[1];
					break;
			}
        } break;

        default:
			status = -1;
			lastFrameType = frame->rx.frame_type;
            break;
    }

	//EnableInterrupts();
    return 0;
}

void SetXBeeBroadcastAddress(XBeeAddress* addr) {
	memset(addr, 0x00, sizeof(XBeeAddress)-2);
	memset(&addr->addr[6], 0xFF, 2);
}