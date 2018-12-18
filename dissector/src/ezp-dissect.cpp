/*******************************************************/
/* BEGIN HEADER FILE INCLUDES, NAMESPACES, AND DEFINES */
/*******************************************************/

// For cin, cout, etc
#include <iostream>
// For ifstream, ofstream, etc
#include <fstream>
// For string
#include <string>
// For find
#include <algorithm>
// For numeric_limits
#include <limits>
// For uint16_t
#include <cstdint>
// For bitset
#include <bitset>

// For uint256_t
#include <boost/multiprecision/cpp_int.hpp>

// For CRC-XMODEM implementation
#include "CRC.h"


// For cin, cout, string, map, etc.
using namespace std;
// For uint256_t
using namespace boost::multiprecision;


// Define a few values to reduce amount of magic numbers
// Sum total of known-length fixed sections is 47 bits
#define KNOWN_FIXED_LENGTH 47
// Smallest byte-aligned boundary after KNOWN_FIXED_LENGTH
#define BOUNDARY_MIN 48
// Largest byte-aligned boundary after KNOWN_FIXED_LENGTH which still leaves at least 1 byte for
//   Programmable Block 1 and 64 bits for Programmable Block 2 and CRC
#define BOUNDARY_MAX 184
// Used many times for bit operations
#define BYTE_LEN 8

/*****************************************************/
/* END HEADER FILE INCLUDES, NAMESPACES, AND DEFINES */
/*****************************************************/


/*****************************/
/* BEGIN FUNCTION PROTOTYPES */
/*****************************/

// Initialize string explanations for integer values of some data fields
void initSectionStringLookups(map<int, string>* mTagType, map<int, string>* mApplicationID);

// Read in a GNURadioCompanion-output file and convert to 256bit packet structure
uint256_t readGRCPacket(ifstream* ifPacket);

// Read 256bit binary packet file
uint256_t readShortPacket(ifstream* ifPacket);

// Read in a file, dissect packet, and display output
void dispPacket(uint256_t uPacket);

// Prompt user and pack data sectors into 256bit packet structure
uint256_t craftPacket();

// Write packet to specified output file
void writePacket(uint256_t uPacket, ofstream* ofPacket);

// Check if a given string exists in the command line arguments
bool argExists(vector<string>* vArgs, string sOption);

// If a string exists in the command line arguments, get its accompanying value
string getArg(vector<string>* vArgs, string sOption);

// Calculates and returns CRC for first 240 bits of a packet
uint16_t calcPacketCRC(uint256_t uPacket);

// Evaluates a packet's CRC and prompts user to continue or quit if CRC does not match
bool checkPacketCRC(uint256_t uPacket);

/***************************/
/* END FUNCTION PROTOTYPES */
/***************************/


/**************/
/* BEGIN MAIN */
/**************/

