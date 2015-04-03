K7NVH DC PDU
=======

The K7NVH DC PDU is a device designed to provide remote power control of DC devices in an affordable, low power, and flexible package. Each port is capable of handling 3+ amps, and can also measure the current flowing through each port.

Power can be supplied to the device via either USB or a standard barrel jack at 5V.

The device is available to the host system as a USB serial device, which when accessed accepts a number of basic ASCII commands which can be entered manually, or via a user created script to automate control actions.

This is the source code to be compiled by AVR-GCC for installation on the device itself. The device should already have this installed, and is only needed if you want to make modifications.

## Supported Commands
### HELP
The 'HELP' command will print a short message referencing the project page containing documentation.

### STATUS
The 'STATUS' command is designed to give an overview of the running state of each port on the PDU. The port numbers, any configured custom port names, and port state (ENABLED/DISABLED) will be displayed. Input voltage, board temperature, and per port current and power values will be shown. Voltages sensed on the auxiliary inputs will also be displayed. Note that unconnected auxiliary inputs may float and show voltages on unconnected pins. Messages that ports have been disabled due to overload, or are under automatic voltage control are displayed if applicable.

```plain
> STATUS
Voltage: 13.76V Temperature: 25C
PORT 1 "": ENABLED Current: 0.00A Power: 0.0W 
PORT 2 "": ENABLED Current: 0.00A Power: 0.0W 
PORT 3 "": ENABLED Current: 0.00A Power: 0.0W 
PORT 4 "": ENABLED Current: 0.00A Power: 0.0W 
PORT 5 "": ENABLED Current: 0.00A Power: 0.0W 
PORT 6 "": ENABLED Current: 0.00A Power: 0.0W 
PORT 7 "": ENABLED Current: 0.00A Power: 0.0W 
PORT 8 "": ENABLED Current: 0.00A Power: 0.0W 
AUX 1:0.01V 2:0.04V 3:0.01V 4:0.02V 5:0.03V 6:0.00V
```

### PSTATUS
The 'PSTATUS' command is designed to give an overview of the running state of the PDU in a easily parseable format for use with programs querying the PDU for information. The output is in a simple CSV format, defined as follows.

```plain
K7NVH DC PDU,Version Number,Device Name
Input Voltage,Board Temperature
AIN1,AIN2,AIN3,AIN4,AIN5,AIN6
Port Number,Port Name,Binary Enabled/Disabled Flag,Port Current,Port Power,Binary Overload Flag,Automatic Voltage Control Flag
(The above line is repeated for each port)
```
An example output might look like the following.
```plain
> PSTATUS
K7NVH DC PDU,1.1,
2.15,23
0.02,0.04,0.03,0.01,0.01,0.03
0,,1,0.00,0.0,0,0
1,,1,0.00,0.0,0,0
2,,1,0.00,0.0,0,0
3,,1,0.00,0.0,0,0
4,,1,0.00,0.0,0,0
5,,1,0.00,0.0,0,0
6,,1,0.00,0.0,0,0
7,Port 8,1,0.03,0.1,0,0
```

### PON
The 'PON' command is used to enable one or more ports on the PDU.

The PDU supports enabling multiple ports at a time. For example, all of the following are valid 'PON' syntaxes. 'A' can also be substituted for a port number to enable all ports.
`PON 1` `PON 1 2 3 4` `PON A`

### POFF
The 'POFF' command is used to disable one or more ports on the PDU.

The PDU supports disabling multiple ports at a time. For example, all of the following are valid 'POFF' syntaxes. 'A' can also be substituted for a port number to disable all ports.
`POFF 1` `POFF 1 2 3 4` `POFF A`

### PCYCLE
The 'PCYCLE' command is used to disable one or more ports on the PDU for a period of time set by the 'SETCYCLE' command, after which, the port is enabled again.

If 'PCYCLE' is issued on a port that is already disabled, the port will remain disabled for the period of time set by the 'SETCYCLE' command, and will be enabled with any other ports specified.

The PDU supports cycling multiple ports at a time. 'A' can also be substituted for a port number to cycle all ports. The following are valid PCYCLE syntaxes. `PCYCLE 1` `PCYCLE 1 2 3 4` `PCYCLE A`

### SETCYCLE
The 'SETCYCLE' command is used to set the period of time in seconds that ports will be disabled on the PDU during a 'PCYCLE' command. This command has no immediate impact on any ports.

