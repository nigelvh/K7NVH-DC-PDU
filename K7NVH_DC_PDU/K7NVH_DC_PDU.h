/* (c) 2015 Nigel Vander Houwen */

#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/power.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <string.h>
#include <stdio.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>

#include "Descriptors.h"

#include <LUFA/Drivers/USB/USB.h>
#include <LUFA/Platform/Platform.h>

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

// Standard file stream for the CDC interface when set up, so that the virtual CDC COM port can be used like any regular character stream in the C APIs.
static FILE USBSerialStream;

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

// EEPROM Offsets
#define EEPROM_OFFSET_PORT_DEFAULTS 0 // Eight bytes at offset 0
#define EEPROM_OFFSET_REF_V 8 // Four bytes at offset 8
#define EEPROM_OFFSET_P8_SENSE 12 // One byte at offset 12

#define EEPROM_OFFSET_P0NAME 64 // 16 bytes at offset 64
#define EEPROM_OFFSET_P1NAME 80 // 16 bytes at offset 80
#define EEPROM_OFFSET_P2NAME 96 // 16 bytes at offset 96
#define EEPROM_OFFSET_P3NAME 112 // 16 bytes at offset 112
#define EEPROM_OFFSET_P4NAME 128 // 16 bytes at offset 128
#define EEPROM_OFFSET_P5NAME 144 // 16 bytes at offset 144
#define EEPROM_OFFSET_P6NAME 160 // 16 bytes at offset 160
#define EEPROM_OFFSET_P7NAME 176 // 16 bytes at offset 176

static inline void run_lufa(void);

static inline void SPI_begin(void);
static inline void SPI_end(void);
static inline uint8_t SPI_transfer(uint8_t _data);
//void SPI_attachInterrupt(void);
//void SPI_detachInterrupt(void);
static inline void SPI_setBitOrder(uint8_t bitOrder);
static inline void SPI_setDataMode(uint8_t mode);
static inline void SPI_setClockDivider(uint8_t rate);

static inline void LED_CTL(uint8_t led, uint8_t state);
static inline uint8_t PORT_CTL(uint8_t port, uint8_t state);

static inline void EEPROM_Read_Port_Defaults(void);
static inline void EEPROM_Write_Port_Defaults(void);
static inline void EEPROM_Read_REF_V(void);
static inline void EEPROM_Write_REF_V(void);
static inline void EEPROM_Read_P8_Sense(void);
static inline void EEPROM_Write_P8_Sense(uint8_t mode);
static inline const char * EEPROM_Read_Port_Name(uint8_t port);
static inline void EEPROM_Dump_Vars(void);

static inline float ADC_Read_Current(uint8_t port);
static inline float ADC_Read_Voltage(void);
static inline uint16_t ADC_Read_Raw(uint8_t port);

static inline void printPGMStr(PGM_P s);
static inline void PRINT_Status(void);

static inline void INPUT_Clear(void);
static inline void INPUT_Parse(void);