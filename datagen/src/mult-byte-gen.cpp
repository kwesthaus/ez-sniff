#include <iostream>
#include <fstream>
#include <algorithm>

using namespace std;

int main() {
	// Prompt user for input file name, then open it, and quit if error
	cout << "Enter input packet file name: ";
	string strPacketFileName;
	cin >> strPacketFileName;
	ifstream fIn(strPacketFileName);
	if(fIn.fail()) {
		cout << "Input file opening error, quitting" << endl;
		return 1;
	}
	// Prompt user for output file name, then open it with binary and
	//   append flags, and quit if error
	cout << "Enter modulated output file name: ";
	string strModFileName;
	cin >> strModFileName;
	ofstream fOut(strModFileName, ios::binary|ios::out);
	if(fOut.fail()) {
		cout << "Output file opening error, quitting" << endl;
		return 2;
	}
	// Interrogation pulse is 20 microseconds long
	uint8_t aPulse[20] = {};
	// Set amplitude multiplier for this 20 microseconds to 1/on
	fill_n(aPulse, 20, (unsigned)1);
	// Gap between interrogation pulses is 7 milliseconds (7000 microseconds) long
	// Note: When at least one element of an array is declared during initialization,
	//   the rest of the array is automatically filled with 0 values
	// The amplitude multiplier for this time period is 0/off
	uint8_t aGap[7000] = {0};
	// Write 100 repetitions of the spaced interrogation pulses
	for(int nCount = 0; nCount < 100; nCount++) {
		fOut.write((char*)aPulse, 20);
		fOut.write((char*)aGap, 7000);
	}
	// Now, move on to actual packet data
	// Output one more trigger, then a gap for the xponder response,
	//   then the desired packet, then a long gap again
	uint8_t aPacket[512];
	fOut.write((char*)aPulse, 20);
	fOut.write((char*)aGap, 552);
	fIn.read((char*)aPacket, 512);
	fOut.write((char*)aPacket, 512);
	fOut.write((char*)aGap, 7000);
	// Close files safely
	fOut.close();
	fIn.close();
}
