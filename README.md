libLTE
========

libLTE is a free and open-source LTE library for SDR UE and eNodeB. The library does not rely on any external dependencies or frameworks. 

The license is LGPLv3.

The project contains a set of Python tools for the automatic code generation of modules for popular SDR frameworks, including GNURadio, ALOE++, IRIS, and OSSIE. These tools are easy to use and adapt for generating targets for specific platforms or frameworks. 

Current Status
===============

The following parts are available:
 * Physical Channels: PBCH, PCFICH, PDCCH, PHICH, PDSCH eNodeB and UE side
 * Synchronization and CFO estimation/correction
 * Equalization
 * UE receiver (MIB + SIB1 decoding) verified with live LTE signals

Hardware
========

The library currently uses Ettus Universal Hardware Driver (UHD). Thus, any hardware supported by UHD can be used. There is no sampling rate conversion, therefore the hardware should support 30.72 MHz clock in order to work correctly with LTE sampling frequencies and decode signals from live LTE base stations. We are using the B210 USRP. 


Download & Install Instructions
=================================

* Requirements: Currently, the library requires libfftw, although we plan make this dependency optional in the future. Also, QT4 and Qwt6 are needed for graphics visualization. Compilation is possible without QT4, although graphics will be disabled.

To install QT4, Qwt6 and libfftw use your distribution packet management system, for instance in ubuntu you can run: `sudo apt-get install libfftw3-dev libqwt-dev libqt4-dev` to install all requirements. 


Finally, to download and build libLTE, just run: 
```
git clone https://github.com/ismagom/libLTE.git
cd libLTE
mkdir build
cd build
cmake ../
make 
```

The library can also be installed using the command ```sudo make install```. 

PHY Examples
==========

* eNodeB and UE PBCH example

Setup one or two computers connected to two USRP or UHD-compatible hardware. From the eNodeB, type

```
lte/phy/examples/pdsch_enodeb -f [frequency_in_Hz] -c [cell_id] [-a [UHD args]] [-h for more commands]
```

From the UE, type 
```
lte/phy/examples/pdsch_ue -f [frequency_in_Hz] [-a [UHD args]] [-h for more commands]
```

And the output should look something like the following video. In this example, we removed the transmitter and receiver antennas in the middle of the demonstration, showing how reception is still possible (despite with some erros). 

https://www.dropbox.com/s/txh1nuzdb0igq5n/demo_pbch.ogv

![Screenshopt of the PBCH example output](pbch_capture.png "Screenshopt of the PBCH example output")


The SIB1 message is decoded and shown on the console, for example: 

```
Decoded SIB1 Message: [40 48 50 03 02 0b 14 4a 30 18 28 20 90 81 84 79 a0 00 ];
```

Then, you can use any ASN.1 SIB decoder to read the message. This site http://www.marben-products.com/asn.1/services/decoder-asn1-lte.html is a good example. 


If you don't have a pair of USRP, you can also test the demo by writing the samples to a file and then reading them: 

From the eNodeB, type

```
lte/phy/examples/pdsch_enodeb -o [output_file] -c [cell_id] [-h for more commands]
```

From the UE, type 
```
lte/phy/examples/pdsch_ue -i [input_file] -c [cell_id] [-h for more commands]
```


* Cell Search Example

This program uses any hardware supported by the UHD driver to scan an LTE band for active cells. See http://niviuk.free.fr/lte_band.php for a list of available bands. The program first obtains a power spectral density of the entire band. For all frequencies with an RSSI higher than a threshold, it tries to find the LTE Primary Synchronization Signal (PSS) and then identifies the CELL ID using the Secondary Synchronization Signal (SSS). Finally, it estimates the Carrier Frequency Offset (CFO) and Sampling Frequency Offset (SFO) and decodes the Master Information Block (MIB) from the PBCH. 

For instance, the command:

``` lte/phy/examples/scan_mib -b 3 ```


Scans the LTE band 3 (1805 to 1880 MHz). Note that you need a hardware supporting these frequencies (e.g. SBX daughterboard for USRP). For more command arguments, type ``` examples/mib_scan_usrp -h ```



Support
========

Mailing list: https://lists.sourceforge.net/lists/listinfo/liblte-users