int main(int argc, char* argv[])
{
	// Declare input and output streams
	ifstream ifPacket;
	ofstream ofPacket;

	vector<string> vArgs(argv+1, argv+argc);

	enum direction
	{
		INPUT_BAD,
		READ_GRC,
		CRAFT_TEST,
		READ_SHORT,
		CONVERT_GRC
	} eUserDirection;
	eUserDirection = INPUT_BAD;

	// Check for reading a GRC-output file as input
	if( argExists(&vArgs, "-i") && ( argExists(&vArgs, "--grc-file") || argExists(&vArgs, "-g") ) && argc == 4 )
	{
		// Get input file name and open in binary mode
		string strInFileName = getArg( &vArgs, "-i" );
		ifPacket.open(strInFileName, ios::in|ios::binary);
		// Make sure input file exists. Otherwise, output error and quit
		if( !ifPacket.good() )
		{
			cerr << "ERROR: Input file \"" << strInFileName << "\" could not be opened" << endl;
		// If only input file is specified and file opened properly, command line syntax is ok
		} else
		{
			eUserDirection = READ_GRC;
		}	
	// Check for outputting a short packet test file
	} else if(argExists(&vArgs, "-o") && argc == 3)
	{
		// Get output file name and open in binary mode
		string strOutFileName = getArg(&vArgs, "-o");
		ofPacket.open(strOutFileName, ios::out|ios::binary);
		// Make sure output file can be opened. Otherwise, output error and quit
		if( !ofPacket.good() )
		{
			cerr << "ERROR: Output file \"" << strOutFileName << "\" could not be opened" << endl;
		// If only output file is specified and file opened properly, command line syntax is ok
		} else
		{
			eUserDirection = CRAFT_TEST;
		}
	// Check for reading short packet file
	} else if( argExists(&vArgs, "-i") && ( argExists(&vArgs, "--short-file") || argExists(&vArgs, "-s") ) && argc == 4 )
	{
		// Get input file name and open in binary mode
		string strInFileName = getArg( &vArgs, "-i" );
		ifPacket.open(strInFileName, ios::in|ios::binary);
		// Make sure input file exists. Otherwise, output error and quit
		if( !ifPacket.good() )
		{
			cerr << "ERROR: Input file \"" << strInFileName << "\" could not be opened" << endl;
		} else
		{
			eUserDirection = READ_SHORT;
		}
	// Check for reading a GRC-output file as input, and outputting it as a short packet
	} else if( argExists(&vArgs, "-i") && argExists(&vArgs, "-o") && argc == 5 )
	{
		// Get input file name and open in binary mode
		string strInFileName = getArg( &vArgs, "-i" );
		ifPacket.open(strInFileName, ios::in|ios::binary);
		// Get output file name and open in binary mode
		string strOutFileName = getArg( &vArgs, "-o");
		ofPacket.open(strOutFileName, ios::out|ios::binary);
		// Make sure input file exists. Otherwise, output error and quit
		if( !ifPacket.good() )
		{
			cerr << "ERROR: Input file \"" << strInFileName << "\" could not be opened" << endl;
		} else if( !ofPacket.good() )
		{
			// Make sure output file exists. Otherwise, output error and quit
			cerr << "ERROR: Output file \"" << strOutFileName << "\" could not be opened" << endl;
		} else
		{
			eUserDirection = CONVERT_GRC;
		}
	} // end if-else ladder for eUserDirection checks

	if(eUserDirection == READ_GRC)
	{
		// If syntax is ok and program is in read GRC packet mode, display packet from file
		uint256_t uPacket = readGRCPacket( &ifPacket );
		bool bContinue = checkPacketCRC( uPacket );
		if( !bContinue )
		{
			ifPacket.close();
			exit(1);
		}
		dispPacket( uPacket );
		// Safely close file
		ifPacket.close();
	} else if(eUserDirection == CRAFT_TEST)
	{
		// If syntax is ok and program is in craft test packet mode, craft test packet and write to short packet file
		uint256_t uPacket = craftPacket();
		writePacket(uPacket, &ofPacket);
		// Safely close file
		ofPacket.close();
	} else if(eUserDirection == READ_SHORT)
	{
		// If syntax is ok and program is in read short packet mode, display packet from file
		uint256_t uPacket = readShortPacket( &ifPacket );
		bool bContinue = checkPacketCRC( uPacket );
		if( !bContinue )
		{
			ifPacket.close();
			exit(1);
		}
		dispPacket( uPacket );
		// Safely close file
		ifPacket.close();
	} else if(eUserDirection == CONVERT_GRC)
	{
		uint256_t uPacket = readGRCPacket( &ifPacket);
		bool bContinue = checkPacketCRC( uPacket );
		if( !bContinue )
		{
			ifPacket.close();
			exit(1);
		}
		dispPacket( uPacket );
		// Safely close file
		ifPacket.close();
		writePacket(uPacket, &ofPacket);
		ofPacket.close();
	} else
	{
		// If syntax is not ok, output program options and exit with error
		cout << endl;
		cout << "*** Usage: ***" << endl;
		cout << "ezp-dissect [-i \"input file name\"] [-g] [--grc-file] [-s] [--short-file] [-o \"output file name\"]" << endl;
		cout << endl;
		cout << "File operations:" << endl;
		cout << "	-i \"input file name\"\t" << "Input packet file to read and analyze" << endl;
		cout << "	-o \"output file name\"\t" << "Output packet file to craft and write" << endl;
		cout << "	** Note: valid options include -i only (read and display), -o only (craft, display, and write)," << endl;
		cout << "	**     or both -i and -o (read GRC file, output as short file)" << endl;
		cout << "	** If both -i and -o are specified, --grc-file is assumed and should not be specified (nor -g)" << endl;
		cout << endl;
		cout << "Input file formatting:" << endl;
		cout << "	-g, --grc-file\t\t" << "Read in a file output by a GnuRadio flowchart." << endl;
		cout << "\t\t\t\t** Expects unpacked (one bit per byte), Manchester encoded, and non-cut data" << endl;
		cout << "	-s, --short-file\t" << "Reads in a file containing only the exact packet" << endl;
		cout << "\t\t\t\t** Expects a 256bit MSB-first file" << endl;
		cout << "\t\t\t\t** Test packets output by this program conform to this option" << endl;
		cout << endl;
		cout << "***Examples:***" << endl;
		cout << "	ezp-dissect -i reader_cap.ezp --short-file" << endl;
		cout << "	ezp-dissect -i xponder_ohioturnpike.grc.bytes -g" << endl;
		cout << "	ezp-dissect -o reader_buzzHMI.ezp" << endl;
		cout << "	ezp-dissect -i xponder_nonconformant.grc.bytes -o xponder-nonconformant.ezp" << endl;
		cout << endl;
		exit(1);
	}

	cout << "Program completed successfully. Quitting..." << endl;
}

/************/
/* END MAIN */
/************/


/******************************/
/* BEGIN FUNCTION DEFINITIONS */
/******************************/

// Initialize string explanations for integer values of some data fields
void initSectionStringLookups(map<int, string>* mTagType, map<int, string>* mApplicationID, map<int, string>* mAgencyID, map<int, string>* mGroupID)
{
	// Set explanation text for each type of tag
	(*mTagType)[0] = "Standard windshield mount transponder";
	(*mTagType)[1] = "Roof mount transponder";
	(*mTagType)[2] = "External mount transponder";
	(*mTagType)[3] = "Data-specific windshield mount transponder (depends on issuing Agency)";
	(*mTagType)[4] = "Commercial vehicle windshield mount transponder";
	(*mTagType)[5] = "HMI windshield mount transponder";
	(*mTagType)[6] = "HOV/HOT windwhield mount transponder";
	(*mTagType)[7] = "Other tag type, encoded in Agency Fixed data field";

	// Set explanation text for each application ID
	(*mApplicationID)[0] = "Test unit";
	(*mApplicationID)[1] = "Toll Collection unit";
	(*mApplicationID)[2] = "Reserved for future use/unknown";
	(*mApplicationID)[3] = "Reserved for future use/unknown";
	(*mApplicationID)[4] = "Reserved for future use/unknown";
	(*mApplicationID)[5] = "Reserved for future use/unknown";
	(*mApplicationID)[6] = "Reserved for future use/unknown";
	(*mApplicationID)[7] = "Reserved for future use/unknown";

	(*mAgencyID)[0] = "Manufacturer (Kapsch)";
	(*mAgencyID)[10] = "Virginia DOT";
	(*mAgencyID)[31] = "Ohio Turnpike and Infrastructure Commission (OTIC)";

	(*mGroupID)[65] = "E-ZPass Interagency Group";

	return;
} // end initSectionStringLookups(map<int, string>*, map<int, string>*, map<int, string>*, map<int, string>*

