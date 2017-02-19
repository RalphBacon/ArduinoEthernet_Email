/* Demo sketch for a simple web page that can monitor (in this case) water
 * temperature and water levels but could be used for any monitoring purpose.
 *
 * This version updated to read data from an SD card. 
 *
 * Space is at a premium on an Arduino UNO or Nano so we've stored the actual
 * web page HTML in program memory - otherwise the run-time SRAM would be
 * immediately filled with the large strings of HTML data.
 *
 * This is using the UIPEthernet library which is drop-in replacement for
 * the standard Ethernet library (which comes with the Arduino IDE).
 *
 * This means that not only can you use the W5100 (which the original Ethernet
 * library supports) you can also use the cheap ENC29J60 module as a retrofit.
 * Just be aware that the UIPEthenet library is quite large so space may be tight.
 * If all else fails use an Arduino MEGA!
 *
 * Attribution:
 * This demo sketch is an adapted version of the example sketch in the library.
 * Plus some stuff from the Internet I should think. Plus my own work.
 */
#include "Arduino.h"
#include <Ethernet.h>
#include <SD.h>

// As our HTML is now on the SD card we no longer need to include that additional
// sketch file where our HTML was in PROGMEM saving space for more code
//#include "HTML.h"

// Some global variables to capture the highest and lowest temperature recorded
int minTemp = 1000;
int maxTemp = 0;

// Let's create our Web Server on port 1000 (use port 80 for standard browsing)
// This means your browser request MUST append :1000 the URL. Use port 80 if you
// don't want to do this.
EthernetServer server = EthernetServer(1000);

// ------------------------------------------------------------------------------
// SETUP     SETUP     SETUP     SETUP     SETUP     SETUP     SETUP     SETUP
// ------------------------------------------------------------------------------
void setup() {
	// Serial Monitor aka Debugging Window
	Serial.begin(115200);

	// Uses SPI on pin 4. Disable W5100 whilst we configure the SD card
	digitalWrite(10, HIGH);

	// Now begin it on the fixed GPIO pin 4 (only for the shield)
	if (!SD.begin(4)) {
		Serial.println("SD Card not initialised.");
	}
	else {
		Serial.print("SD Card initialised.");
	}

	// You must assign a UNIQUE MAC address to the Ethernet module. Well, unique
	// in your home or work setup anyway. Set it and don't keep changing is as
	// your router is tracking it. Write it on the module!
	uint8_t mac[6] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

	// Give your web server a STATIC address here so the browser can find it.
	// From a Windows command prompt, try and "ping" it to see whether you can
	// see it on the network. Like this:
	/*
	 Microsoft Windows [Version 10.0.14393]
	 (c) 2016 Microsoft Corporation. All rights reserved.

	 C:\Users\Ralph>ping 192.168.1.123   					<--- you type this in!

	 Pinging 192.168.1.123 with 32 bytes of data:
	 Reply from 192.168.1.123: bytes=32 time=1ms TTL=64
	 Reply from 192.168.1.123: bytes=32 time=1ms TTL=64
	 Reply from 192.168.1.123: bytes=32 time=1ms TTL=64
	 Reply from 192.168.1.123: bytes=32 time=1ms TTL=64

	 Ping statistics for 192.168.1.123:
	 Packets: Sent = 4, Received = 4, Lost = 0 (0% loss),
	 Approximate round trip times in milli-seconds:
	 Minimum = 1ms, Maximum = 1ms, Average = 1ms
	 */
	IPAddress myIP(192, 168, 1, 123);						// <--- change to suit

	// All information set up, we can begin
	Ethernet.begin(mac, myIP);
	server.begin();

	// Just to prove that your server is running on your specified IP address
	Serial.print("IP Address: ");
	Serial.println(Ethernet.localIP());

	// Let's see how much free ram we have left before the loop starts
	Serial.print(F("Free SRAM: "));
	Serial.println(freeRam());
}

