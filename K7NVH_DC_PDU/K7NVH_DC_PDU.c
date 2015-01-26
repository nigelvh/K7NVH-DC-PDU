/* (c) 2014 Nigel Vander Houwen */
//
// TODO
// Save PCYCLE delay to EEPROM
// Implement PCYCLE
// Finish VSETREF implementation
// Finish PSETNAME implementation
// HIGH current safety shutoff
// Save current limit value/port to EEPROM
// High water mark stored in EEPROM (Lifetime & User resettable)

#include "K7NVH_DC_PDU.h"

#define SOFTWAREVERS "\r\nK7NVH DC PDU V1.0\r\n"

// Reused strings
const char STR_NR_Port[] PROGMEM = "\r\nPORT ";
const char STR_Enabled[] PROGMEM = "ENABLED";
const char STR_Disabled[] PROGMEM = "DISABLED";
const char STR_Port_Init[] PROGMEM = "PORT INIT:\r\n";
const char STR_Port_Default[] PROGMEM = "\r\nPORT DEFAULT ";
const char STR_Port_8_Sense[] PROGMEM = "\r\nPORT 8 SENSE ";

// Variables stored in EEPROM
uint8_t PORT_DEF[8]; // Default state for the ports
float REF_V;
uint8_t PORT8_SENSE; // 0 = Current, 1 = Voltage

// Port to ADC Address look up table
const uint8_t ADC_Ports[8] = \
		{0b10010000, 0b10000000, 0b10110000, 0b10100000, \
		 0b11010000, 0b11000000, 0b11110000, 0b11100000};
float STEP_V = 0; // Will be set at startup.

// State Variables
uint8_t PORT_STATE[8];
char DATA_IN[32];
uint8_t DATA_IN_POS = 0;