// Read in a GNURadioCompanion-output file and convert to 256bit packet structure
uint256_t readGRCPacket(ifstream* ifPacket)
{
	// This function will search for and use the first valid transmission in a file
	
	if( !ifPacket->good() )
	{
		cerr << "Error in input file, quitting..." << endl;
		exit(1);
	}
	
	cout << "Reading packet file ..." << endl << endl;
	// Assume:
	//   that ifPacket.good() has already been checked
	//   that only one packet is contained in the file
	
	// Create new uint packet structure and uint temporary reader variable
	// Use uint16_t since uint8_t likes to output as a char rather than an int, and casting can get complex
	uint256_t uPacket = 0;
	uint16_t uTempReader = 0;

	// Initialize variables to find transmission position in file
	bool bPacketFound = 0;
	int nPacketStart = -1;
	int nPacketLength = 0;
	int nConsecZeros = 0;
	int nConsecOnes = 0;

	// Check the length of the file
	ifPacket->seekg(0, ios::end);
	int nFileLength = ifPacket->tellg();
	ifPacket->seekg(ios::beg);

	if(nFileLength < 512)
	{
		cerr << "Error: File too short to contain a valid packet. Quitting..." << endl;
		ifPacket->close();
		exit(1);
	}

	// Search through the file until we find either a burst matching Manchester-encoded packet 
	//   characteristics or the end of the file
	while( !bPacketFound && ( ifPacket->tellg() < nFileLength ) )
	{
		ifPacket->read(reinterpret_cast<char*>(&uTempReader), 1);
		// Case 1 - Bit is a 1, and NOT in the middle of checking a burst
		if( uTempReader != 0 && nPacketStart == -1 )
		{
			// Reset consecutive zeros, consecutive ones, and packet length, and set current position 
			//   to possible packet start position
			nConsecZeros = 0;
			nConsecOnes = 1;
			nPacketStart = ifPacket->tellg();
			nPacketLength = 0;

			cout << "\tTesting possible packet starting at position " << nPacketStart << endl;

		// Case 2 - Bit is a 1, and already in the middle of checking a burst
		} else if( uTempReader != 0 && nPacketStart != -1 )
		{
			// Reset consecutive zeros and increment consecutive ones
			nConsecZeros = 0;
			nConsecOnes++;
			// Three ones in a row, which doesn't occur in a Manchester-encoded packet
			if( nConsecOnes == 3 )
			{
				// Reset possible packet start position and number of consecutive ones
				nPacketStart = -1;
				nConsecOnes = 0;
				cout << "\tPossible packet failed for reason: three consecutive 1 bits" << endl << endl;
			} else
			{
				// If this isn't the third 1 in a row, just increment the packet length
				nPacketLength++;
			}

		// Case 3 - Bit is a 0, and NOT in the middle of checking a burst
		} else if( uTempReader == 0 && nPacketStart == -1 )
		{
			// Ignore and move forward

		// Case 4 - Bit is a 0, and already in the middle of checking a burst
		} else if( uTempReader == 0 && nPacketStart != -1 )
		{
			// Reset number of consecutive ones and increment number of consecutive zeros and length		
			nConsecOnes = 0;
			nConsecZeros++;
			nPacketLength++;
			// Three zeros in a row, which doesn't occur in a Manchester-encoded packet
			if(nConsecZeros == 3)
			{
				// Check the current length to see if we've reached the expected end of a packet, or if
				//   we've reached the end of something else we can safely ignore (like the trigger pulse)
				if( nPacketLength > 510 && nPacketLength < 518 )
				{
					// Expected packet size is 512 bytes (256 bits, 2 symbols per bit, currently stored as
					//   1 byte per symbol). If this burst structure is within reasonable limits of this
					//   length (accounting for the fact that we extend a bit past the end of packet to check
					//   for consecutive zeros to confirm packet end), then we've found a packet and can
					//   quit looping.
					bPacketFound = 1;
					cout << "\tPacket found at position " << nPacketStart << endl << endl;
				} else
				{
					// Size of this burst doesn't correspond to expected packet length. Reset packet start,
					//   consecutive zeros, and consecutive ones, and keep looping.
					nPacketStart = -1;
					nConsecZeros = 0;
					nConsecOnes = 0;
					cout << "\tPossible packet failed for reason: incorrect packet length: " << nPacketLength << endl << endl;
				}
			}
		}
	} // end while
	if( ifPacket->tellg() == nFileLength && nPacketLength < 511)
	{
		cerr << "Error: No valid transmissions could be found in input file. Consider re-processing input file or adjusting the source code of this program to accept packets of alternate lengths or encodings." << endl;
		ifPacket->close();
		exit(1);
	}
	ifPacket->seekg(nPacketStart-1);
	for(int nReadCount = 0; nReadCount < 256; nReadCount++)
	{
		// At this point, each byte output from GRC contains only 1 bit
		// Read in one byte from file to temporary reader variable
		ifPacket->read(reinterpret_cast<char*>(&uTempReader), 1);
		// Left-shift packet structure one bit
		//   This is an acceptable amount even though uTempReader is 16bits because each file byte will
		//   only contain a value the Least Significant Bit
		uPacket <<= 1;
		// Bitwise-or to store temporary reader value into the packet structure
		uPacket |= uTempReader;
		// Read in another byte, which will just be overwritten and ignored
		//   This works because we only need the first symbol of each bit to determine the bit's value
		//   and can safely ignore the second symbol
		ifPacket->read(reinterpret_cast<char*>(&uTempReader), 1);
	} // end for

	return uPacket;
} // end readGRCPacket(ifstream*)

