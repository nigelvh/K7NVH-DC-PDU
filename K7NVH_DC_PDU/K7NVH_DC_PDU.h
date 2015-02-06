/* (c) 2015 Nigel Vander Houwen */
#ifndef _K7NVH_DC_PDU_H_
#define _K7NVH_DC_PDU_H_

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~ Includes
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/power.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>

#include <LUFA/Drivers/USB/USB.h>
#include <LUFA/Platform/Platform.h>

#include "Descriptors.h"

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~ Macros
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Enable ANSI color codes to be sent. Uses a small bit of extra program space for 
// storage of color codes/modified strings.
#define ENABLECOLORS

#define SOFTWAREVERS "\r\nK7NVH DC PDU V1.0\r\n"
#define PORT_CNT    8
#define DATA_BUFF_LEN    32

#define SPI_CLOCK_DIV4 0x00
#define SPI_CLOCK_DIV16 0x01
#define SPI_CLOCK_DIV64 0x02
#define SPI_CLOCK_DIV128 0x03
#define SPI_CLOCK_DIV2 0x04
#define SPI_CLOCK_DIV8 0x05
#define SPI_CLOCK_DIV32 0x06

#define SPI_MODE0 0x00
#define SPI_MODE1 0x04
#define SPI_MODE2 0x08
#define SPI_MODE3 0x0C

#define SPI_MODE_MASK 0x0C  // CPOL = bit 3, CPHA = bit 2 on SPCR
#define SPI_CLOCK_MASK 0x03  // SPR1 = bit 1, SPR0 = bit 0 on SPCR
#define SPI_2XCLOCK_MASK 0x01  // SPI2X = bit 0 on SPSR

// SPI pins
#define SPI_SS PB0
#define SPI_SCK PB1
#define SPI_MOSI PB2
#define SPI_MISO PB3

// Output port controls
#define P1EN PB4
#define P2EN PB5
#define P3EN PB6
#define P4EN PB7
#define P5EN PC7
#define P6EN PC6
#define P7EN PC5
#define P8EN PC4

// LED1 = Green, LED2 = Red
#define LED1 PD4
#define LED2 PD5

// Limits
#define PCYCLE_MAX_TIME 30 // Seconds
#define VREF_MAX 4400 // 4.4V * 1000
#define VREF_MIN 4000 // 4.0V * 1000
#define LIMIT_MAX 100 // Stored as amps*10 so 50==5.0A

// EEPROM Offsets
#define EEPROM_OFFSET_PORT_DEFAULTS 0 // 8 bytes at offset 0
#define EEPROM_OFFSET_REF_V 8 // 4 bytes at offset 8
#define EEPROM_OFFSET_P8_SENSE 12 // 1 byte at offset 12
#define EEPROM_OFFSET_CYCLE_TIME 13 // 1 byte at offset 13

#define EEPROM_OFFSET_P0LIMIT 16 // 8 Bytes at offset 16

#define EEPROM_OFFSET_P0NAME 64 // 16 bytes at offset 64
#define EEPROM_OFFSET_P1NAME 80 // 16 bytes at offset 80
#define EEPROM_OFFSET_P2NAME 96 // 16 bytes at offset 96
#define EEPROM_OFFSET_P3NAME 112 // 16 bytes at offset 112
#define EEPROM_OFFSET_P4NAME 128 // 16 bytes at offset 128
#define EEPROM_OFFSET_P5NAME 144 // 16 bytes at offset 144
#define EEPROM_OFFSET_P6NAME 160 // 16 bytes at offset 160
#define EEPROM_OFFSET_P7NAME 176 // 16 bytes at offset 176

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~ Globals
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

typedef uint8_t pd_set; // Port Descriptor Set - bitmap of ports

// Port State Set - bitmap of port state
// (NUL,NUL,NUL,NUL,NUL,NUL,Overload,Enabled/Disabled)
typedef uint8_t ps_set; 

// Standard file stream for the CDC interface when set up, so that the
// virtual CDC COM port can be used like any regular character stream
// in the C APIs.
static FILE USBSerialStream;

// Reused strings
#ifdef ENABLECOLORS
//	const char STR_Color_Red[] PROGMEM = "\x1b[31m";
//	const char STR_Color_Green[] PROGMEM = "\x1b[32m";
//	const char STR_Color_Blue[] PROGMEM = "\x1b[34m";
//	const char STR_Color_Cyan[] PROGMEM = "\x1b[36m";
//	const char STR_Color_Reset[] PROGMEM = "\x1b[0m";
	const char STR_Unrecognized[] PROGMEM = "\r\n\x1b[31mINVALID COMMAND\x1b[0m";
	const char STR_Enabled[] PROGMEM = "\x1b[32mENABLED\x1b[0m";
	const char STR_Disabled[] PROGMEM = "\x1b[31mDISABLED\x1b[0m";
	const char STR_Overload[] PROGMEM = "\x1b[31m!OVERLOAD!\x1b[0m";
	const char STR_Prompt[] PROGMEM = "\r\n\r\n\x1b[36m>\x1b[0m ";
