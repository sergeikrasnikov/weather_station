/*
 * 
 *   - CONNECTIONS:
   - nRF24L01 Radio Module
  
1 - GND
2 - VCC 3.3V !!! NOT 5V


ESP8266 pinout   
Pin Name GPIO #  HSPI Function
MTDI  GPIO12  MISO (DIN)
MTCK  GPIO13  MOSI (DOUT)
MTMS  GPIO14  CLOCK
MTDO  GPIO15  CS / SS
so 
CSN_PIN GPIO15
and 
CE_PIN  GPIO2

*/

/*-----( Import needed libraries )-----*/
#include <ESP8266WiFi.h>
#include <SPI.h>   // Comes with Arduino IDE
#include "RF24.h"  // Download and Install 
/*-----( Declare Constants and Pin Numbers )-----*/


// replace with your channel’s thingspeak API key,
String apiKey = "48GHET202Y50NZJK";
// replace with your WIFI SSID & PASSWORD
const char* ssid = "WIFI_SSID";
const char* password = "PASSWORD";
const char* server = "api.thingspeak.com";

//esp8266
#define  CE_PIN  2   // The pins to be used for CE and SN
#define  CSN_PIN 15


/*-----( Declare objects )-----*/
/* Hardware configuration: Set up nRF24L01 radio on SPI bus plus (usually)  */
RF24 radio(CE_PIN, CSN_PIN);

WiFiClient client;


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
  /*-----( Set up wi-fi connection )-----*/
  delay(10);
  
  WiFi.begin(ssid, password);
  
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
  delay(500);
  Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
 
  
  
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

    Serial.println("% send to Thingspeak");


    if (client.connect(server,80)) { // "184.106.153.149" or api.thingspeak.com

    String postStr = apiKey;
    postStr +="&field1=";
    postStr += String(myData.bmp_temperature);
    postStr +="&field2=";
    postStr += String(myData.bmp_pressure);
    postStr +="&field3=";
    postStr += String(myData.sht_humidity);
    postStr +="&field4=";
    postStr += String(myData.sht_temperature);
    postStr +="&field5=";
    postStr += String(myData.lux);
    postStr +="&field6=";
    postStr += String(myData.batteryV);
    postStr += "\r\n\r\n";
    
    client.print("POST /update HTTP/1.1\n");
    client.print("Host: api.thingspeak.com\n");
    client.print("Connection: close\n");
    client.print("X-THINGSPEAKAPIKEY: "+apiKey+"\n");
    client.print("Content-Type: application/x-www-form-urlencoded\n");
    client.print("Content-Length: ");
    client.print(postStr.length());
    client.print("\n\n");
    client.print(postStr);
    
    }
    client.stop();
    
    Serial.println("Waiting…");
    // thingspeak needs minimum 15 sec delay between updates
    delay(20000);


  } // END radio available

}//--(end main loop )---

/*-----( Declare User-written Functions )-----*/

// NONE YET
//*********( THE END )***********