// Read 256bit binary packet file
uint256_t readShortPacket(ifstream* ifPacket)
{
	// This function expects a 256bit file containing a valid packet and no extra space
	
	if( !ifPacket->good() )
	{
		cerr << "Error in input file, quitting..." << endl;
		exit(1);
	}

	ifPacket->seekg(0, ios::end);
	int nFileLength = ifPacket->tellg();
	if(nFileLength != 32)
	{
		cerr << "Error: short packet file is not correct length! Expected 256bits/32bytes. Quitting..." << endl;
		exit(1);
	}
	ifPacket->seekg(0, ios::beg);
	cout << "Reading packet file ..." << endl << endl;
	// Assume:
	//   that ifPacket.good() has already been checked
	//   that only one packet is contained in the file
	
	// Create new uint packet structure and uint temporary reader variable
	// Use uint16_t since uint8_t likes to output as a char rather than an int
	uint256_t uPacket = 0;
	uint16_t uTempReader = 0;

	for(int nReadCount = 0; nReadCount < 32; nReadCount++)
	{
		ifPacket->read(reinterpret_cast<char*>(&uTempReader), 1);
		uPacket <<= BYTE_LEN;
		uPacket |= uTempReader;
	}
	return uPacket;
} // end readShortPacket(ifstream*)