#else
	const char STR_Unrecognized[] PROGMEM = "\r\nINVALID COMMAND";
	const char STR_Enabled[] PROGMEM = "ENABLED";
	const char STR_Disabled[] PROGMEM = "DISABLED";
	const char STR_Overload[] PROGMEM = "!OVERLOAD!";
	const char STR_Prompt[] PROGMEM = "\r\n\r\n> ";
#endif	

const char STR_Backspace[] PROGMEM = "\x1b[D \x1b[D";
const char STR_NR_Port[] PROGMEM = "\r\nPORT ";
const char STR_Port_Default[] PROGMEM = "\r\nPORT DEFAULT ";
const char STR_Port_8_Sense[] PROGMEM = "\r\nPORT 8 SENSE ";
const char STR_PCYCLE_Time[] PROGMEM = "\r\nPCYCLE TIME: ";
const char STR_Port_Limit[] PROGMEM = "\r\nPORT LIMIT: ";
const char STR_VREF[] PROGMEM = "\r\nVREF: ";

// Variables stored in EEPROM
float REF_V; // Stores as volts
uint8_t PCYCLE_TIME; // Seconds

// Port to ADC Address look up table
const uint8_t ADC_Ports[PORT_CNT] = \
		{0b10010000, 0b10000000, 0b10110000, 0b10100000, \
		 0b11010000, 0b11000000, 0b11110000, 0b11100000};
float STEP_V = 0; // Will be set at startup.

// State Variables
ps_set PORT_STATE[PORT_CNT];
char DATA_IN[DATA_BUFF_LEN];
uint8_t DATA_IN_POS = 0;

/** LUFA CDC Class driver interface configuration and state information.
 * This structure is passed to all CDC Class driver functions, so that
 * multiple instances of the same class within a device can be
 * differentiated from one another.
 */ 
USB_ClassInfo_CDC_Device_t VirtualSerial_CDC_Interface = {
	.Config = {
		.ControlInterfaceNumber   = INTERFACE_ID_CDC_CCI,
		.DataINEndpoint           = {
			.Address          = CDC_TX_EPADDR,
			.Size             = CDC_TXRX_EPSIZE,
			.Banks            = 1,
		},
		.DataOUTEndpoint = {
			.Address          = CDC_RX_EPADDR,
			.Size             = CDC_TXRX_EPSIZE,
			.Banks            = 1,
		},
		.NotificationEndpoint = {
			.Address          = CDC_NOTIFICATION_EPADDR,
			.Size             = CDC_NOTIFICATION_EPSIZE,
			.Banks            = 1,
		},
	},
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~ Prototypes
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

static inline void run_lufa(void);

static inline void SPI_begin(void);
static inline void SPI_end(void);
static inline uint8_t SPI_transfer(uint8_t _data);
static inline void SPI_setBitOrder(uint8_t bitOrder);
static inline void SPI_setDataMode(uint8_t mode);
static inline void SPI_setClockDivider(uint8_t rate);

static inline void LED_CTL(uint8_t led, uint8_t state);
static inline void PORT_CTL(uint8_t port, uint8_t state);
static inline void PORT_Set_Ctl(pd_set *pd, uint8_t state);
static inline uint8_t PORT_Check_Current_Limit(uint8_t port);

static inline uint8_t EEPROM_Read_Port_Default(uint8_t port);
static inline void EEPROM_Write_Port_Default(uint8_t port, uint8_t portdef);
static inline void EEPROM_Read_REF_V(void);
static inline void EEPROM_Write_REF_V(float reference);
static inline uint8_t EEPROM_Read_P8_Sense(void);
static inline void EEPROM_Write_P8_Sense(uint8_t mode);
static inline void EEPROM_Read_PCycle_Time(void);
static inline void EEPROM_Write_PCycle_Time(uint8_t time);
static inline void EEPROM_Read_Port_Name(uint8_t port, char *str);
static inline void EEPROM_Write_Port_Name(uint8_t port, char *str);
static inline uint8_t EEPROM_Read_Port_Limit(uint8_t port);
static inline void EEPROM_Write_Port_Limit(uint8_t port, uint8_t limit);
static inline void EEPROM_Dump_Vars(void);

static inline float ADC_Read_Current(uint8_t port);
static inline float ADC_Read_Voltage(void);
static inline uint16_t ADC_Read_Raw(uint8_t port);

static inline void printPGMStr(PGM_P s);
static inline void PRINT_Status(void);

static inline void INPUT_Clear(void);
static inline void INPUT_Parse(void);
static inline void INPUT_Parse_args(pd_set *pd, char *str);

#endif