// ------------------------------------------------------------------------------
// LOOP     LOOP     LOOP     LOOP     LOOP     LOOP     LOOP     LOOP     LOOP
// ------------------------------------------------------------------------------
void loop() {

	// Listen for incoming clients on above port
	if (EthernetClient client = server.available()) {
		Serial.println(F("Client connected."));

		// Let's see how much free ram we have left. This should not grow and grow!
		Serial.print(F("Free SRAM: "));
		Serial.println(freeRam());

		// Display all the info sent back to the Arduino from your browser
		String readData = "";

		// While there is data to read
		while (client.available()) {

			// Read it character by character
			char dataChar = client.read();

			// And concatenate into the string
			readData += dataChar;

			// IF we hit a New Line character we'll print and inspect that line
			if (dataChar == '\n') {

				// Debugging/monitoring - slows down the sending back of the page quite a bit
				Serial.print(readData);

				// We're looking for the GET request and any parameters
				if (readData.startsWith("GET")) {
					if (readData.indexOf("clear") > -1) {

						// Reset the min/max
						minTemp = 1000;
						maxTemp = 0;
					}
				}

				// Clear the concatenated string ready for next line (if any)
				readData = "";
			}
		}

		// Now send back the web page to your browser (desktop, phone etc)
		sendStaticPage(client);

		// Give it a chance to finish
		delay(50);

		// Terminate the client link, ready for another
		client.stop();
		Serial.println(F("Client disconnected."));
	}
}

// ------------------------------------------------------------------------------
// Send back the HTML page with correct headers
// ------------------------------------------------------------------------------
void sendStaticPage(EthernetClient client) {
	// Return the response
	//char buffer[100];

	// Here is where you might call functions to retrieve measurement data such
	// as temperature, flow, presence (or absence), switch status etc
	// I'm just emulating that here by reading random analog values
	int airTemp = analogRead(A0);
	int waterTemp = analogRead(A1);

	// Update the min/max
	minTemp = min(minTemp, waterTemp);
	maxTemp = max(maxTemp, waterTemp);

	// Output the header here manually (it needs "print" not "write".
	client.println(F("HTTP/1.1 200 OK")); //send new page
	client.println(F("Content-Type: text/html"));
	client.println("");

	// Initialise a string (byte array) to hold the entire line of HTML
	String temp = "";

	// Ensure your file on the SD card ends with ".htm" NOT ".html" as we are
	// restricted to the old 8.3 file name format
	File testFile = SD.open("test.htm");

	// Could we open that file?
	if (testFile) {
		Serial.println(F("SD Card file contents:"));

		// While there is still data to read in the file (it treats it as
		// one huge long line of characters regardless of whether you have
		// formatted it with new lines etc
		while (testFile.available()) {

			// Read as many characters until you hit a new line character
			temp = testFile.readStringUntil('\n');

			// Try and substitute those special placeholder variables with actual values
			temp.replace(F("!AirTemp"), String(airTemp, DEC));
			temp.replace(F("!WaterTemp"), String(waterTemp, DEC));
			temp.replace(F("!MinTemp"), String(minTemp, DEC));
			temp.replace(F("!MaxTemp"), String(maxTemp, DEC));

			// Uncomment this line to see what you are sending as an HTML line but
			// be prepared for a slow down in your web page.
			//Serial.println(temp);

			//And send the HTML line back to the client (browser)
			client.println(temp);

			// Clear the HTML string ready for the next line
			temp = "";
		}
		Serial.println(F("End of SD Card contents."));
		testFile.close();
	}
	else {
		Serial.println(F("Cannot open html output file"));
		client.println(F("An error has occurred. Your call is important to us. Really."));
	}
}

// ------------------------------------------------------------------
// This is the routine that will display how much free SRAM you have.
// Ideally place in SETUP before the loop, but no hard rules.
//
// A "size_t" type can store the maximum size of any type. Commonly used for
// array indexing and loop counting. We could use an unsigned int here too.
// ------------------------------------------------------------------
size_t freeRam() {
	return RAMEND - size_t(__malloc_heap_start);
}