Valid range for the 'SETCYCLE' command is 0-30 seconds. 0 seconds being no delay between disabling and re-enabling a port, and 30 seconds being the maximum delay allowable.

For example, to set a 5 second delay, the following is valid 'SETCYCLE' syntax. `SETCYCLE 5`

### SETDEFON
The 'SETDEFON' command is used to store the desired default state of a given port at PDU boot time to enabled. The PDU will default to all ports being enabled at boot, but you may leave ports disabled at boot with the 'SETDEFOFF' command, or set them back to enabled via 'SETDEFON'.

The PDU supports setting multiple ports at a time to be enabled by default. 'A' can also be substituted for a port number to change all ports. `SETDEFON 1` `SETDEFON 1 2 3 4` `SETDEFON A`

### SETDEFOFF
The 'SETDEFOFF' command is used to store the desired default state of a given port at PDU boot time to disabled. The PDU will default to all ports being enabled at boot, but you may leave ports disabled at boot with the 'SETDEFOFF' command, or set them back to enabled via 'SETDEFON'.

The PDU supports setting multiple ports at a time to be disabled by default. 'A' can also be substituted for a port number to change all ports. `SETDEFOFF 1` `SETDEFOFF 1 2 3 4` `SETDEFOFF A`

### SETNAME
The 'SETNAME' command is used to store a user defined "helpful" name for each port on the PDU. By default the custom names are blank, but they may be set to a user defined string of up to 15 characters in length. Strings longer than 15 characters will be truncated.

The user defined name will be displayed alongside the port number in the output from the 'STATUS' command.

The PDU supports setting only one port name at a time. The following is an example of valid 'SETNAME' syntax, setting Port 1's name to "Testing". `SETNAME 1 Testing`

The 'SETNAME' command can also be used to give the PDU device an overall name by using 'P' in place of the port number. The device name will be displayed as part of the input prompt as a visual aid to ensure users are interacting with the intended device. The following is an example of valid 'SETNAME' syntax to set the device name. `SETNAME P PDU-Test`

The same string length limits that apply to individual port names also apply to the device name.

### SETLIMIT
The 'SETLIMIT' command is used to store a user defined overload current limit for each port on the PDU. By default the current limit is set to 10 amps. While the PDU is not rated for this current flow, 10A was chosen to effectively "disable" current limits from disabling PDU ports.

During normal operation the PDU will continuously check current flow on each port, and compare against the stored limits. If a port is found to exceed the stored limit for greater than 10ms (milliseconds), the port will be disabled, a warning message will be printed, and a overload flag will be displayed in the 'STATUS' output.

If a port has been disabled due to current overload, the port can be re-enabled with a manual 'PON' command.

'SETLIMIT' is set in units of mA (milliamps), and accepts values from 0 to 10000 (0 to 10 amps). However, the provided input will be truncated to tenths of amps. For example, a value of 1150, will be truncated to 1.1A.

The PDU supports setting only one port limit at a time. The following is an example of setting Port 1's current limit to 1.5A. `SETLIMIT 1 1500`

### VCTLON
The 'VCTLON' command is used to enable the PDU to control a given port automatically based on the sensed input voltage. This setting is persistent across reboots of the PDU device. Automatic voltage control of a given port is disabled until manually enabled again, or the PDU is rebooted, in the case of any manual action controlling a port, or an overload condition disables a port.

The voltage thresholds for enabling and disabling a port are set with the 'SETVCTLON' and 'SETVCTLOFF' commands.

Input voltage is polled approximately every 5 seconds, and ports will be enabled/disabled after two consecutive polling cycles have shown voltages beyond the configured thresholds.

Automatic voltage control can be very helpful for battery powered environments, to disable devices if available battery power runs too low. As each port can have unique settings, the user can configure staggered shutdown or startup for equipment as batteries are drained, and subsequently recharged.

The PDU supports setting multiple ports at a time for automatic control. The following are examples of valid syntax for enabling automatic control. `VCTLON 1` `VCTLON 1 2 3 4`

### VCTLOFF
The 'VCTLOFF' command is used to disable the PDU from controlling a given port automatically based on the sensed input voltage. This setting is persistent across reboots of the PDU device.

The PDU supports setting multiple ports at a time for disabled automatic control. The following are examples of valid syntax for disabling automatic control. `VCTLOFF 1` `VCTLOFF 1 2 3 4`

### SETVCTLON
The 'SETVCTLON' command is used to set the voltage, at which a port should be enabled, when configured for automatic control. By default this value is set at 40 volts, the limit of the PDU device, effectively disabling automatic port control.

