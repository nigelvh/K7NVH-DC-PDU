/* (c) 2015 Nigel Vander Houwen */
//
// TODO
// HIGH current safety shutoff
// Save current limit value/port to EEPROM
// High water mark stored in EEPROM (Lifetime & User resettable)
// Document available commands

#include "K7NVH_DC_PDU.h"

// Main program entry point.
int main(void) {
	// Read EEPROM stored variables
	EEPROM_Read_Port_Defaults();
	EEPROM_Read_REF_V();
	EEPROM_Read_P8_Sense();
	EEPROM_Read_PCycle_Time();

	// Initialize some variables
	STEP_V = REF_V / 1024;
	int16_t BYTE_IN = -1;

	// Disable watchdog if enabled by bootloader/fuses
	MCUSR &= ~(1 << WDRF);
	wdt_disable();

	// Divide 16MHz crystal down to 1MHz for CPU clock.
	clock_prescale_set(clock_div_16);

	// Init USB hardware and create a regular character stream for the
	// USB interface so that it can be used with the stdio.h functions
	USB_Init();
	CDC_Device_CreateStream(&VirtualSerial_CDC_Interface, &USBSerialStream);

	// Enable interrupts
	GlobalInterruptEnable();

	run_lufa();

	// Wait 5 seconds so that we can open a console to catch startup messages
	for (uint16_t i = 0; i < 500; i++) {
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
	for(uint8_t i = 0; i < PORT_CNT; i++) {
		PORT_CTL(i, PORT_DEF[i]);
		printPGMStr(STR_NR_Port);
		fprintf(&USBSerialStream, "%i ", i+1);
		if (PORT_DEF[i]) {
			printPGMStr(STR_Enabled);
		} else {
			printPGMStr(STR_Disabled);
		}
		run_lufa();
	}

	INPUT_Clear();

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~ Main system loop
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	for (;;) {
		// Read a byte from the USB serial stream
		BYTE_IN = CDC_Device_ReceiveByte(&VirtualSerial_CDC_Interface);

		// USB Serial stream will return <0 if no bytes are available.
		if (BYTE_IN >= 0) {
			// We've gotten a char, so lets blink the green LED onboard. This LED will 
			// remain lit while the board is processing commands. This is most evident 
			// during a PCYCLE.
			LED_CTL(1, 1);
			
			// Echo the char we just received back out the serial stream so the user's 
			// console will display it.
			fputc(BYTE_IN, &USBSerialStream);

			// Switch on the input byte to determine what is is and what to do.
			switch (BYTE_IN) {
				case 8:
				case 127:
					// Handle Backspace chars.
					if (DATA_IN_POS > 0){
						DATA_IN_POS--;
						DATA_IN[DATA_IN_POS] = 0;
						printPGMStr(STR_Backspace);
					}
					break;

				case '\n':
				case '\r':
					// Newline, Parse our command
					INPUT_Parse();
					INPUT_Clear();
					break;

				case 3:
					// Ctrl-c bail out on partial command
					INPUT_Clear();
					break;

				default:
					// Normal char buffering
					if (DATA_IN_POS < (DATA_BUFF_LEN - 1)) {
						DATA_IN[DATA_IN_POS] = BYTE_IN;
						DATA_IN_POS++;
						DATA_IN[DATA_IN_POS] = 0;
					} else {
						// Input is too long.
						printPGMStr(STR_Unrecognized);
						INPUT_Clear();
					}
					break;
			}
		}

		// Turn the green LED back off again
		LED_CTL(1, 0);
		
		// Keep the LUFA USB stuff fed regularly.
		run_lufa();
	}
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~ Command Parsing Functions
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Flush out our data input buffer, reset our position variable, and print a new prompt.
static inline void INPUT_Clear(void) {
	for (uint8_t i = 0; i < DATA_BUFF_LEN; i++) {
		DATA_IN[i] = 0;
	}
	DATA_IN_POS = 0;
#ifdef ENABLECOLORS
			printPGMStr(STR_Color_Cyan);
#endif
	printPGMStr(PSTR("\r\n\r\n> "));
#ifdef ENABLECOLORS
			printPGMStr(STR_Color_Reset);
#endif
}

// Parse command arguments and return pd_set bitmap with relevant port
// bits set.
static inline void INPUT_Parse_args(pd_set *pd, char *str) {
	*pd = 0;

	while (*str != 0 && str < (DATA_IN + DATA_BUFF_LEN)) {
		if (*str >= '1' && *str <= '8') {
			*pd = *pd | (1 << (*str - '1'));
		} else if (*str == 'A' || *str == 'a') {
			*pd = 0b11111111;
		}
		str++;
	}
}

// We've gotten a new command, parse out what they want.
static inline void INPUT_Parse(void) {
	pd_set pd; // Port descriptor bitmap

	// Print a port status summary for all ports
	if (strcmp_P(DATA_IN, PSTR("STATUS")) == 0) {
		PRINT_Status();
		return;
	}
	// Print a report of the variables stored in EEPROM
	if (strcmp_P(DATA_IN, PSTR("EEPROMDUMP")) == 0) {
		EEPROM_Dump_Vars();
		return;
	}
	// Turn on a port or list of ports
	if (strncmp_P(DATA_IN, PSTR("PON"), 3) == 0) {
		INPUT_Parse_args(&pd, DATA_IN + 3);

		for (uint8_t i = 0; i < PORT_CNT; i++) {
			if (pd & (1 << i)) {
				PORT_CTL(i, 1);
				printPGMStr(STR_NR_Port);
				fprintf(&USBSerialStream, "%i ", i+1);
				printPGMStr(STR_Enabled);
			}
		}

		return;
	}
	// Turn off a port or a list of ports
	if (strncmp_P(DATA_IN, PSTR("POFF"), 4) == 0) {
		INPUT_Parse_args(&pd, DATA_IN + 4);

		for (uint8_t i = 0; i < PORT_CNT; i++) {
			if (pd & (1 << i)) {
				PORT_CTL(i, 0);
				printPGMStr(STR_NR_Port);
				fprintf(&USBSerialStream, "%i ", i+1);
				printPGMStr(STR_Disabled);
			}
		}

		return;
	}
	// Power cycle a port or list of ports. Time is defined by PCYCLE_TIME.
	if (strncmp_P(DATA_IN, PSTR("PCYCLE"), 6) == 0) {
		INPUT_Parse_args(&pd, DATA_IN + 6);
		
		for (uint8_t i = 0; i < PORT_CNT; i++) {
			if (pd & (1 << i)) {
				PORT_CTL(i, 0);
				printPGMStr(STR_NR_Port);
				fprintf(&USBSerialStream, "%i ", i+1);
				printPGMStr(STR_Disabled);
			}
		}
		fputs("\r\n", &USBSerialStream);
		run_lufa();
		for (uint16_t i = 0; i < (PCYCLE_TIME); i++) {
			_delay_ms(1000);
			fputc('.', &USBSerialStream);
			run_lufa();
		}
		for (uint8_t i = 0; i < PORT_CNT; i++) {
			if (pd & (1 << i)) {
				PORT_CTL(i, 1);
				printPGMStr(STR_NR_Port);
				fprintf(&USBSerialStream, "%i ", i+1);
				printPGMStr(STR_Enabled);
			}
		}
		
		return;
	}
	// Set PCYCLE_TIME and store in EEPROM
	if (strncmp_P(DATA_IN, PSTR("SETPCYCLE"), 9) == 0) {
		uint16_t temp_set_time = atoi(DATA_IN + 9);
		if (temp_set_time <= PCYCLE_MAX_TIME) {
			printPGMStr(PSTR("\r\nPCYCLE TIME: "));
			fprintf(&USBSerialStream, "%i", temp_set_time);
			EEPROM_Write_PCycle_Time((uint8_t)temp_set_time);
			return;
		}
	}
	// Set a port or list of ports default state at startup to ON
	if (strncmp_P(DATA_IN, PSTR("SETDEFON"), 8) == 0) {
		INPUT_Parse_args(&pd, DATA_IN + 8);
		for (uint8_t i = 0; i < PORT_CNT; i++) {
			if (pd & (1 << i)) {
				PORT_DEF[i] = 1;
				printPGMStr(STR_Port_Default);
				fprintf(&USBSerialStream, "%i ", i+1);
				printPGMStr(STR_Enabled);
			}
		}
		EEPROM_Write_Port_Defaults();
		return;
	}
	// Set a port or list of ports default state at startup to OFF
	if (strncmp_P(DATA_IN, PSTR("SETDEFOFF"), 9) == 0) {
		INPUT_Parse_args(&pd, DATA_IN + 9);
		for (uint8_t i = 0; i < PORT_CNT; i++) {
			if (pd & (1 << i)) {
				PORT_DEF[i] = 0;
				printPGMStr(STR_Port_Default);
				fprintf(&USBSerialStream, "%i ", i+1);
				printPGMStr(STR_Disabled);
			}
		}
		EEPROM_Write_Port_Defaults();
		return;
	}
	// Set the Port 8 Sense mode
	if (strncmp_P(DATA_IN, PSTR("SETSENSE"), 8) == 0) {
		if (DATA_IN[8] == 'V') {
			EEPROM_Write_P8_Sense(1);
			printPGMStr(STR_Port_8_Sense);
			printPGMStr(PSTR("VOLTAGE"));
			return;
		}
		if (DATA_IN[8] == 'I') {
			EEPROM_Write_P8_Sense(0);
			printPGMStr(STR_Port_8_Sense);
			printPGMStr(PSTR("CURRENT"));
			return;
		}
	}
	// Set the VREF voltage and store in EEPROM to correct voltage readings.
	if (strncmp_P(DATA_IN, PSTR("SETVREF"), 7) == 0) {
		uint16_t temp_set_vref = atoi(DATA_IN + 7);
		if (temp_set_vref >= VREF_MIN && temp_set_vref <= VREF_MAX){
			float temp_vref = (float)temp_set_vref / 1000.0;
			EEPROM_Write_REF_V(temp_vref);
			printPGMStr(PSTR("\r\nVREF: "));
			fprintf(&USBSerialStream, "%.3fV", temp_vref);
			return;
		}
	}
	// Set the name for a given port.
	if (strncmp_P(DATA_IN, PSTR("SETNAME"), 7) == 0) {
		char *str = DATA_IN + 7;
		int8_t portid;
		char temp_name[16];

		while (*str == ' ' || *str == '\t') str++;
		if (*str < '1' || *str > '8') {
			// TODO print syntax error
			return;
		}
		portid = *str - '1';
		str++;
		while (*str == ' ' || *str == '\t') str++;

		EEPROM_Write_Port_Name(portid, str);
		printPGMStr(STR_NR_Port);
		EEPROM_Read_Port_Name(portid, temp_name);
		fprintf(&USBSerialStream, "%i NAME: %s", portid + 1, temp_name);
		return;
	}
	
	// If none of the above commands were recognized, print a generic error.
	printPGMStr(STR_Unrecognized);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~ Printing Functions
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Print a summary of all ports status'
static inline void PRINT_Status(void) {
	float voltage, current;
	if (PORT8_SENSE == 1) {
		voltage = ADC_Read_Voltage();
		printPGMStr(PSTR("\r\nInput Voltage: "));
		fprintf(&USBSerialStream, "%.2fV", voltage);
	}
	for(uint8_t i = 0; i < PORT_CNT; i++) {
		printPGMStr(STR_NR_Port);
		char temp_name[16];
		EEPROM_Read_Port_Name(i, temp_name);
		fprintf(&USBSerialStream, "%i \"%s\": ", i+1, temp_name);
		if (PORT_STATE[i] == 1) { printPGMStr(STR_Enabled); } else { printPGMStr(STR_Disabled); }
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

// Print a PGM stored string
static inline void printPGMStr(PGM_P s) {
	char c;
	while((c = pgm_read_byte(s++)) != 0) fputc(c, &USBSerialStream);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~ Port/LED Control Functions
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Turn a port ON (state == 1) or OFF (state == 0)
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

// Turn a LED ON (state == 1) or OFF (state == 0)
// LED 1 == Green, LED 0 == Red
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
	for (uint8_t i = 0; i < PORT_CNT; i++) {
		// Update the PORT_DEF array with the values from EEPROM
		PORT_DEF[i] = eeprom_read_byte((uint8_t*)(EEPROM_OFFSET_PORT_DEFAULTS + i));
		// If the value is not 0 or 1 (uninitialized), default it to 1
		if (PORT_DEF[i] < 0 || PORT_DEF[i] > 1) PORT_DEF[i] = 1;
	}
}
// Write the default port state settings from the PORT_DEF array in RAM
static inline void EEPROM_Write_Port_Defaults(void) {
	for (uint8_t i = 0; i < PORT_CNT; i++) {
		// Update the EERPOM with the values from the PORT_DEF array
		eeprom_update_byte((uint8_t*)(EEPROM_OFFSET_PORT_DEFAULTS + i), PORT_DEF[i]);
	}
}

// Read the stored reference voltage from EEPROM
static inline void EEPROM_Read_REF_V(void) {
	REF_V = eeprom_read_float((float*)(EEPROM_OFFSET_REF_V));
	// If the value seems out of range (uninitialized), default it to 4.2
	if (REF_V < 4.0 || REF_V > 4.4 || isnan(REF_V)) REF_V = 4.2;
}
// Write the reference voltage to EEPROM
static inline void EEPROM_Write_REF_V(float reference) {
	eeprom_update_float((float*)(EEPROM_OFFSET_REF_V), reference);
	REF_V = reference;
}

// Read the stored Port 8 Sense mode
static inline void EEPROM_Read_P8_Sense(void) {
	PORT8_SENSE = eeprom_read_byte((uint8_t*)(EEPROM_OFFSET_P8_SENSE));
	if (PORT8_SENSE < 0 || PORT8_SENSE > 1) PORT8_SENSE = 0;
}
// Write the Port 8 Sense mode to EEPROM
static inline void EEPROM_Write_P8_Sense(uint8_t mode) {
	eeprom_update_byte((uint8_t*)(EEPROM_OFFSET_P8_SENSE), mode);
	PORT8_SENSE = mode;
}

// Read PCYCLE_TIME from EEPROM
static inline void EEPROM_Read_PCycle_Time(void) {
	PCYCLE_TIME = eeprom_read_byte((uint8_t*)(EEPROM_OFFSET_CYCLE_TIME));
	if (PCYCLE_TIME < 0 || PCYCLE_TIME > PCYCLE_MAX_TIME) PCYCLE_TIME = 1; 
}
// Write the PCYCLE_TIME to EEPROM
static inline void EEPROM_Write_PCycle_Time(uint8_t time) {
	eeprom_update_byte((uint8_t*)(EEPROM_OFFSET_CYCLE_TIME), time);
	PCYCLE_TIME = time;
}

// Read the stored port name
static inline void EEPROM_Read_Port_Name(uint8_t port, char *str) {
	char working = 0;
	uint8_t count = 0;
	
	while (1) {
		// Read a byte from the EEPROM
		working = eeprom_read_byte((uint8_t*)(EEPROM_OFFSET_P0NAME+(port*16)+count));
		
		// If we've reached the end of the string, terminate the string, and break.
		if (working  == 255 || working == 0) {
			*str = 0;
			break;
		}
		
		// Take the byte we've read, and attach it to the string
		*str = working;
		str++;
		count++;
	}
}
// Write the port name to EEPROM
static inline void EEPROM_Write_Port_Name(uint8_t port, char *str) {
	// While we haven't reached the end of the string, or reached the end of the buffer
	// Write the name bytes to EEPROM
	for (uint8_t i = 0; i < 15; i++) {
		//if (*str == 0) { break; }
		eeprom_update_byte((uint8_t*)(EEPROM_OFFSET_P0NAME+(port*16)+i), *str);
		str++;
	}
	eeprom_update_byte((uint8_t*)(EEPROM_OFFSET_P0NAME+(port*16)+15), 0);
}

// Dump all EEPROM variables
static inline void EEPROM_Dump_Vars(void) {
	// Read port defaults
	printPGMStr(PSTR("\r\nPORT DEF: "));
	for (uint8_t i = 0; i < PORT_CNT; i++) {
		fprintf(&USBSerialStream, "%i ", eeprom_read_byte((uint8_t*)(EEPROM_OFFSET_PORT_DEFAULTS + i)));
	}
	// Read REF_V
	printPGMStr(PSTR("\r\nREF_V: "));
	fprintf(&USBSerialStream, "%.2f %.2f", eeprom_read_float((float*)(EEPROM_OFFSET_REF_V)), REF_V);
	// Read P8_SENSE
	printPGMStr(PSTR("\r\nP8SENSE: "));
	fprintf(&USBSerialStream, "%i", eeprom_read_byte((uint8_t*)(EEPROM_OFFSET_P8_SENSE)));
	// Read Port Cycle Time
	printPGMStr(PSTR("\r\nPCYCLE: "));
	fprintf(&USBSerialStream, "%iS", eeprom_read_byte((uint8_t*)(EEPROM_OFFSET_CYCLE_TIME)));
	// Read Port Names
	printPGMStr(PSTR("\r\nPNAMES: "));
	for (uint8_t i = 0; i < PORT_CNT; i++) {
		for (uint8_t j = 0; j < 16; j++) {
			fputc(eeprom_read_byte((uint8_t*)(EEPROM_OFFSET_P0NAME+(i*16)+j)), &USBSerialStream);
		}
		fputc(' ', &USBSerialStream);
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

// Set up the SPI registers to enable the SPI hardware
static inline void SPI_begin(void) {
	// Set SS to high so a connected chip will be "deselected" by default
	// Set SS to output

	SPCR |= _BV(MSTR);
	SPCR |= _BV(SPE);

	// Set output for SCK and MOSI pin
}

// Disable the SPI hardware
static inline void SPI_end(void) {
	SPCR &= ~_BV(SPE);
}

// Transfer out a byte on the SPI port, and simultaneously read a byte from SPI
static inline uint8_t SPI_transfer(uint8_t _data) {
	SPDR = _data;
	while(!(SPSR & _BV(SPIF))) {};
	return SPDR;
}

// 0 = LSBFIRST
static inline void SPI_setBitOrder(uint8_t bitOrder) {
	if (bitOrder == 0) {
		SPCR |= _BV(DORD);
	} else {
		SPCR &= ~(_BV(DORD));
	}
}

// Set SPI data mode (where in the cycle bits are to be read)
static inline void SPI_setDataMode(uint8_t mode) {
	SPCR = (SPCR & ~SPI_MODE_MASK) | mode;
}

// Set the SPI clock divider to determine overall speed
static inline void SPI_setClockDivider(uint8_t rate) {
	SPCR = (SPCR & ~SPI_CLOCK_MASK) | (rate & SPI_CLOCK_MASK);
	SPSR = (SPSR & ~SPI_2XCLOCK_MASK) | ((rate >> 2) & SPI_2XCLOCK_MASK);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~ USB Functions
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Run the LUFA USB tasks (except reading)
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

