/* (c) 2015 Nigel Vander Houwen */
//
// TODO
// High water mark stored in EEPROM (Lifetime & User resettable)
// Document available commands

#include "K7NVH_DC_PDU.h"

// Main program entry point.
int main(void) {
	// Read EEPROM stored variables
	EEPROM_Read_REF_V();

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

	// Start up SPI
	SPI_begin();

	// Set up control pins
	DDRB |= (1 << P1EN)|(1 << P2EN)|(1 << P3EN)|(1 << P4EN);
	DDRC |= (1 << P5EN)|(1 << P6EN)|(1 << P7EN)|(1 << P8EN);
	DDRD |= (1 << LED1)|(1 << LED2);

	// Enable/Disable ports per their defaults
	for(uint8_t i = 0; i < PORT_CNT; i++) {
		PORT_CTL(i, EEPROM_Read_Port_Default(i));
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
			LED_CTL(0, 1);
			
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
		LED_CTL(0, 0);
		
		// Check for above threshold current usage
		for (uint8_t i = 0; i < 8; i++) {
			if (PORT_Check_Current_Limit(i)) {
				// Current is above threshold. Double check a moment later to make sure
				// we're not just seeing inrush current.
				_delay_ms(10);
				if (PORT_Check_Current_Limit(i)) {
					// Print a warning message.
					fprintf(&USBSerialStream, "\r\n");
					printPGMStr(STR_Overload);
					
					// Disable the port
					PORT_CTL(i, 0);
				
					// Mark the overload bit for this port
					PORT_STATE[i] |= 0b00000010;
					
					INPUT_Clear();
				}
			}
		}
		
		// Keep the LUFA USB stuff fed regularly.
		run_lufa();
	}
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~ Command Parsing Functions
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Flush out our data input buffer, reset our position variable, and print a new prompt.
static inline void INPUT_Clear(void) {
	memset(&DATA_IN[0], 0, sizeof(DATA_IN));
	DATA_IN_POS = 0;
	printPGMStr(STR_Prompt);
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
		PORT_Set_Ctl(&pd, 1);
		return;
	}
	// Turn off a port or a list of ports
	if (strncmp_P(DATA_IN, PSTR("POFF"), 4) == 0) {
		INPUT_Parse_args(&pd, DATA_IN + 4);
		PORT_Set_Ctl(&pd, 0);
		return;
	}
	// Power cycle a port or list of ports. Time is defined by EEPROM_Read_PCycle_Time().
	if (strncmp_P(DATA_IN, PSTR("PCYCLE"), 6) == 0) {
		INPUT_Parse_args(&pd, DATA_IN + 6);
		
		PORT_Set_Ctl(&pd, 0);

		fprintf(&USBSerialStream, "\r\n");
		run_lufa();
		for (uint16_t i = 0; i < (EEPROM_Read_PCycle_Time()); i++) {
			_delay_ms(1000);
			fputc('.', &USBSerialStream);
			run_lufa();
		}

		PORT_Set_Ctl(&pd, 1);
		
		return;
	}
	// Set PCYCLE_TIME and store in EEPROM
	if (strncmp_P(DATA_IN, PSTR("SETCYCLE"), 8) == 0) {
		uint16_t temp_set_time = atoi(DATA_IN + 8);
		if (temp_set_time <= PCYCLE_MAX_TIME) {
			printPGMStr(STR_PCYCLE_Time);
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
				EEPROM_Write_Port_Default(i, 1);
			}
		}
		return;
	}
	// Set a port or list of ports default state at startup to OFF
	if (strncmp_P(DATA_IN, PSTR("SETDEFOFF"), 9) == 0) {
		INPUT_Parse_args(&pd, DATA_IN + 9);
		for (uint8_t i = 0; i < PORT_CNT; i++) {
			if (pd & (1 << i)) {
				EEPROM_Write_Port_Default(i, 0);
			}
		}
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
			printPGMStr(STR_VREF);
			fprintf(&USBSerialStream, "%.3fV", temp_vref);
			return;
		}
	}
	// Set the name for a given port.
	if (strncmp_P(DATA_IN, PSTR("SETNAME"), 7) == 0) {
		char *str = DATA_IN + 7;
		uint8_t portid;
		char temp_name[16];

		while (*str == ' ' || *str == '\t') str++;
		if (*str >= '1' && *str <= '8') {
			portid = *str - '1';
			str++;
			while (*str == ' ' || *str == '\t') str++;

			EEPROM_Write_Port_Name(portid, str);
			printPGMStr(STR_NR_Port);
			EEPROM_Read_Port_Name(portid, temp_name);
			fprintf(&USBSerialStream, "%i NAME: %s", portid + 1, temp_name);
			return;
		}
	}
	// Set the current limit for a given port.
	if (strncmp_P(DATA_IN, PSTR("SETLIMIT"), 8) == 0) {
		char *str = DATA_IN + 8;
		uint8_t portid;
		
		while (*str == ' ' || *str == '\t') str++;
		if (*str >= '1' && *str <= '8') {
			portid = *str - '1';
			str++;
		
			uint16_t temp_set_limit = (atoi(str) / 100);
			if (temp_set_limit <= LIMIT_MAX){
				EEPROM_Write_Port_Limit(portid, temp_set_limit);
				printPGMStr(STR_Port_Limit);
				fprintf(&USBSerialStream, "%.1fA", (float)temp_set_limit/10);
				return;
			}
		}
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
	if (EEPROM_Read_P8_Sense() == 1) {
		voltage = ADC_Read_Voltage();
		printPGMStr(PSTR("\r\nInput Voltage: "));
		fprintf(&USBSerialStream, "%.2fV", voltage);
	}
	for(uint8_t i = 0; i < PORT_CNT; i++) {
		printPGMStr(STR_NR_Port);
		char temp_name[16];
		EEPROM_Read_Port_Name(i, temp_name);
		fprintf(&USBSerialStream, "%i \"%s\": ", i+1, temp_name);
		if (PORT_STATE[i] & 0b00000001) { printPGMStr(STR_Enabled); } else { printPGMStr(STR_Disabled); }
		if (i == 7 && EEPROM_Read_P8_Sense() == 1) break;
		current = ADC_Read_Current(i);
		printPGMStr(PSTR(". Current: "));
		fprintf(&USBSerialStream, "%.2fA ", current);
		if (EEPROM_Read_P8_Sense() == 1) {
			printPGMStr(PSTR("Power: "));
			fprintf(&USBSerialStream, "%.1fW ", (voltage * current));
		}
		if (PORT_STATE[i] & 0b00000010) { printPGMStr(STR_Overload); }
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

// Set all ports in a port descriptor set to a state
static inline void PORT_Set_Ctl(pd_set *pd, uint8_t state) {
	for (uint8_t i = 0; i < PORT_CNT; i++) {
		if (*pd & (1 << i)) {
			PORT_CTL(i, state);
		}
	}
}

// Turn a port ON (state == 1) or OFF (state == 0)
static inline void PORT_CTL(uint8_t port, uint8_t state) {
	printPGMStr(STR_NR_Port);
	fprintf(&USBSerialStream, "%i ", port+1);

	if (state == 1) {
		if (port <= 3) {
			PORTB |= (1 << (4 + port));
		} else if (port <= 7) {
			PORTC |= (1 << (11 - port));
		}
		printPGMStr(STR_Enabled);
		PORT_STATE[port] |= 0b00000001;
	} else {
		if (port <= 3) {
			PORTB &= ~(1 << (4 + port));
		} else if (port <= 7) {
			PORTC &= ~(1 << (11 - port));
		}
		printPGMStr(STR_Disabled);
		PORT_STATE[port] &= 0b11111110;
	}
	
	// Clear any overload marks since we're manually setting this port
	PORT_STATE[port] &= 0b11111101;
}

// Turn a LED ON (state == 1) or OFF (state == 0)
// LED 0 == Green, LED 1 == Red
static inline void LED_CTL(uint8_t led, uint8_t state) {
	if (state == 1) {
		PORTD |= (1 << (LED1 + led));
	} else {
		PORTD &= ~(1 << (LED1 + led));
	}
}

// Checks a port against the stored current limit. Returns 0 if below limits, and 1 if
// the port has exceeded current limits.
static inline uint8_t PORT_Check_Current_Limit(uint8_t port){
	// If we were asked for port 8, but we're sensing voltage, we don't know what the
	// current flow is. Return 0.
	if (port == 7 && EEPROM_Read_P8_Sense() == 1) { return 0; }
	
	// Check for above threshold current flow, and return 1.
	if (ADC_Read_Current(port) > ((float)EEPROM_Read_Port_Limit(port) / 10.0)) { return 1; }
	
	// Else return 0;
	return 0;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~ EEPROM Functions
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ 

// Read the default port state setting
static inline uint8_t EEPROM_Read_Port_Default(uint8_t port) {
	uint8_t portdef = eeprom_read_byte((uint8_t*)(EEPROM_OFFSET_PORT_DEFAULTS + port));
	if (portdef > 1) portdef = 1;
	return portdef;
}
// Write the default port state setting
static inline void EEPROM_Write_Port_Default(uint8_t port, uint8_t portdef) {
	eeprom_update_byte((uint8_t*)(EEPROM_OFFSET_PORT_DEFAULTS + port), portdef);
	printPGMStr(STR_Port_Default);
	fprintf(&USBSerialStream, "%i ", port+1);
	printPGMStr(portdef ? STR_Enabled : STR_Disabled);
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
// 0 = Current, 1 = Voltage
static inline uint8_t EEPROM_Read_P8_Sense(void) {
	uint8_t PORT8_SENSE = eeprom_read_byte((uint8_t*)(EEPROM_OFFSET_P8_SENSE));
	if (PORT8_SENSE > 1) PORT8_SENSE = 0;
	return PORT8_SENSE;
}
// Write the Port 8 Sense mode to EEPROM
static inline void EEPROM_Write_P8_Sense(uint8_t mode) {
	eeprom_update_byte((uint8_t*)(EEPROM_OFFSET_P8_SENSE), mode);
}

// Read PCYCLE_TIME from EEPROM
// Stored as Seconds
static inline uint8_t EEPROM_Read_PCycle_Time(void) {
	uint8_t PCYCLE_TIME = eeprom_read_byte((uint8_t*)(EEPROM_OFFSET_CYCLE_TIME));
	if (PCYCLE_TIME > PCYCLE_MAX_TIME) PCYCLE_TIME = 1;
	return PCYCLE_TIME;
}
// Write the PCYCLE_TIME to EEPROM
static inline void EEPROM_Write_PCycle_Time(uint8_t time) {
	eeprom_update_byte((uint8_t*)(EEPROM_OFFSET_CYCLE_TIME), time);
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

// Stored as amps*10 so 50==5.0A
static inline uint8_t EEPROM_Read_Port_Limit(uint8_t port) {
	uint8_t limit = eeprom_read_byte((uint8_t*)(EEPROM_OFFSET_P0LIMIT+(port)));
	if (limit > LIMIT_MAX) {
		return LIMIT_MAX;
	}
	return limit;
}
// Stored as amps*10 so 50==5.0A
static inline void EEPROM_Write_Port_Limit(uint8_t port, uint8_t limit) {
	eeprom_update_byte((uint8_t*)(EEPROM_OFFSET_P0LIMIT+(port)), limit);
}

// Dump all EEPROM variables
static inline void EEPROM_Dump_Vars(void) {
	// Read port defaults
	printPGMStr(STR_Port_Default);
	for (uint8_t i = 0; i < PORT_CNT; i++) {
		fprintf(&USBSerialStream, "%i ", eeprom_read_byte((uint8_t*)(EEPROM_OFFSET_PORT_DEFAULTS + i)));
	}
	// Read REF_V
	printPGMStr(STR_VREF);
	fprintf(&USBSerialStream, "%.2f %.2f", eeprom_read_float((float*)(EEPROM_OFFSET_REF_V)), REF_V);
	// Read P8_SENSE
	printPGMStr(STR_Port_8_Sense);
	fprintf(&USBSerialStream, "%i", eeprom_read_byte((uint8_t*)(EEPROM_OFFSET_P8_SENSE)));
	// Read Port Cycle Time
	printPGMStr(STR_PCYCLE_Time);
	fprintf(&USBSerialStream, "%iS", eeprom_read_byte((uint8_t*)(EEPROM_OFFSET_CYCLE_TIME)));
	// Read Port Limits
	printPGMStr(STR_Port_Limit);
	for (uint8_t i = 0; i < PORT_CNT; i++) {
		fprintf(&USBSerialStream, "%i:%i ", eeprom_read_byte((uint8_t*)(EEPROM_OFFSET_P0LIMIT + i)), EEPROM_Read_Port_Limit(i));
	}
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
	// Set up SPI pins
	DDRB |= (1 << SPI_SS)|(1 << SPI_SCK)|(1 << SPI_MOSI);
	PORTB |= (1 << SPI_SS);
	PORTB &= ~(1 << SPI_SCK);

	// Enable SPI Master bit in the register
	SPCR |= _BV(MSTR);
	SPCR |= _BV(SPE);

	// Set our mode options
	SPI_setClockDivider(SPI_CLOCK_DIV128);
	SPI_setDataMode(SPI_MODE0);
	SPI_setBitOrder(1);
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

