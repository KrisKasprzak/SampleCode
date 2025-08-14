/*

  Free code for the NeoPixed garage attendent.
  uses a ultrasonic distance sensor to find distance to front of car
  illuminates pixels for the distance
  that way you don't smash into things

*/


#include <Adafruit_NeoPixel.h>
#include <Ultrasonic.h> // https://github.com/ErickSimoes/Ultrasonic/tree/master

#define PIN_SIG        8
#define PIN_ECHO       9
#define PIN_TRIG       10
#define PIN_DIST       A0

#define NUMPIXELS 60
#define MINBRIGHT 1
#define MAXBRIGHT 100

// needed for Arduino in handling timeout
unsigned long OldTime;

// distance measuring variables
int Distance, OldDistance, Offset;
byte i, Pulse = 0;
bool Run = false;
bool Up = true;

// create the light strip object
Adafruit_NeoPixel LightStrip(NUMPIXELS, PIN_SIG, NEO_GRB + NEO_KHZ800);

// increase timeout to measure longer distance
Ultrasonic UDist(PIN_ECHO, PIN_TRIG, 30000UL);

// include if you are using a Teensy
// elapsedMillis ElapsedTime;

void setup() {

  Serial.begin(9600);

  // initialize the light strip
  LightStrip.begin();

}

void loop() {
  Distance = 0;

  // some distance flickering so do some averaging
  for (i = 0; i < 5; i++) {
    Distance = Distance + UDist.read(INC) ;
  }
  // average the distance
  Distance = Distance / 5;

  // my neopixel as 60 leds but i'd like to scale the 0-60 to 0 to 100 inches
  Distance = map(Distance, 0, 100, 0, 60);

  // do some range checking to keep pixels in 0-60
  // recall we are mapping 0 to 100 inches, so the range can be outside the
  // pixel count
  if (Distance > 60) {
    Distance = 60;
  }

  if (Distance < 0) {
    Distance = 0;
  }

  // add up to another 20 inches for adjustment
  // this lets' you calibrate the unit so you can account for large bumpers
  // stuff near the wall etc
  // simple here just divide 1024 measured bits by 50 to get 20
  Offset = analogRead(PIN_DIST) / 50;

  // if activity, reset counter which will wake up the unit, and display measured distance
  if (abs(Distance - OldDistance) > 2) {
    OldDistance = Distance;
    ElapsedTime = 0;
    Run = true;
  }

  // code for arduino (poor mans wrap around fixer)
  // my timeout is 8 seconds
  if (abs(millis() - OldTime) > 8000) {
    OldTime = millis();
    LightStrip.clear();
    Run = false;
  }

  // use this code if you are using a teensy
  /*
    if (ElapsedTime > 8000) {
      ElapsedTime = 0;
      LightStrip.clear();
      Run = false;
    }
  */

  if (Run) {

    Distance = Distance - Offset;

    if (Distance > 20) {
      // in the yellow region
      LightStrip.fill(LightStrip.Color(MINBRIGHT, MINBRIGHT, 0), 0, 40); // yellow
      LightStrip.fill(LightStrip.Color(0, MINBRIGHT, 0), 40, 10); // green
      LightStrip.fill(LightStrip.Color(MINBRIGHT, 0, 0), 50, 10); // red
      // now paint amount of yellow
      if (Distance < 60) {
        LightStrip.fill(LightStrip.Color(MAXBRIGHT, MAXBRIGHT, 0), 0, 60 - Distance); // yellow
      }
    }
    else if (( Distance > 10) && (Distance <= 20) ) {
      // in the green region
      LightStrip.fill(LightStrip.Color(MAXBRIGHT, MAXBRIGHT, 0), 0, 40); // yellow
      LightStrip.fill(LightStrip.Color(0, MINBRIGHT, 0), 40, 10); // green
      LightStrip.fill(LightStrip.Color(MINBRIGHT, 0, 0), 50, 10); // red
      // now paint amount of green
      LightStrip.fill(LightStrip.Color(0, MAXBRIGHT, 0), 40, 21 - Distance); // green
    }
    else if (( Distance >= 0) && (Distance <= 10) ) {
      // in the red region
      LightStrip.fill(LightStrip.Color(MAXBRIGHT, MAXBRIGHT, 0), 0, 40); // yellow
      LightStrip.fill(LightStrip.Color(0, MAXBRIGHT, 0), 40, 10); // green
      LightStrip.fill(LightStrip.Color(MINBRIGHT, 0, 0), 50, 10); // red
      // now paint amount of green
      LightStrip.fill(LightStrip.Color(MAXBRIGHT, 0, 0), 50, 11 - Distance); // red
    }


  }


  // OK my little blue LED pulsing in brightness to let you know the unit
  // is waiting..really don't need this :)
  if (!Run) {

    if (Pulse == 100) {
      Up = false;
    }
    if (Pulse == 0) {
      Up = true;
    }
    if (Up) {
      Pulse += 5;
    }
    else {
      Pulse -= 5;
    }
    LightStrip.setPixelColor(0, LightStrip.Color(0, 0, Pulse));
    delay(10);
  }
  else {
    LightStrip.setPixelColor(0, LightStrip.Color(0, 0, 100));
    delay(50);
  }

  // now that we have built the LED array, fire them up
  LightStrip.show();


}