// Read in a file, dissect packet, and display output
void dispPacket(uint256_t uPacket)
{
	cout << endl << "Displaying packet ..." << endl << endl;

	// Declare offset value for which bit of uPacket we need to shift to be the LSB when reading each data field
	int nShift = 253;
	// Declare maps for Tag Type and Application ID, which act as a form of dictionary
	//   i.e., offers a string interpretation of unsigned values read from file
	map<int, string> mTagType, mApplicationID, mAgencyID, mGroupID;
	initSectionStringLookups(&mTagType, &mApplicationID, &mAgencyID, &mGroupID);

	// Read first (most significant) 3 bits as Header
	uint8_t uHeader = static_cast<uint8_t>(uPacket >> nShift & 0x07u);
	// Read next 3 bits as Application ID
	nShift -= 3;
	uint8_t uTagType = static_cast<uint8_t>(uPacket >> nShift & 0x07u);
	// Read next 3 bits as Application ID
	nShift -= 3;
	uint8_t uAppID = static_cast<uint8_t>(uPacket >> nShift & 0x07u);
	// Read next 7 bits as Group ID
	nShift -= 7;
	uint8_t uGrpID = static_cast<uint8_t>(uPacket >> nShift & 0x7Fu);
	// Read next 7 bits as Agency ID
	nShift -= 7;
	uint8_t uAgID = static_cast<uint8_t>(uPacket >> nShift & 0x7Fu);
	// Read next 24 bits as Serial Number
	nShift -= 24;
	uint32_t uSerial = static_cast<uint32_t>(uPacket >> nShift & 0x00FFFFFFu);

	// Initialize variables for reading Agency Fixed and Programmable Block 1 sections
	bool bDividerKnown = 0;
	int nAgFixLength = 0;
	uint256_t uAgFixMask = 0;
	uint256_t uProg1Mask = 0;
	uint256_t uProg1_AgFix_Comb = 0;
	uint256_t uProg1 = 0;
	uint256_t uAgFix = 0;
	uint16_t uVehClass = 0;
	uint8_t uUnkField = 0;
	// If packets from these agencies have been tested before, we know the packet layout
	if( uAgID == 10 || uAgID == 31 )
	{
		bDividerKnown = 1;
		nAgFixLength = 17;
		uAgFixMask = 0x0001FFFFu;
	} else
	{
		// Otherwise, prompt user if they know the packet layout for this agency
		cout << "Agency ID: " << +uAgID << endl;
		cout << "Packets from this Agency have not been examined before, and therefore the byte-aligned boundary between the Agency Fixed and Programmable Block 1 sections is unknown. Unless you specify the boundary, the two sections will be output together as decimal, hex, and binary." << endl;
		bool bInputOk = 0;
		string strInput;
		do
		{
			cout << "Do you know the boundary? (y/n): ";
			cin >> strInput;
			if( strInput == "y" || strInput == "n" )
			{
				bInputOk = 1;
			}
		} while( !bInputOk );
		if(strInput == "y")
		{
			bDividerKnown = 1;
			bInputOk = 0;

			// The border between the Agency Fixed and Programmable Block 1 data fields is left up to each agency,
			//   with the only restricitons being that it must occur at the end of a byte and occur between the
			//   Serial Number and HOV/Feedback Flag data fields
			//
			//   Because of this, the user is prompted for an acceptable bit to set the border at before prompting
			//     for data for each of these fields
			int nDivider;
			while( !bInputOk )
			{
				cout << "Enter the bit location of the boundary between the Agency Fixed and Programmable Block 1 sections (48-184): ";
				if( !(cin >> nDivider) )
				{
					cin.clear();
					cin.ignore(numeric_limits<streamsize>::max(), '\n');
					cout << "Error: Improper type, please try again" << endl;
				} else if(nDivider < BOUNDARY_MIN || nDivider > BOUNDARY_MAX)
				{
					cout << "Error: Improper value, please try again" << endl;
				} else if( nDivider % BYTE_LEN )
				{
					cout << "Error: Not a byte-aligned (multiple of 8) bit, please try again" << endl;
				} else
				{
					bInputOk = 1;
				}
			}
			nAgFixLength = nDivider-KNOWN_FIXED_LENGTH;
			// Create a mask variable that will be used to select the bits we need
			for(int nMaskBit = 0; nMaskBit < nAgFixLength; nMaskBit++)
			{
				uAgFixMask <<= 1;
				uAgFixMask |= 1;
			}
		} // end if
		
	} // end if-else
	if(bDividerKnown)
	{
		// If the divider is known or user-specified, we can split the Agency Fixed and Programmable Block 1 sections
		//   using the section length and mask we generated earlier
		nShift -= nAgFixLength;
		uAgFix = uPacket >> nShift & uAgFixMask;
		// Special cases for packets from well-studied agencies
		if(uAgID == 10 || uAgID == 31)
		{
			uVehClass = static_cast<uint16_t>(uAgFix >> 6);
			uUnkField = static_cast<uint8_t>(uAgFix & 0x3Fu);
		}
		// Next, calculate length and mask for Programmable Block 1, then read it
		int nProg1Length = 145 - nAgFixLength;
		for(int nMaskBit = 0; nMaskBit < nProg1Length; nMaskBit++)
		{
			uProg1Mask <<= 1;
			uProg1Mask |= 1;
		}
		nShift -= nProg1Length;
		uProg1 = uPacket >> nShift & uProg1Mask;
	} else
	{
		// Otherwise, read Agency Fixed and Programmable Block 1 sections as one large chunk
		
		// Up to this point, we have been specifying the exact bits to read with a bitwise and,
		//   along with a literal unsigned int that serves as a mask
		//   The Agency Fixed and Programamble Block 1 fields have the possibility to be large enough
		//   to require a mask that is larger than the maximum unsigned int literal
		//
		//   To get around this, a 256bit mask structure is created. The mask is fed 64bit unsigned
		//   literals at a time, and left-shifted inbetween in order to fill it properly
		uint256_t uMask = 0x1FFFFu;
		uMask <<= 64;
		uMask |= 0xFFFFFFFFFFFFFFFFu;
		uMask <<= 64;
		uMask |= 0xFFFFFFFFFFFFFFFFu;
		// Read next 145 bits as combined Agency Fixed and Programmable Block 1 data fields
		nShift -= 145;
		uProg1_AgFix_Comb = uPacket >> nShift & uMask;
	}

	// Read next 8 bits as HOV and Feedback Flags
	nShift -= 8;
	uint8_t uHovFeed = static_cast<uint8_t>(uPacket >> nShift & 0xFFu);
	// Read next 40 bits as Programmable Block 2
	nShift -= 40;
	uint64_t uProg2 = static_cast<uint64_t>(uPacket >> nShift & 0x000000FFFFFFFFFFu);
	// Read lowest 16 bits as CRC (checksum value)
	// This value has already been calculated before dispPacket() is called, so we don't need to check again
	uint16_t uCRC = static_cast<uint16_t>(uPacket & 0xFFFFu);

	// Output calculated values with pretty data field headers
	cout << "*** Begin Packet ***" << endl << endl;

	// Preface uint8_t values with a + sign so that they output as unsigned ints rather than chars
	cout << "*** Section 1: Factory Fixed ***" << endl;
	cout << "\tHeader: " << +uHeader << endl;
	cout << "\tTag Type: " << mTagType[ +uTagType ] << " (" << +uTagType << ")" << endl;
	cout << "\tApplication ID: " << mApplicationID[ +uAppID ] << " (" << +uAppID << ")" << endl;
	cout << "\tGroup ID: " << mGroupID[ +uGrpID ] << " (" << +uGrpID << ")" << endl;
	cout << "\tAgency ID: " << mAgencyID[ +uAgID ] << " (" << +uAgID << ")" << endl;
	cout << "\tSerial Number: " << uSerial << endl;
	cout << "*** End Section 1: Factory Fixed ***" << endl << endl;
	
	// Output Agency Fixed and Reader Programmable sections as one if boundary unknown
	if( !bDividerKnown )
	{
		cout << "*** Section 2: Agency Fixed and Section 3: Reader Programmable ***" << endl;
		cout << "\tAgency Fixed and Programmable Block 1:" << endl << "\t\tDecimal: " << uProg1_AgFix_Comb << endl << "\t\tHex: " << hex << uProg1_AgFix_Comb << dec << endl;
		// Mask combined field to 64bit subsections so that we can initialize bitsets to output binary
		uint32_t uHigh = static_cast<uint32_t>(uProg1_AgFix_Comb >> 128);
		uint64_t uMid = static_cast<uint64_t>(uProg1_AgFix_Comb >> 64 & 0xFFFFFFFFFFFFFFFFu);
		uint64_t uLow = static_cast<uint64_t>(uProg1_AgFix_Comb & 0xFFFFFFFFFFFFFFFFu);
		cout << "\t\tBinary: " << bitset<17>(uHigh) << bitset<64>(uMid) << bitset<64>(uLow) << endl;
	}else if(uAgID == 10 || uAgID == 31)
	{
		// Specific output for packets from well-studied agencies
		cout << "*** Section 2: Agency Fixed ***" << endl;
		cout << "\tVehicle Class: " << uVehClass << endl;
		cout << "\tUnknown Field: " << +uUnkField << endl;
		cout << "*** End Section 2: Agency Fixed ***" << endl << endl;

		cout << "*** Section 3: Reader Programmable ***" << endl;
		cout << "\tProgrammable Block 1: " << endl << "\t\tDecimal: " << uProg1 << endl << "\t\tHex: " << hex << uProg1 << dec << endl;
		// Mask field to 64bit subsections so that we can initialize bitsets to output binary
		uint64_t uHigh = static_cast<uint64_t>(uProg1 >> 64 & 0xFFFFFFFFFFFFFFFFu);
		uint64_t uLow = static_cast<uint64_t>(uProg1 & 0xFFFFFFFFFFFFFFFFu);
		cout << "\t\tBinary: " << bitset<64>(uHigh) << bitset<64>(uLow) << endl;
	} else
	{
		cout << "*** Section 2: Agency Fixed ***" << endl;
		cout << "\tAgency Fixed: " << endl << "\t\tDecimal: " << uAgFix << endl << "\t\tHex: " << hex << uAgFix << dec << endl;
		cout << "*** End Section 2: Agency Fixed ***" << endl << endl;
		cout << "*** Section 3: Reader Programmable ***" << endl;
		cout << "\tProgrammable Block 1: " << endl << "\t\tDecimal: " << uProg1 << endl << "\t\tHex: " << hex << uProg1 << dec << endl;
	}
	cout << "\tHOV and Feedback Flags: " << +uHovFeed << endl;
	cout << "\tProgrammable Block 2: " << endl << "\t\tDecimal: " << uProg2 << endl << "\t\tHex: " << hex << uProg2 << dec << endl << "\t\tBinary: " << bitset<40>(uProg2) << endl;
	cout << "*** End Section 3: Reader Programmable ***" << endl << endl;

	cout << "*** Section 4: CRC ***" << endl;
	cout << "\tCRC (in hex): " << hex << uCRC << dec << endl;
	cout << "*** End Section 4: CRC ***" << endl << endl;

	cout << "*** End Packet ***" << endl;
	
	return;
} // end dispPacket(uint256_t)

