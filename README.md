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

The PDU supports somewhat arbitrary specification of which ports to enable. For example, all of the following are valid 'PON' syntaxes. 'A' can also be substituted for a port number to enable all ports.
`PON 1` `PON1` `PON 1 2 3 4` `PON1234` `PON 1234` `PON A`

### POFF
The 'POFF' command is used to disable one or more ports on the PDU.

The PDU supports somewhat arbitrary specification of which ports to disable. For example, all of the following are valid 'POFF' syntaxes. 'A' can also be substituted for a port number to disable all ports.
`POFF 1` `POFF1` `POFF 1 2 3 4` `POFF1234` `POFF 1234` `POFF A`

### PCYCLE
The 'PCYCLE' command is used to disable one or more ports on the PDU for a period of time set by the 'SETCYCLE' command, after which, the port is enabled again.

If 'PCYCLE' is issued on a port that is already disabled, the port will remain disabled for the period of time set by the 'SETCYCLE' command, and will be enabled with any other ports specified.

The PDU supports somewhat arbitrary specification of which ports to cycle. For example, all of the following are valid 'PCYCLE' syntaxes. 'A' can also be substituted for a port number to cycle all ports. `PCYCLE 1` `PCYCLE1` `PCYCLE 1 2 3 4` `PCYCLE1234` `PCYCLE 1234` `PCYCLE A`

### SETCYCLE
The 'SETCYCLE' command is used to set the period of time in seconds that ports will be disabled on the PDU during a 'PCYCLE' command. This command has no immediate impact on any ports.

Valid range for the 'SETCYCLE' command is 0-30 seconds. 0 seconds being no delay between disabling and re-enabling a port, and 30 seconds being the maximum delay allowable.

For example, to set a 5 second delay, the following are valid 'SETCYCLE' syntaxes. `SETCYCLE5` `SETCYCLE 5`

### SETDEFON
The 'SETDEFON' command is used to store the desired default state of a given port at PDU boot time to enabled. The PDU will default to all ports being enabled at boot, but you may leave ports disabled at boot with the 'SETDEFOFF' command, or set them back to enabled via 'SETDEFON'.

The PDU supports somewhat arbitrary specification of which ports to change default state for. For example, all of the following are valid 'SETDEFON' syntaxes. 'A' can also be substituted for a port number to change all ports.`SETDEFON 1` `SETDEFON1` `SETDEFON 1 2 3 4` `SETDEFON1234` `SETDEFON 1234` `SETDEFON A`

### SETDEFOFF
The 'SETDEFOFF' command is used to store the desired default state of a given port at PDU boot time to disabled. The PDU will default to all ports being enabled at boot, but you may leave ports disabled at boot with the 'SETDEFOFF' command, or set them back to enabled via 'SETDEFON'.

The PDU supports somewhat arbitrary specification of which ports to change default state for. For example, all of the following are valid 'SETDEFOFF' syntaxes. 'A' can also be substituted for a port number to change all ports. `SETDEFOFF 1` `SETDEFOFF1` `SETDEFOFF 1 2 3 4` `SETDEFOFF1234` `SETDEFOFF 1234` `SETDEFOFF A`

### SETNAME
The 'SETNAME' command is used to store a user defined "helpful" name for each port on the PDU. By default the custom names are blank, but they may be set to a user defined string of up to 15 characters in length. Strings longer than 15 characters will be truncated.

The user defined name will be displayed alongside the port number in the output from the 'STATUS' command.

The PDU supports setting only one port name at a time, but the syntax is flexible like with other multi-port commands. The following are examples of valid 'SETNAME' syntax, setting Port 1's name to "Testing". `SETNAME1Testing` `SETNAME1 Testing` `SETNAME 1Testing` `SETNAME 1 Testing`

### SETLIMIT
The 'SETLIMIT' command is used to store a user defined overload current limit for each port on the PDU. By default the current limit is set to 10 amps. While the PDU is not rated for this current flow, 10A was chosen to effectively "disable" current limits from disabling PDU ports.

During normal operation the PDU will continuously check current flow on each port, and compare against the stored limits. If a port is found to exceed the stored limit for greater than 10ms (milliseconds), the port will be disabled, a warning message will be printed, and a overload flag will be displayed in the 'STATUS' output.

'SETLIMIT' is set in units of mA (milliamps), and accepts values from 0 to 10000 (0 to 10 amps). However, the provided input will be truncated to tenths of amps. For example, a value of 1150, will be truncated to 1.1A.

The PDU supports setting only one port limit at a time, but the syntax is flexible like with other multi-port commands. The following are examples of valid 'SETLIMIT' syntax, setting Port 1's limit to 1.5A. `SETLIMIT11500` `SETLIMIT1 1500` `SETLIMIT 11500` `SETLIMIT 1 1500`

### SETVREF
The 'SETVREF' command is used only during calibration of the PDU. Measuring the voltage reference regulator with an accurate voltage meter, the calibration of voltage and current measurements can be updated.

'SETVREF' is set in units of mV (millivolts), and accepts values from 4000 to 4400 (4.0V to 4.4V). The default value if uncalibrated is the regulator's nominal output voltage of 4.2V.

For example, to set the voltage reference calibration to 4.2V, the following are valid 'SETVREF' syntaxes. `SETVREF4200` `SETVREF 4200`

### SETVDIV
The 'SETVDIV' command is used only during calibration of the PDU. Measuring the voltage divider on the input port with an accurate voltage meter, calibration of voltage supply measurements can be made.

'SETVDIV' is a unitless multiplier, specified in VALUE*10, and accepts values from 120 to 80 (12.0X to 8.0X). The default value if uncalibrated is the dividers's nominal voltage division of 10.1X.

For example, to set the divider reference calibration to 10.1X, the following are valid 'SETVDIV' syntaxes. `SETVREF101` `SETVREF 101`

### SETSENSE
The 'SETSENSE' command is used only after having changed the SENSE solder jumper on the PDU board, selecting the sense channel for Port 8 between sensing INPUT voltage, or the current flowing through Port 8.

'SETSENSE' is set with either I for current, or V for voltage. The default value will assume the jumper is set to measure current.

For example, to set the sensing mode to INPUT voltage, the following are valid 'SETSENSE' syntaxes. `SETSENSEV` `SETSENSE V`

### EEPROMDUMP
The 'EEPROMDUMP' command is used only for debugging PDU stored state. It will output a variety of values stored in various locations in EEPROM memory, and may not be formatted for easy understanding.

In the cases of unset or default values, the 'EEPROMDUMP' command may return a number of unprintable characters to your terminal. This is expected behavior.

## Drivers
The PDU board is automatically recognized as a USB serial device under OSX and Linux, however, windows requires a driver to associate the device with the built in USB serial device drivers.

When first attaching the device to a windows based host, windows will attempt to find a driver and fail. You may then specify a driver. This is provided in K7NVH_PDU.inf.

Once you have installed the driver, the PDU will register as a COM port device under windows, and you may access it with any serial/terminal application of your preference.