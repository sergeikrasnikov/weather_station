/* 
  - WHAT IT DOES
   Arduino sends sensors' data by nRF25L01 Radio to
   another part with nRF24L01 radio 

  - SEE the comments after "//" on each line below
  - CONNECTIONS:
   - nRF24L01 Radio Module: See http://arduino-info.wikispaces.com/Nrf24L01-2.4GHz-HowTo
   1 - GND
   2 - VCC 3.3V !!! NOT 5V
   3 - CE to Arduino pin 7
   4 - CSN to Arduino pin 8
   5 - SCK to Arduino pin 13
   6 - MOSI to Arduino pin 11
   7 - MISO to Arduino pin 12
   8 - UNUSED
  - V2.12 02/08/2016
   - Uses the RF24 Library by TMRH20 and Maniacbug: https://github.com/TMRh20/RF24 (Download ZIP)
    */

/*-----( Import needed libraries )-----*/
#include <SPI.h>   // Comes with Arduino IDE
#include "RF24.h"  // Download and Install (See above)
#include "printf.h" // Needed for "printDetails" Takes up some memory
#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/wdt.h>
#include "nRF24L01.h"
/*-----( Declare Constants and Pin Numbers )-----*/
#define  CE_PIN  7   // The pins to be used for CE and SN
#define  CSN_PIN 8

#define BATTERY_SENSE_PIN A0  // select the input pin for the battery sense point


#include <Wire.h>
#include <SPI.h>
#include "Adafruit_Sensor.h"
#include "Adafruit_BMP280.h"
#include "Sodaq_SHT2x.h"
#include "MAX44009.h"

// Sleep declarations
typedef enum { wdt_16ms = 0, wdt_32ms, wdt_64ms, wdt_128ms, wdt_250ms, wdt_500ms, wdt_1s, wdt_2s, wdt_4s, wdt_8s } wdt_prescalar_e;

void setup_watchdog(uint8_t prescalar);
void do_sleep(void);

const short sleep_cycles_per_transmission = 4;
volatile short sleep_cycles_remaining = sleep_cycles_per_transmission;


MAX44009 light;


Adafruit_BMP280 bme; // I2C

/*-----( Declare objects )-----*/
/* Hardware configuration: Set up nRF24L01 radio on SPI bus plus (usually) pins 7 & 8 (Can be changed) */
RF24 radio(CE_PIN, CSN_PIN);

/*-----( Declare Variables )-----*/
byte addresses[][6] = {"1Node", "2Node"}; // These will be the names of the "Pipes"

unsigned long timeNow;  // Used to grab the current time, calculate delays
unsigned long started_waiting_at;
boolean timeout;       // Timeout? True or False


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

  analogReference(INTERNAL); // use the 1.1 V internal reference
  
  Serial.begin(115200);

      light.begin();
      delay(500);

      if (!bme.begin()) {  
        Serial.println(F("Could not find a valid BMP280 sensor, check wiring!"));
        while (1);
      }
  
  // NOTE: The "F" in the print statements means "unchangable data; save in Flash Memory to conserve SRAM"
  Serial.println(F("Send weather station data by nRF24L01 radio to another Arduino"));
  printf_begin(); // Needed for "printDetails" Takes up some memory

  setup_watchdog(wdt_8s);
  
  radio.begin();          // Initialize the nRF24L01 Radio
  radio.setChannel(108);  // Above most WiFi frequencies
  radio.setDataRate(RF24_250KBPS); // Fast enough.. Better range

  // Set the Power Amplifier Level low to prevent power supply related issues since this is a
  // getting_started sketch, and the likelihood of close proximity of the devices. RF24_PA_MAX is default.
  // PALevelcan be one of four levels: RF24_PA_MIN, RF24_PA_LOW, RF24_PA_HIGH and RF24_PA_MAX
  radio.setPALevel(RF24_PA_LOW);
  //  radio.setPALevel(RF24_PA_MAX);

  // Open a writing and reading pipe on each radio, with opposite addresses
  radio.openWritingPipe(addresses[0]);
  radio.openReadingPipe(1, addresses[1]);

  // Start the radio listening for data
  radio.startListening();

//  radio.printDetails(); //Uncomment to show LOTS of debugging information
}//--(end setup )---


