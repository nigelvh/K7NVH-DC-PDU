SNMP
=======

## k7nvh_dc_pdu.c
This is the C source for a program to interact with the PDU, and output the data to stdout. Invocation will be along the lines of `k7nvh_dc_pdu /dev/serialDeviceName 'STATUS'` to send the PDU the STATUS command, and print the output. To connect to the PDU, the user running the application must be a member of the 'dialout' group under linux. Under other systems, whatever permissions needed to interact with serial ports are required.

Note that this utility at present simply responds with all output received before a 1 second timeout. This should accomodate all functions except PCYCLE.

## pdu_snmp
This is a PHP script intended to be run on the command line (php-cli is required), and interprets the output from a STATUS command into SNMP OIDs. By default it will read this output from the `/tmp/pdu_status.txt` file. In my environment, this file is populated by an entry in the system crontab, that runs the `k7nvh_dc_pdu` utility. This crontab entry looks like: 
`* * * * *       root    /nigel_scripts/PDU/k7nvh_dc_pdu /dev/ttyACM1 'STATUS' |egrep -i "Voltage|PORT" > /tmp/pdu_status.txt`

Then your snmpd installation needs to be configured to pass the proper OID requests to the `pdu_snmp` script. In my `/etc/snmp/snmpd.conf` file, I have added the following config option:
`pass .1.3.6.1.4.1.8072.2.2048   "/nigel_scripts/PDU/pdu_snmp"`

Now, you should be able to snmpget or snmpwalk PDU data. For example, this will snmpwalk the current usage on each port.
```plain
snmpwalk -v 2c -c <COMMUNITY> <IP ADDRESS> .1.3.6.1.4.1.8072.2.2048.1.3.3
iso.3.6.1.4.1.8072.2.2048.1.3.3.1.1 = STRING: "0.00"
iso.3.6.1.4.1.8072.2.2048.1.3.3.2.1 = STRING: "0.00"
iso.3.6.1.4.1.8072.2.2048.1.3.3.3.1 = STRING: "0.00"
iso.3.6.1.4.1.8072.2.2048.1.3.3.4.1 = STRING: "0.00"
iso.3.6.1.4.1.8072.2.2048.1.3.3.5.1 = STRING: "0.17"
iso.3.6.1.4.1.8072.2.2048.1.3.3.6.1 = STRING: "0.22"
iso.3.6.1.4.1.8072.2.2048.1.3.3.7.1 = STRING: "0.00"
```

Other OIDs of interest that can be walked are:
```plain
.1.3.6.1.4.1.8072.2.2048.1.2.1.1.1 -> Input Voltage
.1.3.6.1.4.1.8072.2.2048.1.3.0 -> Port Index
.1.3.6.1.4.1.8072.2.2048.1.3.1 -> Port Names
.1.3.6.1.4.1.8072.2.2048.1.3.2 -> Port Status
.1.3.6.1.4.1.8072.2.2048.1.3.3 -> Port Current
.1.3.6.1.4.1.8072.2.2048.1.3.4 -> Port Power
```

Please note that SNMP is dumb (IMO) and when interacting with programs like Cacti that expect data to be formatted and organized in a certain way, when adding new data to the OID tree, I may have to reorganize, and thus I do not guarantee the OIDs will remain the same with any updates to this script. Sorry!

## Cacti
The Cacti folder contains some example configuration/templates for Cacti to use to create graphs based on the SNMP data available through the above scripts from the PDU. These work for me and have been exported from my working config, but may not work for you. Please take them as references/examples.