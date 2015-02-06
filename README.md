K7NVH DC PDU
=======

The K7NVH DC PDU is a device designed to provide remote power control of DC devices in an affordable, low power, and flexible package. Each port is capable of handling 3+ amps, and can also measure the current flowing through each port.

Power can be supplied to the device via either USB or a standard barrel jack at 5V.

The device is available to the host system as a USB serial device, which when accessed accepts a number of basic ASCII commands which can be entered manually, or via a user created script to automate control actions.

This is the source code to be compiled by AVR-GCC for installation on the device itself. The device should already have this installed, and is only needed if you want to make modifications.

## Supported Commands
### STATUS
The 'STATUS' command is designed to give an overview of the running state of each port on the PDU. The port numbers, any configured custom port names, and port state (ENABLED/DISABLED) will be displayed. If the PDU is configured for Voltage Sense, 'STATUS' will display the voltage on the INPUT port, as well as the current (amps) and power (watts) usage on ports 1 through 7. If the PDU is configured for Current Sense, 'STATUS' will display the current (amps) usage on ports 1 through 8.

### PON
The 'PON' command is used to enable one or more ports on the PDU. The PDU supports somewhat arbitrary specification of which ports to enable, for example, all of the following are valid 'PON' syntaxes.
`PON 1` `PON1` `PON 1 2 3 4` `PON1234` `PON 1234`

### POFF
The 'POFF' command is used to disable one or more ports on the PDU. The PDU supports somewhat arbitrary specification of which ports to disable, for example, all of the following are valid 'POFF' syntaxes.
`POFF 1` `POFF1` `POFF 1 2 3 4` `POFF1234` `POFF 1234`