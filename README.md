K7NVH DC PDU
=======

The K7NVH DC PDU is a device designed to provide remote power control of DC devices in an affordable, low power, and flexible package. Each port is capable of handling 3+ amps, and can also measure the current flowing through each port.

Power can be supplied to the device via either USB or a standard barrel jack at 5V.

The device is available to the host system as a USB serial device, which when accessed accepts a number of basic ASCII commands which can be entered manually, or via a user created script to automate control actions.

This is the source code to be compiled by AVR-GCC for installation on the device itself. The device should already have this installed, and is only needed if you want to make modifications.

## Supported Commands
### STATUS
The 'STATUS' command is designed to give an overview of the running state of each port on the PDU. The port numbers, any configured custom port names, and port state (ENABLED/DISABLED) will be displayed.

If the PDU is configured for Voltage Sense, 'STATUS' will display the voltage on the INPUT port, as well as the current (amps) and power (watts) usage on ports 1 through 7.

If the PDU is configured for Current Sense, 'STATUS' will display the current (amps) usage on ports 1 through 8.

### PON
The 'PON' command is used to enable one or more ports on the PDU.

The PDU supports somewhat arbitrary specification of which ports to enable. For example, all of the following are valid 'PON' syntaxes.
`PON 1` `PON1` `PON 1 2 3 4` `PON1234` `PON 1234`

### POFF
The 'POFF' command is used to disable one or more ports on the PDU.

The PDU supports somewhat arbitrary specification of which ports to disable. For example, all of the following are valid 'POFF' syntaxes.
`POFF 1` `POFF1` `POFF 1 2 3 4` `POFF1234` `POFF 1234`

### PCYCLE
The 'PCYCLE' command is used to disable one or more ports on the PDU for a period of time set by the 'SETCYCLE' command, after which, the port is enabled again.

If 'PCYCLE' is issued on a port that is already disabled, the port will remain disabled for the period of time set by the 'SETCYCLE' command, and will be enabled with any other ports specified.

The PDU supports somewhat arbitrary specification of which ports to cycle. For example, all of the following are valid 'PCYCLE' syntaxes. `PCYCLE 1` `PCYCLE1` `PCYCLE 1 2 3 4` `PCYCLE1234` `PCYCLE 1234`

### SETCYCLE
The 'SETCYCLE' command is used to set the period of time in seconds that ports will be disabled on the PDU during a 'PCYCLE' command. This command has no immediate impact on any ports.

Valid range for the 'SETCYCLE' command is 0-30 seconds. 0 seconds being no delay between disabling and re-enabling a port, and 30 seconds being the maximum delay allowable.

For example, to set a 5 second delay, the following are valid 'SETCYCLE' syntaxes. `SETCYCLE5` `SETCYCLE 5`

### SETDEFON
The 'SETDEFON' command is used to store the desired default state of each port at PDU boot time to enabled. The PDU will default to all ports being enabled at boot, but you may leave ports disabled at boot with the 'SETDEFOFF' command, or set them back to enabled via 'SETDEFON'.

The PDU supports somewhat arbitrary specification of which ports to change default state for. For example, all of the following are valid 'SETDEFON' syntaxes. `SETDEFON 1` `SETDEFON1` `SETDEFON 1 2 3 4` `SETDEFON1234` `SETDEFON 1234`

### SETDEFOFF
The 'SETDEFOFF' command is used to store the desired default state of each port at PDU boot time to disabled. The PDU will default to all ports being enabled at boot, but you may leave ports disabled at boot with the 'SETDEFOFF' command, or set them back to enabled via 'SETDEFON'.

The PDU supports somewhat arbitrary specification of which ports to change default state for. For example, all of the following are valid 'SETDEFOFF' syntaxes. `SETDEFOFF 1` `SETDEFOFF1` `SETDEFOFF 1 2 3 4` `SETDEFOFF1234` `SETDEFOFF 1234`