// Prompt user and pack data sectors into 256bit packet structure
uint256_t craftPacket()
{
	cout << "Crafting packet ..." << endl << endl;
	uint256_t uPacket = 0;
	// Create boolean flag so program only continues when user has entered a proper value
	bool bInputOk = 0;
	
	// Declare value to hold header
	//   Note: The smallest unsigned int that will be used to hold user variables is a uint16_t
	//   because uint8_t is interpreted as a char unless otherwise casted, complicating the
	//   process and syntax of storing it properly
	uint16_t uHeader;
	// Read in value for header, checking that it is an appropriate value and fits in 3 bits, before
	//   placing into 256bit packet structure
	while( !bInputOk )
	{
		cout << "Enter a value for the Header (6 or 7): ";
		if( !(cin >> uHeader) )
		{
			// Clear and flush cin upon error reading int
			cin.clear();
			cin.ignore(numeric_limits<streamsize>::max(), '\n');
			cout << "Error: Improper type, please try again" << endl;
		} else if(uHeader != 6 && uHeader != 7)
		{
			// Make sure user enters proper values according to specification
			cout << "Error: Improper value, please try again" << endl;
		} else
		{
			bInputOk = 1;
		}
	}
	uPacket |= uHeader;

	// Reset proper input flag. This will occur after each data field entry by user
	bInputOk = 0;

	// Read in value for tag type, checking that it is an appropriate value and fits in 3 bits, before
	//   placing into packet
	uint16_t uTagType;
	while( !bInputOk )
	{
		cout << "Enter a value for the tag Type (0-7): ";
		if( !(cin >> uTagType) )
		{
			cin.clear();
			cin.ignore(numeric_limits<streamsize>::max(), '\n');
			cout << "Error: Improper type, please try again" << endl;
		} else if(uTagType > 7)
		{
			cout << "Error: Improper value, please try again" << endl;
		} else
		{
			bInputOk = 1;
		}
	}
	uPacket <<= 3;
	uPacket |= uTagType;

	bInputOk = 0;

	// Read in value for Application ID, checking that it is an appropriate value and fits in 3 bits,
	//   before placing into packet
	uint16_t uAppID;
	while( !bInputOk )
	{
		cout << "Enter a value for the Application ID (0-7): ";
		if( !(cin >> uAppID) )
		{
			cin.clear();
			cin.ignore(numeric_limits<streamsize>::max(), '\n');
			cout << "Error: Improper type, please try again" << endl;
		} else if(uAppID > 7)
		{
			cout << "Error: Improper value, please try again" << endl;
		} else
		{
			bInputOk = 1;
		}
	}
	uPacket <<= 3;
	uPacket |= uAppID;

	bInputOk = 0;

	// Read in value for Group ID, checking that it is an appropriate value and fits in 7 bits,
	//   before placing into packet
	uint16_t uGroupID;
	while( !bInputOk )
	{
		cout << "Enter a value for the Group ID (0-127): ";
		if( !(cin >> uGroupID) )
		{
			cin.clear();
			cin.ignore(numeric_limits<streamsize>::max(), '\n');
			cout << "Error: Improper type, please try again" << endl;
		} else if(uGroupID > 127)
		{
			cout << "Error: Improper value, please try again" << endl;
		} else
		{
			bInputOk = 1;
		}
	}
	uPacket <<= 7;
	uPacket |= uGroupID;

	bInputOk = 0;

	// Read in value for Agency ID, checking that it is an appropriate value and fits into 7 bits,
	//   before placing into packet
	uint16_t uAgID;
	while( !bInputOk )
	{
		cout << "Enter a value for the Agency ID (0-127): ";
		if( !(cin >> uAgID) )
		{
			cin.clear();
			cin.ignore(numeric_limits<streamsize>::max(), '\n');
			cout << "Error: Improper type, please try again" << endl;
		} else if(uAgID > 127)
		{
			cout << "Error: Improper value, please try again" << endl;
		} else
		{
			bInputOk = 1;
		}
	}
	uPacket <<= 7;
	uPacket |= uAgID;

	bInputOk = 0;

	// Read in value for Serial Number, checking that it is an appropriate value and fits into 24 bits,
	//   before placing into packet
	uint32_t uSerial;
	while( !bInputOk )
	{
		cout << "Enter a value for the Serial Number (0-16777215): ";
		if( !(cin >> uSerial) )
		{
			cin.clear();
			cin.ignore(numeric_limits<streamsize>::max(), '\n');
			cout << "Error: Improper type, please try again" << endl;
		} else if(uSerial > 16777215)
		{
			cout << "Error: Improper value, please try again" << endl;
		} else
		{
			bInputOk = 1;
		}
	}
	uPacket <<= 24;
	uPacket |= uSerial;

	bInputOk = 0;

	// The border between the Agency Fixed and Programmable Block 1 data fields is left up to each agency,
	//   with the only restricitons being that it must occur at the end of a byte and occur between the
	//   Serial Number and HOV/Feedback Flag data fields
	//
	//   Because of this, the user is prompted for an acceptable bit to set the border at before prompting
	//     for data for each of these fields
	int nDivider;
	while( !bInputOk )
	{
		cout << "The border between the Agency Fixed and Programmable Block 1 sections is determined by each agency and must occur at a byte order (i.e. a multiple of 8)." << endl;
		cout << "This also serves the boundary between the fixed and writeable sections of transponder memory." << endl;
		cout << "Enter the length (in bits) of the fixed section of memory (48-184): ";
		if( !(cin >> nDivider) )
		{
			cin.clear();
			cin.ignore(numeric_limits<streamsize>::max(), '\n');
			cout << "Error: Improper type, please try again" << endl;
		} else if(nDivider < BOUNDARY_MIN || nDivider > BOUNDARY_MAX)
		{
			cout << "Error: Improper value, please try again" << endl;
		} else if( nDivider % BYTE_LEN )
		{
			cout << "Error: Not a byte-aligned (multiple of 8) bit, please try again" << endl;
		} else
		{
			bInputOk = 1;
		}
	}

	bInputOk = 0;

	// Determine the length and maximum value of the Agency Fixed field based on the data field border
	//   that the user specified above
	int nSectionLength = nDivider - KNOWN_FIXED_LENGTH;
	uint256_t uAgFixed;
	uint256_t uMaxVal = 1;
	uMaxVal <<= nSectionLength;
	uMaxVal -= 1;
	// Then, read in an unsigned integer representation of the Agency Fixed section before placing into packet
	while( !bInputOk )
	{
		cout << "Enter an unsigned integer representation of the Agency Fixed block (0-" << uMaxVal << "): ";
		if( !(cin >> uAgFixed) )
		{
			cin.clear();
			cin.ignore(numeric_limits<streamsize>::max(), '\n');
			cout << "Error: Improper type, please try again" << endl;
		} else if(uAgFixed > uMaxVal )
		{
			cout << "Error: Improper value, please try again" << endl;
		} else
		{
			bInputOk = 1;
		}
	}
	uPacket <<= nSectionLength;
	uPacket |= uAgFixed;
	
	bInputOk = 0;

	// Determine the length and maximum value of the Programmable Block 1 field based on the data field
	//   border that the user specified above
	uint256_t uProgData1;
	nSectionLength = 256-64-nDivider;
	uMaxVal = 1;
	uMaxVal <<= nSectionLength;
	uMaxVal -= 1;
	// Then, read in an unsigned integer representation of the Programmable block 1 section before
	//   placing into packet
	while( !bInputOk )
	{
		cout << "Enter an unsigned integer representation of Programmable Block 1 (0-" << uMaxVal << "): ";
		if( !(cin >> uProgData1) )
		{
			cin.clear();
			cin.ignore(numeric_limits<streamsize>::max(), '\n');
			cout << "Error: Improper type, please try again" << endl;
		} else if(uProgData1 > uMaxVal )
		{
			cout << "Error: Improper value, please try again" << endl;
		} else
		{
			bInputOk = 1;
		}
	}
	uPacket <<= nSectionLength;
	uPacket |= uProgData1;

	bInputOk = 0;

	// Read in value for the HOV and Feedback flags, checking that it is an appropriate value and fits
	//   into 8 bits before placing into packet
	uint16_t uHOVFeed;
	while( !bInputOk )
	{
		cout << "Enter a value for the HOV and Feedback Flags (0-255): ";
		if( !(cin >> uHOVFeed) )
		{
			cin.clear();
			cin.ignore(numeric_limits<streamsize>::max(), '\n');
			cout << "Error: Improper type, please try again" << endl;
		} else if(uHOVFeed > 255)
		{
			cout << "Error: Improper value, please try again" << endl;
		} else
		{
			bInputOk = 1;
		}
	}
	uPacket <<= 8;
	uPacket |= uHOVFeed;

	bInputOk = 0;

	// Read in unsigned integer representation of Programmable Block 2, checking that it is an appropriate
	//   value and fits into 40 bits before placing into packet
	uint256_t uProgData2;
	uMaxVal = 1;
	uMaxVal <<= 40;
	uMaxVal -= 1;
	while( !bInputOk )
	{
		cout << "Enter an unsigned integer representation of Programmable Block 2 (0-1099511627775): ";
		if( !(cin >> uProgData2) )
		{
			cin.clear();
			cin.ignore(numeric_limits<streamsize>::max(), '\n');
			cout << "Error: Improper type, please try again" << endl;
		} else if(uProgData2 > uMaxVal )
		{
			cout << "Error: Improper value, please try again" << endl;
		} else
		{
			bInputOk = 1;
		}
	}
	uPacket <<= 40;
	uPacket |= uProgData2;

	// Calculate and append CRC-XMODEM error correction code to end of packet
	uPacket <<= 16;
	uint16_t uCRC = calcPacketCRC(uPacket);
	uPacket |= uCRC;
	
	// Output complete packet in decimal, hex, and binary
	cout << endl << "Packet generation successful" << endl;
	cout << "Generated packet:" << endl;
	cout << "\tDecimal: " << uPacket << endl;
	cout << "\tHex: " << showbase << hex << uPacket << dec << endl;
	// Use bitset to output packet as binary
	//   However, bitset does not have a constructor that can accept boost's uint256_t
	//   So, create bitshift and bitmask uint256_t to create 4 uin64_t values which can be strung together
	//   This will appear as one continuous 256 bit output
	uint64_t uHigh = static_cast<uint64_t>(uPacket >> 192);
	uint64_t uMidHigh = static_cast<uint64_t>(uPacket >> 128 & 0xFFFFFFFFFFFFFFFFu);
	uint64_t uMidLow = static_cast<uint64_t>(uPacket >> 64 & 0xFFFFFFFFFFFFFFFFu);
	uint64_t uLow = static_cast<uint64_t>(uPacket & 0xFFFFFFFFFFFFFFFFu);
	cout << "\tBinary: " << bitset<64>(uHigh) << bitset<64>(uMidHigh) << bitset<64>(uMidLow) << bitset<64>(uLow) << endl << endl;
	return uPacket;
} // end craftPacket()

