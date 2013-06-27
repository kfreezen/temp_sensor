;----------------------------------------------------------------------
; PIC CRC16 Implementation with Restricted Table Size
;
; Digital Nemesis Pty Ltd
; www.digitalnemesis.com
;
; Original Code: Ashley Roll
; Optimisations: Scott Dattalo
;----------------------------------------------------------------------

	list      p=16lf1783           ; list directive to define processor
	#include <pic16lf1783.inc>       ; processor specific variable definitions

;----------------------------------------------------------------------
; Program Variables
; Note that we start at RAM location 0x07
;----------------------------------------------------------------------
	CBLOCK
		CRC16_High			; The CRC Register
		CRC16_Low

		Index				; Temp registers used during CRC update, can
		Temp				; be reused when not calling CRC_Update
		Count
		CRC16_MessageByte
	ENDC

;----------------------------------------------------------------------
; CRC16 Lookup Table. This is actually the Low and High lookup tables
; conconated together to save a few words of ROM space.
;
; To access the Low table, Enter with 0 <= W <= 15
; To access the High table, Enter with 16 <= W <= 31
;
; This can easily be achieved by setting or clearing bit 4.
CRC16_Lookup:
	ANDLW 0x1F
	ADDWF PCL, F

	RETLW 0x00		; LOW Byte Data
	RETLW 0x21
	RETLW 0x42
	RETLW 0x63
	RETLW 0x84
	RETLW 0xA5
	RETLW 0xC6
	RETLW 0xE7
	RETLW 0x08
	RETLW 0x29
	RETLW 0x4A
	RETLW 0x6B
	RETLW 0x8C
	RETLW 0xAD
	RETLW 0xCE
	RETLW 0xEF
	RETLW 0x00		; HIGH Byte DATA
	RETLW 0x10
	RETLW 0x20
	RETLW 0x30
	RETLW 0x40
	RETLW 0x50
	RETLW 0x60
	RETLW 0x70
	RETLW 0x81
	RETLW 0x91
	RETLW 0xA1
	RETLW 0xB1
	RETLW 0xC1
	RETLW 0xD1
	RETLW 0xE1
	RETLW 0xF1

;----------------------------------------------------------------------
; CRC_Init Subroutine
CRC_Init:
	MOVLW	0xFF
	MOVWF	CRC16_High
	MOVWF	CRC16_Low
	RETLW	0

;----------------------------------------------------------------------
; CRC_Update Subroutine
CRC_Update:
	; Save the Message Byte in the W register
	MOVWF	CRC16_MessageByte

	; We need to perform two iterations each processing 4 bits from the
	; CRC16_MessageByte register. MSB first.
	MOVLW	2
	MOVWF	Count

CRC16_UpdateLoop:
	; Get the top 4 bits from the message byte and XOR it with the
	; top 4 bits extracted from the CRC register to generate the lookup index.
	; Note that on the second iteration the nibbles in the message byte
	; will have been swaped again so we are actually getting the low
	; nibble of the message byte
	MOVF	CRC16_High, W
	XORWF	CRC16_MessageByte, W
	ANDLW	0xF0
	MOVWF	Index			; Index = (CRC16_High ^ CRC16_MessageByte) & 0xF0

	; Shift the CRC Register left 4 bits
	MOVLW	0x0f
	ANDWF	CRC16_High, F
	SWAPF	CRC16_High, F
	SWAPF	CRC16_Low, F
	ANDWF	CRC16_Low, W
	IORWF	CRC16_High, F	; CRC16_High = (CRC16_High << 4) | (CRC16_Low >> 4)
	XORWF	CRC16_Low, F	; CRC16_Low = CRC16_Low << 4

	; Do the table lookups and XOR into the CRC Registers
	SWAPF	Index, W
	CALL	CRC16_Lookup
	XORWF	CRC16_Low, F	; CRC16_Low = CRC16_Low ^ CRC16_LookupLow[t]

	SWAPF	Index, W
	IORLW	0x10			; Access High Table
	CALL	CRC16_Lookup
	XORWF	CRC16_High, F	; CRC16_High = CRC16_High ^ CRC16_LookupHigh[t]

	; Swap the nibbles in the message byte so that next iteration we do the
	; low nibble
	SWAPF	CRC16_MessageByte, F

	; Check if we need to iterate again
	DECFSZ	Count, F
	GOTO	CRC16_UpdateLoop
	RETLW	0				; return Finished

;----------------------------------------------------------------------
; Beginnig of Main Program
;----------------------------------------------------------------------
BeginProgram:

	; Initialise the CRC registers
	CALL	CRC_Init

	; Test Vector: "123456789"
	; Result: 0x29B1
	MOVLW	'1'
	CALL	CRC_Update
	MOVLW	'2'
	CALL	CRC_Update
	MOVLW	'3'
	CALL	CRC_Update
	MOVLW	'4'
	CALL	CRC_Update
	MOVLW	'5'
	CALL	CRC_Update
	MOVLW	'6'
	CALL	CRC_Update
	MOVLW	'7'
	CALL	CRC_Update
	MOVLW	'8'
	CALL	CRC_Update
	MOVLW	'9'
	CALL	CRC_Update