/** LUFA CDC Class driver interface configuration and state information. This structure is
 *  passed to all CDC Class driver functions, so that multiple instances of the same class
 *  within a device can be differentiated from one another.
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

// Main program entry point.
int main(void) {
	// Read EEPROM stored variables
	EEPROM_Read_Port_Defaults();
	EEPROM_Read_REF_V();
	EEPROM_Read_P8_Sense();

	// Initialize some variables
	STEP_V = REF_V / 1024;
	int16_t BYTE_IN = -1;

	for(uint8_t i = 0; i < 32; i++) {
		DATA_IN[i] = 0;
	}

	// Disable watchdog if enabled by bootloader/fuses
	MCUSR &= ~(1 << WDRF);
	wdt_disable();

	// Disable clock division
	clock_prescale_set(clock_div_16);

	// USB Hardware Initialization
	USB_Init();

	// Create a regular character stream for the interface 
	// so that it can be used with the stdio.h functions
	CDC_Device_CreateStream(&VirtualSerial_CDC_Interface, &USBSerialStream);

	// Enable interrupts
	GlobalInterruptEnable();

	run_lufa();

	for(uint16_t i = 0; i < 500; i++) {
		_delay_ms(10);
	}

	// Print startup message
	printPGMStr(PSTR(SOFTWAREVERS));
	run_lufa();

	// Set up SPI pins
	DDRB |= (1 << SPI_SS)|(1 << SPI_SCK)|(1 << SPI_MOSI);
	PORTB |= (1 << SPI_SS);
	PORTB &= ~(1 << SPI_SCK);

	// Start up SPI
	SPI_begin();
	SPI_setClockDivider(SPI_CLOCK_DIV128);
	SPI_setDataMode(SPI_MODE0);
	SPI_setBitOrder(1);

	// Set up control pins
	DDRB |= (1 << P1EN)|(1 << P2EN)|(1 << P3EN)|(1 << P4EN);
	DDRC |= (1 << P5EN)|(1 << P6EN)|(1 << P7EN)|(1 << P8EN);
	DDRD |= (1 << LED1)|(1 << LED2);

	// Enable/Disable ports per their defaults
	printPGMStr(STR_Port_Init);
	for(uint8_t i = 0; i < 8; i++) {
		PORT_CTL(i, PORT_DEF[i]);
		fprintf(&USBSerialStream, "P%i", i+1);
		if (PORT_DEF[i]) {
			printPGMStr(PSTR(" ON\r\n"));
		} else {
			printPGMStr(PSTR(" OFF\r\n"));
		}
		run_lufa();
	}

	INPUT_Clear();

	for(;;) {
		BYTE_IN = CDC_Device_ReceiveByte(&VirtualSerial_CDC_Interface);

		if (BYTE_IN >= 0) {
			LED_CTL(1, 1);
			fputc(BYTE_IN, &USBSerialStream);

			switch(BYTE_IN) {
				case 8:
				case 127:
					// Backspace
					if (DATA_IN_POS > 0) DATA_IN_POS--;
					DATA_IN[DATA_IN_POS] = 0;
					break;

				case '\n':
				case '\r':
					// Newline, Parse our command
					INPUT_Parse();
					INPUT_Clear();
					break;

				default:
					// Normal char buffering
					if (DATA_IN_POS < 31) {
						DATA_IN[DATA_IN_POS] = BYTE_IN;
						DATA_IN_POS++;
					} else {
						// Input is too long.
						// TODO: print message that command is too long.
						INPUT_Clear();
					}
					break;
			}
		}

		LED_CTL(1, 0);
		run_lufa();
	}
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~ Command Parsing Functions
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

static inline void INPUT_Clear(void) {
	for (uint8_t i = 0; i < 32; i++) {
		DATA_IN[i] = 0;
	}
	DATA_IN_POS = 0;
	printPGMStr(PSTR("\r\n\r\n> "));
}

static inline void INPUT_Parse(void) {
	if (strcmp_P(DATA_IN, PSTR("STATUS")) == 0) {
		PRINT_Status();
		return;
	}
	if (strcmp_P(DATA_IN, PSTR("EEPROMDUMP")) == 0) {
		EEPROM_Dump_Vars();
		return;
	}
	if (strncmp_P(DATA_IN, PSTR("PON"), 3) == 0) {
		if (DATA_IN[3] >= 49 && DATA_IN[3] <= 56) {
			PORT_CTL(DATA_IN[3]-48-1, 1);
			printPGMStr(STR_NR_Port);
			printPGMStr(STR_Enabled);
			return;
		} else if (DATA_IN[3] == 'A') {
			for (uint8_t i = 0; i < 8; i++) {
				PORT_CTL(i, 1);
				printPGMStr(STR_NR_Port);
				fprintf(&USBSerialStream, "%i ", i);
				printPGMStr(STR_Enabled);
			}
			return;
		}
	}
	if (strncmp_P(DATA_IN, PSTR("POFF"), 4) == 0) {
		if (DATA_IN[4] >= 49 && DATA_IN[4] <= 56) {
			PORT_CTL(DATA_IN[4]-48-1, 0);
			printPGMStr(STR_NR_Port);
			printPGMStr(STR_Disabled);
			return;
		} else if (DATA_IN[4] == 'A') {
			for(uint8_t i = 0; i < 8; i++) {
				PORT_CTL(i, 0);
				printPGMStr(STR_NR_Port);
				fprintf(&USBSerialStream, "%i ", i);
				printPGMStr(STR_Disabled);
			}
			return;
		}
	}
	if (strncmp_P(DATA_IN, PSTR("PDEFON"), 6) == 0) {
		if (DATA_IN[6] >= 49 && DATA_IN[6] <= 56) {
			PORT_DEF[DATA_IN[6]-48-1] = 1;
			EEPROM_Write_Port_Defaults();
			printPGMStr(STR_Port_Default);
			printPGMStr(STR_Enabled);
			return;
		} else if (DATA_IN[6] == 'A') {
			for (uint8_t i = 0; i < 8; i++) {
				PORT_DEF[i] = 1;
				printPGMStr(STR_Port_Default);
				fprintf(&USBSerialStream, "%i ", i);
				printPGMStr(STR_Enabled);
			}
			EEPROM_Write_Port_Defaults();
			return;
		}
	}
	if (strncmp_P(DATA_IN, PSTR("PDEFOFF"), 7) == 0) {
		if (DATA_IN[7] >= 49 && DATA_IN[7] <= 56) {
			PORT_DEF[DATA_IN[7]-48-1] = 0;
			EEPROM_Write_Port_Defaults();
			printPGMStr(STR_Port_Default);
			printPGMStr(STR_Disabled);
			return;
		} else if (DATA_IN[7] == 'A') {
			for(uint8_t i = 0; i < 8; i++) {
				PORT_DEF[i] = 0;
				printPGMStr(STR_Port_Default);
				fprintf(&USBSerialStream, "%i ", i);
				printPGMStr(STR_Disabled);
			}
			EEPROM_Write_Port_Defaults();
			return;
		}
	}
	if (strncmp_P(DATA_IN, PSTR("P8SENSEV"), 8) == 0) {
		EEPROM_Write_P8_Sense(1);
		printPGMStr(STR_Port_8_Sense);
		printPGMStr(PSTR("VOLTAGE"));
		return;
	}
	if (strncmp_P(DATA_IN, PSTR("P8SENSEI"), 8) == 0) {
		EEPROM_Write_P8_Sense(0);
		printPGMStr(STR_Port_8_Sense);
		printPGMStr(PSTR("CURRENT"));
		return;
	}
	if (strncmp_P(DATA_IN, PSTR("SETVREF"), 7) == 0) {
		if (DATA_IN_POS > 7) {
			char temp_str[5];
			strncpy(temp_str, DATA_IN+7, 4);
			temp_str[4] = '\0';
			uint16_t temp_int = atoi(temp_str);
			fprintf(&USBSerialStream, "\r\n%s %i %i", temp_str, DATA_IN_POS, temp_int);
			return;
		}
	}
	printPGMStr(PSTR("\r\nUNRECOGNIZED COMMAND"));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~ Printing Functions
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

static inline void PRINT_Status(void) {
	float voltage, current;
	if (PORT8_SENSE == 1) {
		voltage = ADC_Read_Voltage();
		printPGMStr(PSTR("\r\nInput Voltage: "));
		fprintf(&USBSerialStream, "%.2fV", voltage);
	}
	for(uint8_t i = 0; i < 8; i++) {
		printPGMStr(STR_NR_Port);
		fprintf(&USBSerialStream, "%i \"%s\": ", i+1, EEPROM_Read_Port_Name(i));
		if (PORT_STATE[i] == 1) { printPGMStr(PSTR("ON")); } else { printPGMStr(PSTR("OFF")); }
		if (i == 7 && PORT8_SENSE == 1) break;
		current = ADC_Read_Current(i);
		printPGMStr(PSTR(". Current: "));
		fprintf(&USBSerialStream, "%.2f", current);
		if (PORT8_SENSE == 1) {
			printPGMStr(PSTR("A, Power: "));
			fprintf(&USBSerialStream, "%.1fW", (voltage * current));
		}
	}
}

static inline void printPGMStr(PGM_P s) {
	char c;
	while((c = pgm_read_byte(s++)) != 0) fputc(c, &USBSerialStream);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~ Port/LED Control Functions
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

static inline uint8_t PORT_CTL(uint8_t port, uint8_t state) {
	// If state isn't on or off, or the port isn't a valid number, return error
	if (state != 0 && state != 1) return 1;
	if (port < 0 || port > 7) return 1;

	// Ports 0 through 3
	if (port >= 0 && port <= 3) {
		if (state == 1) {
			PORTB |= (1 << (4 + port));
		} else {
			PORTB &= ~(1 << (4 + port));
		}
		// Ports 4 through 7
	} else {
		if (state == 1) {
			PORTC |= (1 << (11 - port));
		} else {
			PORTC &= ~(1 << (11 - port));
		}
	}

	// Update our port state array
	PORT_STATE[port] = state;

	return 0;
}

static inline void LED_CTL(uint8_t led, uint8_t state) {
	if (led == 1) {
		if (state == 1) {
			PORTD |= (1 << LED1);
		} else {
			PORTD &= ~(1 << LED1);
		}
	} else {
		if (state == 1) {
			PORTD |= (1 << LED2);
		} else {
			PORTD &= ~(1 << LED2);
		}
	}
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~ EEPROM Functions
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ 

// Read the default port state settings into the PORT_DEF array in RAM
static inline void EEPROM_Read_Port_Defaults(void) {
	for(uint8_t i = 0; i < 8; i++) {
		// Update the PORT_DEF array with the values from EEPROM
		PORT_DEF[i] = eeprom_read_byte((uint8_t*)(EEPROM_OFFSET_PORT_DEFAULTS + i));
		// If the value is not 0 or 1 (uninitialized), default it to 1
		if (PORT_DEF[i] < 0 || PORT_DEF[i] > 1) PORT_DEF[i] = 1;
	}
}

// Write the default port state settings from the PORT_DEF array in RAM
static inline void EEPROM_Write_Port_Defaults(void) {
	for(uint8_t i = 0; i < 8; i++) {
		// Update the EERPOM with the values from the PORT_DEF array
		eeprom_update_byte((uint8_t*)(EEPROM_OFFSET_PORT_DEFAULTS + i), PORT_DEF[i]);
	}
}

// Read the stored reference voltage from EEPROM
static inline void EEPROM_Read_REF_V(void) {
	REF_V = eeprom_read_float((float*)(EEPROM_OFFSET_REF_V));
	// If the value seems out of range (uninitialized), default it to 4.2
	//float REF_V = 4.227;
	if (REF_V < 4.0 || REF_V > 4.4 || isnan(REF_V)) REF_V = 4.2;
}

// Write the reference voltage to EEPROM
static inline void EEPROM_Write_REF_V(void) {
	eeprom_update_float((float*)(EEPROM_OFFSET_REF_V), REF_V);
}

// Write the Port 8 Sense mode to EEPROM
static inline void EEPROM_Write_P8_Sense(uint8_t mode) {
	eeprom_update_byte((uint8_t*)(EEPROM_OFFSET_P8_SENSE), mode);
	PORT8_SENSE = mode;
}

// Read the stored Port 8 Sense mode
static inline void EEPROM_Read_P8_Sense(void) {
	PORT8_SENSE = eeprom_read_byte((uint8_t*)(EEPROM_OFFSET_P8_SENSE));
	if (PORT8_SENSE < 0 || PORT8_SENSE > 1) PORT8_SENSE = 0;
}

// Read the stored port name
static inline const char * EEPROM_Read_Port_Name(uint8_t port) {
	char working[16];
	eeprom_read_block((void*)working, (const void*)EEPROM_OFFSET_P0NAME+(port*16), 16);
	if (working[0] >= 127 || working[0] < 32) { memset(working,0,sizeof(working)); }
	return working;
}

// Dump all EEPROM variables
static inline void EEPROM_Dump_Vars(void) {
	// Read port defaults
	printPGMStr(PSTR("\r\nPORT DEF: "));
	for(uint8_t i = 0; i < 8; i++) {
		fprintf(&USBSerialStream, "%i ", eeprom_read_byte((uint8_t*)(EEPROM_OFFSET_PORT_DEFAULTS + i)));
	}
	// Read REF_V
	printPGMStr(PSTR("\r\nREF_V: "));
	fprintf(&USBSerialStream, "%.2f %.2f", eeprom_read_float((float*)(EEPROM_OFFSET_REF_V)), REF_V);
	// Read P8_SENSE
	printPGMStr(PSTR("\r\nP8SENSE: "));
	fprintf(&USBSerialStream, "%i", eeprom_read_byte((uint8_t*)(EEPROM_OFFSET_P8_SENSE)));
	// Read Port Names
	printPGMStr(PSTR("\r\nPNAMES: "));
	for(uint8_t i = 0; i < 8; i++) {
		char working[16];
		eeprom_read_block((void*)working, (const void*)EEPROM_OFFSET_P0NAME+(i*16), 16);
		fprintf(&USBSerialStream, "%s ", working);
	}
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~ ADC Functions
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Read current flow on a given port
static inline float ADC_Read_Current(uint8_t port) {
	float voltage = (ADC_Read_Raw(port) * STEP_V) / 7.8;
	if (voltage < 0.002) voltage = 0.0; // Ignore the lowest voltages so we don't falsely say there's current where there isn't.
	return (voltage / 0.1);
}

// Read input voltage on ADC channel 8 if not measuring current
static inline float ADC_Read_Voltage(void) {
	return ((ADC_Read_Raw(7)* STEP_V) * 10.1);
}

// Return raw counts from the ADC
static inline uint16_t ADC_Read_Raw(uint8_t port) {
	PORTB &= ~(1 << SPI_SS);
	SPI_transfer(0x01); // Start bit
	uint8_t temp1 = SPI_transfer(ADC_Ports[port]); // Single ended, input number, clocking in 4 bits
	uint8_t temp2 = SPI_transfer(0x00); // Clocking in 8 bits
	PORTB |= (1 << SPI_SS);

	uint16_t adc_counts = ((temp1 & 0b00000011) << 8) | temp2;

	return adc_counts;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~ SPI Functions
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

static inline void SPI_begin(void) {
	// Set SS to high so a connected chip will be "deselected" by default
	// Set SS to output

	SPCR |= _BV(MSTR);
	SPCR |= _BV(SPE);

	// Set output for SCK and MOSI pin
}

static inline void SPI_end(void) {
	SPCR &= ~_BV(SPE);
}

static inline uint8_t SPI_transfer(uint8_t _data) {
	SPDR = _data;
	while(!(SPSR & _BV(SPIF))) {};
	return SPDR;
}

//void SPI_attachInterrupt(void) {
//  SPCR |= _BV(SPIE);
//}

//void SPI_detachInterrupt(void) {
//  SPCR &= ~_BV(SPIE);
//}

// 0 = LSBFIRST
static inline void SPI_setBitOrder(uint8_t bitOrder) {
	if (bitOrder == 0) {
		SPCR |= _BV(DORD);
	} else {
		SPCR &= ~(_BV(DORD));
	}
}

static inline void SPI_setDataMode(uint8_t mode) {
	SPCR = (SPCR & ~SPI_MODE_MASK) | mode;
}

static inline void SPI_setClockDivider(uint8_t rate) {
	SPCR = (SPCR & ~SPI_CLOCK_MASK) | (rate & SPI_CLOCK_MASK);
	SPSR = (SPSR & ~SPI_2XCLOCK_MASK) | ((rate >> 2) & SPI_2XCLOCK_MASK);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~ USB Functions
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

static inline void run_lufa(void) {
	//CDC_Device_ReceiveByte(&VirtualSerial_CDC_Interface);
	CDC_Device_USBTask(&VirtualSerial_CDC_Interface);
	USB_USBTask();
}

// Event handler for the library USB Connection event.
void EVENT_USB_Device_Connect(void) {
	// Turn on the first LED to indicate we're enumerated.
	//PORTD = PORTD & 0b11111110;
}

// Event handler for the library USB Disconnection event.
void EVENT_USB_Device_Disconnect(void) {
	// Turn off the first LED to indicate we're not enumerated.
	//PORTD = PORTD | 0b00000001;
}

// Event handler for the library USB Configuration Changed event.
void EVENT_USB_Device_ConfigurationChanged(void) {
	bool ConfigSuccess = true;
	ConfigSuccess &= CDC_Device_ConfigureEndpoints(&VirtualSerial_CDC_Interface);

	// Set the second LED to indicate USB is ready or not.
	//if (ConfigSuccess) {
	//PORTD = PORTD & 0b11111101;
	//} else {
	//PORTD = PORTD | 0b00000010;
	//}
}

// Event handler for the library USB Control Request reception event.
void EVENT_USB_Device_ControlRequest(void) {
	CDC_Device_ProcessControlRequest(&VirtualSerial_CDC_Interface);
}
