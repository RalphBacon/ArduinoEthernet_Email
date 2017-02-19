/* A simple UDP example which connects to an NTP (Network Time Protocol) ideally in a pool (so we get
 * a random NTP server given to us from that pool).
 *
 * The reply (seconds since midnight 1970) is then converted into the date and time for use in emails.
 *
 */
#include <arduino.h>
#include <SPI.h>

// We're using a drop in replacement for the standard "Ethernet.h" library
// because we want to use an ENC28J60 Ethernet module (instead of a W5100).
// It also works without change on a WizNet W5100 Ethernet Controller.
// You can use the original Ethernet.h library (if you have a W5100) if you prefer.
#include <UIPEthernet.h>
#include <UIPUdp.h>

// See Arduino Playground for details of this useful time synchronisation library
#include <TimeLib.h>

// Enter a MAC address for your controller below.
// Newer Ethernet shields have a MAC address printed on a sticker on the shield
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

// Just an open port we can use for the UDP packets coming back in
unsigned int localPort = 8888;

// this is the "pool" name for any number of NTP servers in the pool.
// If you're not in the UK, use "time.nist.gov"
// Elsewhere: USA us.ppol.ntp.org
// Read more here: http://www.pool.ntp.org/en/use.html
char timeServer[] = "uk.pool.ntp.org";

// NTP time stamp is in the first 48 bytes of the message
const int NTP_PACKET_SIZE = 48;

//buffer to hold incoming and outgoing packets
byte packetBuffer[NTP_PACKET_SIZE];

// A UDP instance to let us send and receive packets over UDP
EthernetUDP Udp;

// Your time zone relative to GMT / UTC
const int timeZone = 1;

// Days of week. Day 1 = Sunday
String DoW[] = {"Sun", "Mon","Tue","Wed","Thur","Fri","Sat"};

//-----------------------------------------------------------------------------
// SETUP     SETUP     SETUP     SETUP     SETUP     SETUP     SETUP     SETUP
//-----------------------------------------------------------------------------
void setup() {
	// Open serial communications and wait for port to open:
	Serial.begin(9600);

	Serial.println("UIP Ethernet Test for NTP");
	Serial.flush();

	// start Ethernet and UDP with YOUR IP address given to the Ethernet card
	IPAddress myIP(192, 168, 1, 123);

	// This is YOUR DNS server (use ipconfig /all from a Windows CMD prompt to find it)
	IPAddress gWayDNS(192, 168, 1, 254);

	// And off we go
	Ethernet.begin(mac, myIP, gWayDNS, gWayDNS);

	// What port will the UDP/NTP packet respond on?
	Udp.begin(localPort);

	// What is the function that gets the time (in ms since 01/01/1900)?
	setSyncProvider(getNTPTime);

	// How often should we synchronise the time on this machine (in seconds)?
	// Use 300 for 5 minutes but once an hour (3600) is more than enough usually
	setSyncInterval(12); // just for demo purposes!
}

//-----------------------------------------------------------------------------
// LOOP     LOOP     LOOP     LOOP     LOOP     LOOP     LOOP     LOOP     LOOP
//-----------------------------------------------------------------------------
void loop() {

	// This just prints the "system time"
	digitalClockDisplay();

	// Do other stuff etc, delay here just emulates that
	delay(3000);
}

//-----------------------------------------------------------------------------
// Prints a nice time display
//-----------------------------------------------------------------------------
void digitalClockDisplay() {
	// We'll grab the time so it doesn't change whilst we're printing it
	time_t t=now();

	//Now print all the elements of the time secure that it won't change under our feet
	printDigits(hour(t));
	Serial.print(":");
	printDigits(minute(t));
	Serial.print(":");
	printDigits(second(t));
	Serial.print("    ");
	Serial.print(DoW[weekday(t)-1]);
	Serial.print(", ");
	printDigits(day(t));
	Serial.print("/");
	printDigits(month(t));
	Serial.print("/");
	printDigits(year(t));
	Serial.println();
}

void printDigits(int digits) {
	// utility for digital clock display: prints leading 0
	if (digits < 10) Serial.print('0');
	Serial.print(digits);
}

//-----------------------------------------------------------------------------
// This is the function to contact the NTP pool and retrieve the time
//-----------------------------------------------------------------------------
time_t getNTPTime() {

	// Send a UDP packet to the NTP pool address
	Serial.print("\nSending NTP packet to ");
	Serial.println(timeServer);
	sendNTPpacket(timeServer);

	// Wait to see if a reply is available
	delay(2000);
	if (Udp.parsePacket()) {
		// We've received a packet, read the data from it
		Udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer

		// The time-stamp starts at byte 40 of the received packet and is four bytes,
		// or two words, long. First, extract the two words:
		unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
		unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);

		// combine the four bytes (two words) into a long integer
		// this is NTP time (seconds since Jan 1 1900)
		unsigned long secsSince1900 = highWord << 16 | lowWord;
		Serial.print("Seconds since Jan 1 1900 = ");
		Serial.println(secsSince1900);

		// now convert NTP time into everyday time:
		//Serial.print("Unix time = ");

		// Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
		const unsigned long seventyYears = 2208988800UL;

		// subtract seventy years:
		unsigned long epoch = secsSince1900 - seventyYears;

		return epoch;
	}

	// Failed to get an NTP/UDP response
	Serial.println("No response");
	return 0;
}

//-----------------------------------------------------------------------------
// send an NTP request to the time server at the given address
//-----------------------------------------------------------------------------
void sendNTPpacket(char* address) {
	// set all bytes in the buffer to 0
	memset(packetBuffer, 0, NTP_PACKET_SIZE);
	// Initialize values needed to form NTP request
	// (see URL above for details on the packets)
	packetBuffer[0] = 0b11100011;   // LI, Version, Mode
	packetBuffer[1] = 0;     // Stratum, or type of clock
	packetBuffer[2] = 6;     // Polling Interval
	packetBuffer[3] = 0xEC;  // Peer Clock Precision
	// 8 bytes of zero for Root Delay & Root Dispersion
	packetBuffer[12] = 49;
	packetBuffer[13] = 0x4E;
	packetBuffer[14] = 49;
	packetBuffer[15] = 52;

	// all NTP fields have been given values, now you can send a packet requesting a timestamp:
	// Note that Udp.begin will request automatic translation (via a DNS server) from a
	// name (eg pool.ntp.org) to an IP address. Never use a specific IP address yourself,
	// let the DNS give back a random server IP address
	Udp.beginPacket(address, 123); //NTP requests are to port 123

	// Get the data back
	Udp.write(packetBuffer, NTP_PACKET_SIZE);

	// All done, the underlying buffer is now updated
	Udp.endPacket();
}

