# ez-sniff : a software-defined radio (SDR) utility
Demodulate and dissect radio frequency (RF) packets from toll booth transponders in the E-ZPass network

## Disclaimer
* All provided demo E-ZPass packets were captured with explicit permission of the owner.
* Toll booth evasion is illegal, and E-ZPass has infrastructure in place to catch people who attempt to leverage this code to perform attacks.
* I don't take responsibility for anything you do with this code.

## Table of Contents
1. [Introduction](#introduction)
2. [Setup](#setup)
   - [Dependencies](#dependencies)
   - [Building](#building)
3. [Usage](#usage)
   - [Receiving and Analyzing](#receiving_and_analyzing)
   - [Transmitting or Cloning](#transmitting_or_cloning)
   - [C++ Script](#c++_script)
4. [Licenses](#licenses)

## Introduction
Background
osmo-fl2k wiki, purchase, library link


This project began as part of the interview process for a summer internship (which graciously let me keep rights to the code). I continued the research to investigate transmitting when I heard of the Osmo-FL2k.

I gave a presentation on this research at BSides Columbus 2019. A video of the presentation and slides is accessible on [this page at IronGeek.](https://www.irongeek.com/i.php?page=videos/bsidescolumbus2019/bsidescmh2019-3-01-e-zhack-an-update-on-sdr-and-toll-booth-reverse-engineering-kyle-westhaus)

## Setup
This program is currently provided for Linux systems and has been tested on Manjaro with the 4.19 kernel and GNURadio 3.7.13.4.
### Dependencies
This project makes use of other libraries including:
* [Daniel Bahr's single-header CRC++](https://github.com/d-bahr/CRCpp) (included in this repo)
* Boost::Multiprecision for uint256_t (not included in this repo)
Boost libraries can be obtained either from your system's package manager or according to their [Getting Started page.](https://www.boost.org/doc/libs/1_69_0/more/getting_started/unix-variants.html)
* The files in the grc directory rely on GNURadio Companion (GRC) (not included in this repo)
The GNURadio set of tools can likely be installed from your package manager, which is typically much simpler than building from source.

### Building
Begin by cloning the repo. Assuming you have the Boost header libraries properly installed, compiling should then be as simple as:
```console
cd src
g++ ./ezp-dissect.cpp -o ./ezp-dissect.out
```

## Usage
This code serves as the software to reverse engineer and experiment with E-ZPass toll booth transponders in various ways. Uses include:
### Receiving and Analyzing
1. Use an SDR device to record raw RF samples on the appropriate frequency (usually 914.3-916.0MHz) while located near a transponder passing through a toll booth.
2. Adjust the variables for LISTOFVARIABLES in one of either NEWFILENAMES for your specific use case.
3. Process the raw capture file with the adjusted GRC flowchart.
4. Dissect and analyze the processed file with the compiled C++ script to read transponder packet data.
This process was tested using captures from an RTL-SDR sampling at 3.2MHz, and a lab-grade SDR sampling at 10MHz.

### Transmitting or Cloning
1. Modify the variables for LISTOFVARIABLES in the OUTPUTGRC file for your specific use.
2. Process a packet data file (either from capturing with SDR or a test packet from the C++ script) with the OUTPUTGRCFILENAME GRC flowchart.
3. Transmit the raw RF sample file produced with a capable SDR device. This program currently outputs 8-bit SIGNEDORNOT? samples for use with an Osmo-Fl2k device.

### C++ Script
Command-line usage for the C++ processing and analysis script is as follows:
```

*** Usage: ***
ezp-dissect [-i "input file name"] [-g] [--grc-file] [-s] [--short-file] [-o "output file name"]

File operations:
	-i "input file name"	Input packet file to read and analyze
	-o "output file name"	Output packet file to craft and write
	** Note: valid options include -i only (read and display), -o only (craft, display, and write),
	**     or both -i and -o (read GRC file, output as short file)
	** If both -i and -o are specified, --grc-file is assumed and should not be specified (nor -g)

Input file formatting:
	-g, --grc-file		Read in a file output by a GnuRadio flowchart.
				** Expects unpacked (one bit per byte), Manchester encoded, and non-cut data
	-s, --short-file	Reads in a file containing only the exact packet
				** Expects a 256bit MSB-first file
				** Test packets output by this program conform to this option

***Examples:***
	ezp-dissect -i reader_cap.ezp --short-file
	ezp-dissect -i xponder_ohioturnpike.grc.bytes -g
	ezp-dissect -o reader_buzzHMI.ezp
	ezp-dissect -i xponder_nonconformant.grc.bytes -o xponder-nonconformant.ezp


```

## Licenses
This code is free to use and available under the GPLv3 license. Full license texts for both this project and associated libraries are located in the LICENSES directory.