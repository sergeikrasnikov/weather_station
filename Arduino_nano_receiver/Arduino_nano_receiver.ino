/* 
  - CONNECTIONS:
   - nRF24L01 Radio Module
   1 - GND
   2 - VCC 3.3V !!! NOT 5V
   3 - CE to Arduino pin 7
   4 - CSN to Arduino pin 8
   5 - SCK to Arduino pin 13
   6 - MOSI to Arduino pin 11
   7 - MISO to Arduino pin 12
   8 - UNUSED

   - Uses the RF24 Library by TMRH20 and Maniacbug: https://github.com/TMRh20/RF24 (Download ZIP)
*/

/*-----( Import needed libraries )-----*/
#include <SPI.h>   // Comes with Arduino IDE
#include "RF24.h"  // Download and Install (See above)
//#include "printf.h" // Needed for "printDetails" Takes up some memory
/*-----( Declare Constants and Pin Numbers )-----*/
//arduino nano
#define  CE_PIN  7   // The pins to be used for CE and SN
#define  CSN_PIN 8

/* Hardware configuration: Set up nRF24L01 radio on SPI bus plus (usually) pins 7 & 8 (Can be changed) */
RF24 radio(CE_PIN, CSN_PIN);


/*-----( Declare Variables )-----*/
byte addresses[][6] = {"1Node", "2Node"}; // These will be the names of the "Pipes"


/**
  Create a data structure for transmitting and receiving data
  This allows many variables to be easily sent and received in a single transmission
  See http://www.cplusplus.com/doc/tutorial/structures/
*/

struct dataStruct {
  unsigned long _micros;  // to save response times

  float bmp_temperature;
  float bmp_pressure; 

  float sht_humidity;
  float sht_temperature;

  float lux;
  float batteryV;
  
} myData;                 // This can be accessed in the form:  myData.Xposition  etc.


void setup()   /****** SETUP: RUNS ONCE ******/
{
  Serial.begin(115200);   // MUST reset the Serial Monitor to 115200 (lower right of window )
  // NOTE: The "F" in the print statements means "unchangable data; save in Flash Memory to conserve SRAM"
  Serial.println(F("Receive weather station data by nRF24L01 radio from another Arduino"));
 // printf_begin(); // Needed for "printDetails" Takes up some memory
  /*-----( Set up servos )-----*/

  radio.begin();          // Initialize the nRF24L01 Radio
  radio.setChannel(108);  // 2.508 Ghz - Above most Wifi Channels
  radio.setDataRate(RF24_250KBPS); // Fast enough.. Better range

  // Set the Power Amplifier Level low to prevent power supply related issues since this is a
  // getting_started sketch, and the likelihood of close proximity of the devices. RF24_PA_MAX is default.
  // PALevelcan be one of four levels: RF24_PA_MIN, RF24_PA_LOW, RF24_PA_HIGH and RF24_PA_MAX
  radio.setPALevel(RF24_PA_LOW);
  //   radio.setPALevel(RF24_PA_MAX);

  // Open a writing and reading pipe on each radio, with opposite addresses
  radio.openWritingPipe(addresses[1]);
  radio.openReadingPipe(1, addresses[0]);

  // Start the radio listening for data
  radio.startListening();
//  radio.printDetails(); //Uncomment to show LOTS of debugging information
}//--(end setup )---


void loop()   /****** LOOP: RUNS CONSTANTLY ******/
{

  if ( radio.available())
  {

    while (radio.available())   // While there is data ready to be retrieved from the receive pipe
    {
      radio.read( &myData, sizeof(myData) );             // Get the data
    }

    radio.stopListening();                               // First, stop listening so we can transmit
    radio.write( &myData, sizeof(myData) );              // Send the received data back.
    radio.startListening();                              // Now, resume listening so we catch the next packets.

    Serial.print(F("Packet Received - Sent response "));  // Print the received packet data
    Serial.print(myData._micros);
    Serial.print(F("uS bmp_temp= "));
    Serial.print(myData.bmp_temperature);
    Serial.print(F(" bmp_press= "));
    Serial.print(myData.bmp_pressure);
    Serial.print(F(" sht_humid= "));
    Serial.print(myData.sht_humidity);
    Serial.print(F(" sht_temp= "));
    Serial.print(myData.sht_temperature);
    Serial.print(F(" lux= "));
    Serial.print(myData.lux);
    Serial.print(F(" battery= "));
    Serial.println(myData.batteryV);


  } // END radio available

}//--(end main loop )---

/*-----( Declare User-written Functions )-----*/

// NONE YET
//*********( THE END )***********