'SETVCTLON' is set in units of hundredths of volts, and accepts values from 0 to 4000 (0 to 40 volts).

The PDU supports setting only one port enable threshold at a time. The following is an example of setting the enable threshold of Port 1 to 10 volts. `SETVCTLON 1 1000`

### SETVCTLOFF
The 'SETVCTLOFF' command is used to set the voltage, at which a port should be disabled, when configured for automatic control. By default this value is set at 0 volts, effectively disabling automatic port control.

'SETVCTLOFF' is set in units of hundredths of volts, and accepts values from 0 to 4000 (0 to 40 volts).

The PDU supports setting only one port disable threshold at a time. The following is an example of setting the disable threshold of Port 1 to 10 volts. `SETVCTLON 1 1000`

### SETVREF
The 'SETVREF' command is used only during calibration of the PDU. Measuring the voltage reference regulator with an accurate voltage meter, the calibration of voltage and current measurements can be updated.

'SETVREF' is set in units of mV (millivolts), and accepts values from 4000 to 4400 (4.0V to 4.4V). The default value if uncalibrated is the regulator's nominal output voltage of 4.2V.

For example, to set the voltage reference calibration to 4.2V, the following are valid 'SETVREF' syntaxes. `SETVREF4200` `SETVREF 4200`

### SETVCAL
The 'SETVCAL' command is used only during calibration of the PDU. Measuring the voltage divider on the input port with an accurate voltage meter, calibration of voltage supply measurements can be made.

'SETVCAL' is a unitless multiplier, specified in VALUE*10, and accepts values from 150 to 70 (15.0X to 7.0X). The default value if uncalibrated is the dividers's nominal voltage division of 11.0X.

For example, to set the divider reference calibration to 11.0X, the following are valid 'SETVCAL' syntaxes. `SETVCAL110` `SETVCAL 110`

### SETICAL
The 'SETICAL' command is used only during calibration of the PDU. Measuring the voltage divider on the current sense op amp for a given port with an accurate meter, calibration of current sense measurements can be made.

'SETICAL' is a unitless multiplier, specified in VALUE*10, and accepts values from 160 to 60 (16.0X to 6.0X). The default value if uncalibrated is the dividers's nominal voltage division of 11.0X.

For example, to set the calibration on port 1 to 11.0X, the following is valid 'SETICAL' syntax. `SETICAL 1 110`

### EEPROMDUMP
The 'EEPROMDUMP' command is useful for debugging PDU stored state. It will output a variety of values stored in various locations in EEPROM memory, and may not be formatted for easy understanding.

In the cases of unset or default values, the 'EEPROMDUMP' command may return a number of unprintable characters to your terminal. This is expected behavior.

## Drivers
The PDU board is automatically recognized as a USB serial device under OSX and Linux, however, windows requires a driver to associate the device with the built in USB serial device drivers.

When first attaching the device to a windows based host, windows will attempt to find a driver and fail. You may then specify a driver. This is provided in K7NVH_PDU.inf.

Once you have installed the driver, the PDU will register as a COM port device under windows, and you may access it with any serial/terminal application of your preference.

### Windows 8 Users
During the driver installation process on Windows 8 operating systems you may encounter *"The third-party INF does not contain digital signature information"* preventing the installation of the included PDU driver.  The steps below outline the process for disabling driver signature enforcement on Windows 8/8.1 systems.

**NOTE:** If your system uses BitLocker Drive Encryption, you will need your encryption key to gain access to advanced settings to disable driver signature enforcement.

Windows 8 - Navigate to PC settings > General and look for the *Advanced startup* dialog.  Click the **Restart now** button to access the advanced startup settings.  Your system will now reboot.

Windows 8.1 - Navigate to PC settings > Update and Recovery > Recovery and look for the *Advanced startup* dialog.  Click the **Restart now** button to access the advanced startup settings.  Your system will now reboot.

After rebooting select Troubleshoot > Advanced options > Startup Settings, click the **Restart** button, your machine will reboot a second time.

After rebooting a second time you should be presented with the Windows Startup Settings dialog, choose *Disable driver signature enforcement* by pressing number `7` on your keyboard.  Your machine will reboot automatically.

Once rebooted, you should now be able to install the PDU drviers as outlined above.  Once the appropriate driver is selected you will see a Windows Security dialog warning against the installation of the unsigned driver.  Click **Install this driver software anyway** to complete the driver installation.