// Write packet to specified output file
void writePacket(uint256_t uPacket, ofstream* ofPacket)
{
	if( !ofPacket->good() )
	{
		cerr << "Error in output file, quitting..." << endl;
		exit(1);
	}
	
	cout << "Writing packet to file ..." << endl << endl;
	
	// Create a 32-byte array to hold data
	uint8_t auBytes[32] = {0};
	// Initialize a temporary variable to read each byte
	uint8_t uTempByte = 0;
	for(int nByte = 0; nByte < 32; nByte++)
	{
		// For each byte, starting from the LSB, shift and mask packet to store in temporary reader byte
		//   Then, store in array, starting from the back (high index)
		uTempByte = static_cast<uint8_t>(uPacket >> ( BYTE_LEN*nByte) & 0xFFu);
		auBytes[32-1-nByte] = uTempByte;
	}
	char* pcOutByte = (char*)&auBytes[0];
	// Write one byte of binary data at a time
	//   Additionally, write from most significant byte to least significant byte to retain transmission order
	for(int nWriteCount = 0; nWriteCount < 32; nWriteCount++)
	{
		ofPacket->write(pcOutByte, 1);
		pcOutByte++;
	}
	return;
} // end writePacket(uint256_t, ofstream*)

// Check if a given string exists in the command line arguments
bool argExists(vector<string>* vArgs, string sOption)
{
	return ( find(vArgs->begin(), vArgs->end(), sOption) != vArgs->end() );
} // end argExists(vector<string>*, string)

