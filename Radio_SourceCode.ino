/////////////////////////////////////////////////////////////////////////////////
//// RECEIVER CODE ///////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////


/*

  receiver code for an RC sail boat that has rudder and main and jib sheet control

  Revisions
  rev     date        author      description
  1       06-17-2011  kasprzak    initial creation

  Pin connections
  Arduino   device
  Ground   ground
  A0
  A1
  A2
  A3
  A4
  A5
  1
  2      IRQ (not used) brown
  3
  4     Rudder servo control line
  5
  6     Sail servo control line
  7     
  8
  9     CE (UNO only) 40 mega
  10    CSN (UNO only) 53 mega
  11    MOSI (UNO only) 51 mega
  12    MISO (UNO only) 50 mega
  13    SCK (UNO only)  52 mega
  SDA
  SLC

  Top view (pins on bottom)
  IRQ     MISO
  MOSI    SCK
  CSN     CE
  VCC     GND

  links to library
  https://github.com/maniacbug/RF24

  link to Transceiver
  http://www.amazon.com/gp/product/B00E594ZX0?psc=1&redirect=true&ref_=oh_aui_detailpage_o03_s00

  link to rf24 help page
  http://starter-kit.nettigo.eu/2014/connecting-and-programming-nrf24l01-with-arduino-and-other-boards/

*/

#define RUD_PIN 4
#define SAI_PIN 6

#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <Wire.h>
#include <Servo.h>

// a common data structure used to pass data to the reciever
// commented below
#include <Structure.h>


// uno
RF24 radio(9, 10);

//mega
// RF24 radio(40,53);

// address for both units
const byte rxAddr[6] = "00001";

int Rudd;
byte Sail;

// initialize the data structure
RCPacket data;

// initialize the servos
Servo RuddSVO;
Servo SailSVO;

void setup()
{
  
  RuddSVO.attach(RUD_PIN);
  SailSVO.attach(SAI_PIN);

  radio.begin();

  /*
    rf24 power constants
      RF24_PA_MIN
      RF24_PA_LOW
      RF24_PA_HIGH
      RF24_PA_MAX

  */

  radio.setPALevel(RF24_PA_HIGH);
  radio.setRetries(15, 15);
  radio.openReadingPipe(0, rxAddr);
  radio.startListening();

}

void loop()
{

  if (radio.available())  {

    radio.read(&data, sizeof(data));

    // the input pot has already been converted to the rudder sweep angle
    // so just write it to the servo


    RuddSVO.write(data.Rudd);

    // the directon and speed has already been converted for the sheet in / out
    // so just write it to the servo
    SailSVO.write(data.Sail);
    
  }
  

  // future
  // 1. write a call back to send back what was recieved for transmitter to verify
  // 2. adjust power lower if reciever can find the radio, else adjust the power higher
  // 3. come home function with GPS so if radio is lost boat comes home

}



/////////////////////////////////////////////////////////////////////////////////
///// DATA STRUCTURE ////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////

/*


  This structure is in a .h file called structure.h and stored in the Arduino library folder



  struct RCPacket {

  unsigned long curtime;
  byte Rudd;
  byte Sail;



  };

*/



/////////////////////////////////////////////////////////////////////////////////
//// TRANSMITTER CODE ///////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////

/*


  transmitter code for an RC sail boat that has rudder and main and jib sheet control

  Revisions
  rev     date        author      description
  1       06-17-2011  kasprzak    initial creation

  Pin connections
  Arduino   device
  Ground   ground
  A0
  A1
  A2
  A3
  A4
  A5
  1
  2      IRQ (not used) brown
  3
  4
  5
  6     Sail servo control line
  7     Rudder servo control line
  8
  9     CE (UNO only) 40 mega
  10    CSN (UNO only) 53 mega
  11    MOSI (UNO only) 51 mega
  12    MISO (UNO only) 50 mega
  13    SCK (UNO only)  52 mega
  SDA
  SLC

  Top view (pins on bottom)
  IRQ     MISO
  MOSI    SCK
  CSN     CE
  VCC     GND

  links to library
  https://github.com/maniacbug/RF24

  link to Transceiver
  http://www.amazon.com/gp/product/B00E594ZX0?psc=1&redirect=true&ref_=oh_aui_detailpage_o03_s00

  link to rf24 help page
  http://starter-kit.nettigo.eu/2014/connecting-and-programming-nrf24l01-with-arduino-and-other-boards/



#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <Structure.h>

#define ADJ_PIN A0
#define SIN_PIN 2
#define SOU_PIN 4

int  Rudd;
bool SailIn;
bool SailOu;

// mega
// RF24 radio(40, 53);

//uno
RF24 radio(9, 10);

// initialize the data structure
RCPacket data;

// address for both units
const byte rxAddr[6] = "00001";

void setup() {
  Serial.begin(9600);
  radio.begin();


 //     rf24 power constants
  //      RF24_PA_MIN
   //     RF24_PA_LOW
   //     RF24_PA_HIGH
    //    RF24_PA_MAX



  radio.setPALevel(RF24_PA_HIGH);
  radio.setRetries(15, 15);
  radio.openWritingPipe(rxAddr);
  radio.stopListening();

  // 100k pot for the rudder input
  pinMode(ADJ_PIN, INPUT);

  // use pullup to use Arduino internal 20K pullups
  // saves me having to solder in resistors
  pinMode(SIN_PIN, INPUT_PULLUP);
  pinMode(SOU_PIN, INPUT_PULLUP);

}

void loop() {

  //  may some day use this for a call back comparison to ensure signal was recieved
  data.curtime = millis() / 1000;

  // read the pot value and the digital pins for the rudder adjustment
  Rudd = analogRead(ADJ_PIN) ;


  // store the pot value in the data structure
  // map the 0 to 3.3 volt signal to a 110 degree arc
  // mapping to 110 degree to give full pot sweep for finer control
  // note i'm using 3.3 volts (204.6 * 3.3) as the pin was easier to solder to...
  // write the converted voltage
  
  data.Rudd =  map(Rudd, 0, 690, 35, 145);
 
 
  // now set the sail adjustment
  SailIn = digitalRead(SIN_PIN) ;
  SailOu = digitalRead(SOU_PIN) ;
  // for this constant rotation servo 0 is CCW, 180 is CW and 90 is off
  // other values set the direction and speed
  // i want the servo to run slower to avoild tangling the sheet line so
  // using 70 for CCW, 110 for CW and 90 for off
  // the transmitte simply sends these values depending on what switched is pressed
  if (SailIn == LOW & SailOu == HIGH) {
    // bring sail in
    data.Sail = 70;
  }
  else if (SailIn == HIGH & SailOu == LOW) {
    // bring sail out
    data.Sail = 110;
  }
  else {
    // stop sail
    data.Sail = 90;
  }

   Serial.println(data.Rudd);
  Serial.println(data.Sail);
  Serial.println(" ");

  // write the data to the radio
  radio.write(&data, sizeof(data));

  // future
  // 1. write a call back to ask what was recieved and verify
  // 2. adjust power lower but if recieve can find the radio adjust the power higher
  // 3. come home function with GPS so if radio is lost boat comes home


}

*/