void loop()   /****** LOOP: RUNS CONSTANTLY ******/
{
  radio.powerUp();                                // Power up the radio after sleeping
  radio.stopListening();                                    // First, stop listening so we can talk.


    int sensorValue = analogRead(BATTERY_SENSE_PIN);
    // 1M, 300K divider across battery and using internal ADC ref of 1.1V
    // Sense point is bypassed with 0.1 uF cap to reduce noise at that point
    //(1M + 300K)/300K *1.1 = Vmax = 4.7666666 V
    //4.766 /1023 = Volts per bit =0.00465949;

    //(1M + 333K)/333K *1.1 = Vmax = 4.4033033 V
    //4.4033033 /1023 = Volts per bit =0.00430430 ;
    myData.batteryV = sensorValue * 0.0043043 ;
    
    myData.bmp_temperature = bme.readTemperature();
    myData.bmp_pressure = bme.readPressure();
    myData.sht_humidity = SHT2x.GetHumidity();
    myData.sht_temperature = SHT2x.GetTemperature();
    myData.lux = light.get_lux();
    

  myData._micros = micros();  // Send back for timing


  Serial.print(F("Now sending  -  "));

  if (!radio.write( &myData, sizeof(myData) )) {            // Send data, checking for error ("!" means NOT)
    Serial.println(F("Transmit failed "));
  }

  radio.startListening();                                    // Now, continue listening

  started_waiting_at = micros();               // timeout period, get the current microseconds
  timeout = false;                            //  variable to indicate if a response was received or not

  while ( ! radio.available() ) {                            // While nothing is received
    if (micros() - started_waiting_at > 200000 ) {           // If waited longer than 200ms, indicate timeout and exit while loop
      timeout = true;
      break;
    }
  }

  if ( timeout )
  { // Describe the results
    Serial.println(F("Response timed out -  no Acknowledge."));
  }
  else
  {
    // Grab the response, compare, and send to Serial Monitor
    radio.read( &myData, sizeof(myData) );
    timeNow = micros();

    // Show it
    Serial.print(F("Sent "));
    Serial.print(timeNow);
    Serial.print(F(", Got response "));
    Serial.print(myData._micros);
    Serial.print(F(", Round-trip delay "));
    Serial.print(timeNow - myData._micros);
    Serial.println(F(" microseconds "));

  }

  // Send again after delay. When working OK, change to something like 100
  //delay(100);

      // Shut down the system
    delay(100);                     // Experiment with some delay here to see if it has an effect
                                    // Power down the radio.  
    radio.powerDown();              // NOTE: The radio MUST be powered back up again manually

    //wdt_reset();
    //myWatchdogEnable();
                                    // Sleep the MCU.
    do_sleep();


}//--(end main loop )---

/*-----( Declare User-written Functions )-----*/

void wakeUp(){
  sleep_disable();
}

// Sleep helpers

//Prescaler values
// 0=16ms, 1=32ms,2=64ms,3=125ms,4=250ms,5=500ms
// 6=1 sec,7=2 sec, 8=4 sec, 9= 8sec

void setup_watchdog(uint8_t prescalar){

  uint8_t wdtcsr = prescalar & 7;
  if ( prescalar & 8 )
    wdtcsr |= _BV(WDP3);
  MCUSR &= ~_BV(WDRF);                      // Clear the WD System Reset Flag
  WDTCSR = _BV(WDCE) | _BV(WDE);            // Write the WD Change enable bit to enable changing the prescaler and enable system reset
  WDTCSR = _BV(WDCE) | wdtcsr | _BV(WDIE);  // Write the prescalar bits (how long to sleep, enable the interrupt to wake the MCU
}

void myWatchdogEnable() {  // turn on watchdog timer; interrupt mode every 2.0s
 cli();
 MCUSR = 0;
 WDTCSR |= B00011000;
 WDTCSR = B01000111;
 sei();
}

void do_sleep(void)
{
  sleep_enable();
  set_sleep_mode(SLEEP_MODE_PWR_DOWN); // sleep mode is set here
  //attachInterrupt(0,wakeUp,LOW);
  WDTCSR |= _BV(WDIE);
  sleep_mode();                        // System sleeps here
                                       // The WDT_vect interrupt wakes the MCU from here
  sleep_disable();                     // System continues execution here when watchdog timed out  
 // detachInterrupt(0);  
  WDTCSR &= ~_BV(WDIE);  
}
