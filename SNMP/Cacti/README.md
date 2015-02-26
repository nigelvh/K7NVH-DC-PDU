Cacti
=======

These are some example graph templates (with (in theory) dependencies included) for Cacti to graph SNMP provided PDU data.

## k7nvh_pdu.xml
This xml file defines how to query the indexed SNMP data. Usually this file gets installed in a default cacti folder for SNMP queries. On my Ubuntu install, this was located at /usr/share/cacti/resource/snmp_queries

## cacti_graph_template_pdu_-_port_current.xml
## cacti_graph_template_pdu_-_port_power.xml
## cacti_graph_template_pdu_-_voltage.xml
These xml files are graph templates, and can be imported into cacti via the standard web interface via the Import Templates option.

These configs are taken from my working installation of Cacti, however, I have seen issues with template import/export in the past, so you may run into issues, so please take them as a reference/starting point. The parent folder dealing with the raw SNMP side of this equation provides the actual data. How you wish to capture and/or display that data will vary based on your use case.