// If a string exists in the command line arguments, get its accompanying (following) value
string getArg(vector<string>* vArgs, string sOption)
{
	
	auto ivIter = find(vArgs->begin(), vArgs->end(), sOption);
	if( ivIter != vArgs->end() && ++ivIter != vArgs->end() )
	{
		string strResult(*ivIter);
		return strResult;
	}
	return "";
} // end getArg(vector<string>*, string)

// Calculates and returns CRC for first 240 bits of a packet
uint16_t calcPacketCRC(uint256_t uPacket)
{
	cout << "Calculating packet error check code..." << endl;
	
	// Create a 32-byte array to hold data
	uint8_t auReflected[32] = {0};
	// Initialize a temporary variable to read each byte
	uint8_t uTempByte = 0;
	for(int nByte = 0; nByte < 32; nByte++)
	{
		// For each byte, starting from the LSB, shift and mask packet to store in temporary reader byte
		//   Then, store in array, starting from the back (high index)
		uTempByte = static_cast<uint8_t>(uPacket >> ( BYTE_LEN*nByte) & 0xFFu);
		auReflected[32-1-nByte] = uTempByte;
	}
	// Call CRC generation library function, using proper initialization values for CRC-XMODEM, and return 
	uint16_t uCRC = CRC::Calculate( auReflected, 30, CRC::CRC_16_XMODEM() );
	return uCRC;	
} // end calcPacketCRC(uint256_t)

// Evaluates a packet's CRC and prompts user to continue or quit if CRC does not match
bool checkPacketCRC(uint256_t uPacket)
{
	// Read CRC from packet
	uint16_t uProvided = static_cast<uint16_t>(uPacket & 0xFFFFu);
	// Calculate expected CRC from first 240 bits of packet
	uint16_t uCRC = calcPacketCRC(uPacket);

	// Output both CRC
	cout << "\tCRC from Packet: " << showbase << hex << uProvided << dec << endl;
	cout << "\tCalculated CRC from Packet Data: " << hex << uCRC << dec << endl;
	if( uCRC == uProvided )
	{
		// Continue as normal if calculated and read CRC match
		cout << "\tValues match, continuing..." << endl << endl;
		return true;
	} else
	{
		// Otherwise, let user know values might be incorrect, but let them choose to continue or not
		string strContinue;
		cout << "Values do not match!" << endl;

		bool bInputOk = 0;
		do
		{
			cout << "Continue displaying packet with bit errors anyways? (y/n): ";
			cin >> strContinue;
			if(strContinue == "y")
			{
				cout << "Continuing anyways..." << endl;
				cout << "*** WARNING - OUTPUT VALUES MAY NOT BE CORRECT! ***" << endl << endl;
				bInputOk = 1;
				return true;
			} else if(strContinue == "n")
			{
				cout << "Exiting gracefully..." << endl;
				bInputOk = 1;
				return false;
			}
		} while( !bInputOk );
	}
	return false;
} // end checkPacketCRC(uint256_t)

/****************************/
/* END FUNCTION DEFINITIONS */
/****************************/


