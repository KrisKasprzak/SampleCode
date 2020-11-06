/*

  This program is for upsizing an 8 x 8 array of thermal camera readings
  it will size up by 10x and display to a 240 x 320
  interpolation is linear and "good enough" given the display is a 5-6-5 color palet
  Total final array is an array of 70 x 70 of internal points only

  Revisions
  1.0     Kasprzak      Initial code
  2.0     Kasprzak      removed functions for better speed (and went from 2.37 to 4.44 fps

  MCU                       https://www.amazon.com/Teensy-3-2-with-pins/dp/B015QUPO5Y/ref=sr_1_2?s=industrial&ie=UTF8&qid=1510373806&sr=1-2&keywords=teensy+3.2
  Display                   https://www.amazon.com/Wrisky-240x320-Serial-Module-ILI9341/dp/B01KX26JJU/ref=sr_1_10?ie=UTF8&qid=1510373771&sr=8-10&keywords=240+x+320+tft
  Thermal sensor            https://learn.adafruit.com/adafruit-amg8833-8x8-thermal-camera-sensor/overview
  display library           https://github.com/PaulStoffregen/ILI9341_t3
  font library              https://github.com/PaulStoffregen/ILI9341_fonts
  sensor library            https://github.com/adafruit/Adafruit_AMG88xx
  touchscreen lib           https://github.com/dgolda/UTouch
  equations generated from  http://web-tech.ga-usa.com/2012/05/creating-a-custom-hot-to-cold-temperature-color-gradient-for-use-with-rrdtool/index.html

  Pinouts
  MCU         Device
  A4          AMG SDA
  A5          AMG SCL
  Gnd         Dispaly GND, AMG Gnd
  3v3         Dispaly Vcc,Display LED,Display RST, AMG Vcc
  2           Dispaly T_CLK
  3           Dispaly T_CS
  4           Dispaly T_DIN
  5           Dispaly T_DO
  6           Dispaly T_IRQ
  9           Display D/C
  10          Display CS
  11          Display MOSI
  12          Dispaly MISO
  13          Display SCK

*/

#include "ILI9341_t3.h"             // very fast library
#include "font_ArialBoldItalic.h"   // fonts for the startup screen
#include "font_Arial.h"             // fonts for the legend
#include "font_DroidSans.h"         // fonts for the startup screen
// #include "Fonts\kris.h"         // fonts for the startup screen
#include <Adafruit_AMG88xx.h>       // thermal camera lib
#include <UTouch.h>                 // touchscreen lib

#define PIN_CS 10                   // chip select for the display
#define PIN_DC 9                    // d/c pin for the display

// constants for the cute little keypad
#define KEYPAD_TOP 15
#define KEYPAD_LEFT 50
#define BUTTON_W 60
#define BUTTON_H 30
#define BUTTON_SPACING_X 10
#define BUTTON_SPACING_Y 10
#define BUTTON_TEXTSIZE 2

// fire up the display using a very fast driver
// this next line is for my modified library where I pass the screen dimensions in--that way i can use the same lib for my 3.5", 2.8" and other sizes
// ILI9341_t3 Display = ILI9341_t3(PIN_CS, PIN_DC, 240, 320);

// you will need to use this line
ILI9341_t3 Display = ILI9341_t3(PIN_CS, PIN_DC, 240, 320);


// create some colors for the keypad buttons
#define C_BLUE Display.color565(0,0,255)
#define C_RED Display.color565(255,0,0)
#define C_GREEN Display.color565(0,255,0)
#define C_WHITE Display.color565(255,255,255)
#define C_BLACK Display.color565(0,0,0)
#define C_LTGREY Display.color565(200,200,200)
#define C_DKGREY Display.color565(80,80,80)
#define C_GREY Display.color565(127,127,127)


unsigned long CurTime;

// create some text for the keypad butons
char KeyPadBtnText[12][5] = {"1", "2", "3", "4", "5", "6", "7", "8", "9", "Done", "0", "Clr" };

// define some colors for the keypad buttons
uint16_t KeyPadBtnColor[12] = {C_BLUE, C_BLUE, C_BLUE, C_BLUE, C_BLUE, C_BLUE, C_BLUE, C_BLUE, C_BLUE, C_GREEN, C_BLUE, C_RED };
uint16_t TheColor;
// start with some initial colors
uint16_t MinTemp = 25;
uint16_t MaxTemp = 35;

// variables for interpolated colors
byte red, green, blue;

// variables for row/column interpolation
byte i, j, k, row, col, incr;
float intPoint, val, a, b, c, d, ii;
byte aLow, aHigh;

// size of a display "pixel"
byte BoxWidth = 3;
byte BoxHeight = 3;


uint16_t PixelColor;


int x, y;
char buf[20];

// variable to toggle the display grid
int ShowGrid = -1;
int DefaultTemp = -1;

// array for the 8 x 8 measured pixels
float pixels[64];

// array for the interpolated array
float HDTemp[80][80];



char name[] = "9px_0000.bmp";       // filename convention (will auto-increment)
const int w = 320;                   // image width in pixels
const int h = 240;                    // " height
const boolean debugPrint = true;    // print details of process over serial?

const int imgSize = w * h;
int px[w * h];                      // actual pixel data (grayscale - added programatically below)

const uint8_t cardPin = 8;          // pin that the SD is connected to (d8 for SparkFun MicroSD shield)



// create the keypad buttons
// note the ILI9438_3t library makes use of the Adafruit_GFX library (which makes use of the Adafruit_button library)
Adafruit_GFX_Button KeyPadBtn[12];

// create the camara object
Adafruit_AMG88xx ThermalSensor;

// create the touch screen object
UTouch  Touch( 2, 3, 4, 5, 6);

void setup() {

  //  Serial.begin(9600);

  // start the display and set the background to black
  Display.begin();
  Display.fillScreen(C_BLACK);

  // initialize the touch screen and set location precision
  Touch.InitTouch();
  Touch.setPrecision(PREC_EXTREME);

  // create the keypad buttons
  for (row = 0; row < 4; row++) {
    for (col = 0; col < 3; col++) {
      KeyPadBtn[col + row * 3].initButton(&Display, BUTTON_H + BUTTON_SPACING_X + KEYPAD_LEFT + col * (BUTTON_W + BUTTON_SPACING_X ),
                                          KEYPAD_TOP + 2 * BUTTON_H + row * (BUTTON_H + BUTTON_SPACING_Y),
                                          BUTTON_W, BUTTON_H, C_WHITE, KeyPadBtnColor[col + row * 3], C_WHITE,
                                          KeyPadBtnText[col + row * 3], BUTTON_TEXTSIZE);
    }
  }

  // set display rotation (you may need to change to 0 depending on your display
  Display.setRotation(3);


  // show a cute splash screen (paint text twice to show a little shadow
  Display.setFont(DroidSans_40);
  Display.setCursor(22, 21);
  Display.setTextColor(C_WHITE, C_BLACK);
  Display.print("Thermal");

  Display.setFont(DroidSans_40);
  Display.setCursor(20, 20);
  Display.setTextColor(C_BLUE, C_BLACK);
  Display.print("Thermal");

  Display.setFont(Arial_48_Bold_Italic);
  Display.setCursor(52, 71);
  Display.setTextColor(C_WHITE, C_BLACK);
  Display.print("Camera");

  Display.setFont(Arial_48_Bold_Italic);
  Display.setCursor(50, 70);
  Display.setTextColor(C_RED, C_BLACK);
  Display.print("Camera");


  // let sensor boot up
  bool status = ThermalSensor.begin();
  delay(100);

  // check status and display results
  if (!status) {
    while (1) {
      Display.setFont(DroidSans_20);
      Display.setCursor(20, 150);
      Display.setTextColor(C_RED, C_BLACK);
      Display.print("Sensor: FAIL");
      delay(500);
      Display.setFont(DroidSans_20);
      Display.setCursor(20, 150);
      Display.setTextColor(C_BLACK, C_BLACK);
      Display.print("Sensor: FAIL");
      delay(500);
    }
  }
  else {
    Display.setFont(DroidSans_20);
    Display.setCursor(20, 150);
    Display.setTextColor(C_GREEN, C_BLACK);
    Display.print("Sensor: FOUND");
  }

  // read the camera for initial testing
  ThermalSensor.readPixels(pixels);

  // check status and display results
  if (pixels[0] < 0) {
    while (1) {
      Display.setFont(DroidSans_20);
      Display.setCursor(20, 180);
      Display.setTextColor(C_RED, C_BLACK);
      Display.print("Readings: FAIL");
      delay(500);
      Display.setFont(DroidSans_20);
      Display.setCursor(20, 180);
      Display.setTextColor(C_BLACK, C_BLACK);
      Display.print("Readings: FAIL");
      delay(500);
    }
  }
  else {
    Display.setFont(DroidSans_20);
    Display.setCursor(20, 180);
    Display.setTextColor(C_GREEN, C_BLACK);
    Display.print("Readings: OK");
    delay(2000);
  }

  // set display rotation and clear the fonts..the rotation of this display is a bit weird

  Display.setFontAdafruit();
  Display.fillScreen(C_BLACK);

  // get the cutoff points for the color interpolation routines
  // note this function called when the temp scale is changed
  Getabcd();

  // draw a cute legend with the scale that matches the sensors max and min
  DrawLegend();

  // draw a large white border for the temperature area
  Display.fillRect(10, 10, 220, 220, C_WHITE);

  Display.setRotation(2);


}

void loop() {

  //  CurTime = millis();

  // if someone touched the screen do something with it
  if (Touch.dataAvailable()) {
    ProcessTouch();

  }

  // read the sensor
  ThermalSensor.readPixels(pixels);
  // InterpolateRows();
  // now that we have an 8 x 8 sensor array
  // interpolate to get a bigger screen
  // interpolate the 8 rows (interpolate the 70 column points between the 8 sensor pixels first)
  for (row = 0; row < 8; row ++) {
    for (col = 0; col < 70; col ++) {
      // get the first array point, then the next
      // also need to bump by 8 for the subsequent rows
      aLow =  col / 10 + (row * 8);
      aHigh = (col / 10) + 1 + (row * 8);
      // get the amount to interpolate for each of the 10 columns
      // here were doing simple linear interpolation mainly to keep performace high and
      // display is 5-6-5 color palet so fancy interpolation will get lost in low color depth
      intPoint =   (( pixels[aHigh] - pixels[aLow] ) / 10.0 );
      // determine how much to bump each column (basically 0-9)
      incr = col % 10;
      // find the interpolated value
      val = (intPoint * incr ) +  pixels[aLow];
      // store in the 70 x 70 array
      // since display is pointing away, reverse row to transpose row data
      HDTemp[ (7 - row) * 10][col] = val;

    }
  }

  // now that we have row data with 70 columns
  // interpolate each of the 70 columns
  // forget Arduino..no where near fast enough..Teensy at > 72 mhz is the starting point
  //  InterpolateCols();
  for (col = 0; col < 70; col ++) {
    for (row = 0; row < 70; row ++) {
      // get the first array point, then the next
      // also need to bump by 8 for the subsequent cols
      aLow =  (row / 10 ) * 10;
      aHigh = aLow + 10;
      // get the amount to interpolate for each of the 10 columns
      // here were doing simple linear interpolation mainly to keep performace high and
      // display is 5-6-5 color palet so fancy interpolation will get lost in low color depth
      intPoint =   (( HDTemp[aHigh][col] - HDTemp[aLow][col] ) / 10.0 );
      // determine how much to bump each column (basically 0-9)
      incr = row % 10;
      // find the interpolated value
      val = (intPoint * incr ) +  HDTemp[aLow][col];
      // store in the 70 x 70 array
      HDTemp[ row ][col] = val;
    }
  }
  // display the 70 x 70 array
  //  DisplayGradient();
  // Display.setRotation(2);

  // rip through 70 rows
  for (row = 0; row < 70; row ++) {

    // fast way to draw a non-flicker grid--just make every 10 pixels 2x2 as opposed to 3x3
    // drawing lines after the grid will just flicker too much
    if (ShowGrid < 0) {
      BoxWidth = 3;
    }
    else {
      if ((row % 10 == 9) ) {
        BoxWidth = 2;
      }
      else {
        BoxWidth = 3;
      }
    }
    // then rip through each 70 cols
    for (col = 0; col < 70; col++) {

      // fast way to draw a non-flicker grid--just make every 10 pixels 2x2 as opposed to 3x3
      if (ShowGrid < 0) {
        BoxHeight = 3;
      }
      else {
        if ( (col % 10 == 9)) {
          BoxHeight = 2;
        }
        else {
          BoxHeight = 3;
        }
      }
      // finally we can draw each the 70 x 70 points, note the call to get interpolated color

      val = HDTemp[ row ][col];

      red = constrain(255.0 / (c - b) * val - ((b * 255.0) / (c - b)), 0, 255);

      if ((val > MinTemp) & (val < a)) {
        green = constrain(255.0 / (a - MinTemp) * val - (255.0 * MinTemp) / (a - MinTemp), 0, 255);
      }
      else if ((val >= a) & (val <= c)) {
        green = 255;
      }
      else if (val > c) {
        green = constrain(255.0 / (c - d) * val - (d * 255.0) / (c - d), 0, 255);
      }
      else if ((val > d) | (val < a)) {
        green = 0;
      }

      if (val <= b) {
        blue = constrain(255.0 / (a - b) * val - (255.0 * b) / (a - b), 0, 255);
      }
      else if ((val > b) & (val <= d)) {
        blue = 0;
      }
      else if (val > d) {
        blue = constrain(240.0 / (MaxTemp - d) * val - (d * 240.0) / (MaxTemp - d), 0, 240);
      }

      // use the displays color mapping function to get 5-6-5 color palet (R=5 bits, G=6 bits, B-5 bits)
      //  TheColor =  Display.color565(red, green, blue);

      TheColor = ((red & 0xF8) << 8) | ((green & 0xFC) << 3) | (blue >> 3);
      Display.fillRect((row * 3) + 15, (col * 3) + 15, BoxWidth, BoxHeight, TheColor);
    }
  }

}



/*
  // interplation function to create 70 columns for 8 rows
  void InterpolateRows() {

  // interpolate the 8 rows (interpolate the 70 column points between the 8 sensor pixels first)
  for (row = 0; row < 8; row ++) {
    for (col = 0; col < 70; col ++) {
      // get the first array point, then the next
      // also need to bump by 8 for the subsequent rows
      aLow =  col / 10 + (row * 8);
      aHigh = (col / 10) + 1 + (row * 8);
      // get the amount to interpolate for each of the 10 columns
      // here were doing simple linear interpolation mainly to keep performace high and
      // display is 5-6-5 color palet so fancy interpolation will get lost in low color depth
      intPoint =   (( pixels[aHigh] - pixels[aLow] ) / 10.0 );
      // determine how much to bump each column (basically 0-9)
      incr = col % 10;
      // find the interpolated value
      val = (intPoint * incr ) +  pixels[aLow];
      // store in the 70 x 70 array
      // since display is pointing away, reverse row to transpose row data
      HDTemp[ (7 - row) * 10][col] = val;

    }
  }
  }
*/

/*
  // interplation function to interpolate 70 columns from the interpolated rows
  void InterpolateCols() {

  // then interpolate the 70 rows between the 8 sensor points
  for (col = 0; col < 70; col ++) {
    for (row = 0; row < 70; row ++) {
      // get the first array point, then the next
      // also need to bump by 8 for the subsequent cols
      aLow =  (row / 10 ) * 10;
      aHigh = aLow + 10;
      // get the amount to interpolate for each of the 10 columns
      // here were doing simple linear interpolation mainly to keep performace high and
      // display is 5-6-5 color palet so fancy interpolation will get lost in low color depth
      intPoint =   (( HDTemp[aHigh][col] - HDTemp[aLow][col] ) / 10.0 );
      // determine how much to bump each column (basically 0-9)
      incr = row % 10;
      // find the interpolated value
      val = (intPoint * incr ) +  HDTemp[aLow][col];
      // store in the 70 x 70 array
      HDTemp[ row ][col] = val;
    }
  }
  }
*/

/*
  // function to display the results
  void DisplayGradient() {

  Display.setRotation(2);

  // rip through 70 rows
  for (row = 0; row < 70; row ++) {

    // fast way to draw a non-flicker grid--just make every 10 pixels 2x2 as opposed to 3x3
    // drawing lines after the grid will just flicker too much
    if (ShowGrid < 0) {
      BoxWidth = 3;
    }
    else {
      if ((row % 10 == 9) ) {
        BoxWidth = 2;
      }
      else {
        BoxWidth = 3;
      }
    }
    // then rip through each 70 cols
    for (col = 0; col < 70; col++) {

      // fast way to draw a non-flicker grid--just make every 10 pixels 2x2 as opposed to 3x3
      if (ShowGrid < 0) {
        BoxHeight = 3;
      }
      else {
        if ( (col % 10 == 9)) {
          BoxHeight = 2;
        }
        else {
          BoxHeight = 3;
        }
      }
      // finally we can draw each the 70 x 70 points, note the call to get interpolated color
      Display.fillRect((row * 3) + 15, (col * 3) + 15, BoxWidth, BoxHeight, GetColor(HDTemp[row][col]));
    }
  }

  Display.setRotation(3);

  }
*/


// my fast yet effective color interpolation routine
uint16_t GetColor(float val) {

  /*
    pass in value and figure out R G B
    several published ways to do this I basically graphed R G B and developed simple linear equations
    again a 5-6-5 color display will not need accurate temp to R G B color calculation

    equations based on
    http://web-tech.ga-usa.com/2012/05/creating-a-custom-hot-to-cold-temperature-color-gradient-for-use-with-rrdtool/index.html

  */

  red = constrain(255.0 / (c - b) * val - ((b * 255.0) / (c - b)), 0, 255);

  if ((val > MinTemp) & (val < a)) {
    green = constrain(255.0 / (a - MinTemp) * val - (255.0 * MinTemp) / (a - MinTemp), 0, 255);
  }
  else if ((val >= a) & (val <= c)) {
    green = 255;
  }
  else if (val > c) {
    green = constrain(255.0 / (c - d) * val - (d * 255.0) / (c - d), 0, 255);
  }
  else if ((val > d) | (val < a)) {
    green = 0;
  }

  if (val <= b) {
    blue = constrain(255.0 / (a - b) * val - (255.0 * b) / (a - b), 0, 255);
  }
  else if ((val > b) & (val <= d)) {
    blue = 0;
  }
  else if (val > d) {
    blue = constrain(240.0 / (MaxTemp - d) * val - (d * 240.0) / (MaxTemp - d), 0, 240);
  }

  // use the displays color mapping function to get 5-6-5 color palet (R=5 bits, G=6 bits, B-5 bits)
  return Display.color565(red, green, blue);

}

// function to automatically set the max / min scale based on adding an offset to the average temp from the 8 x 8 array
// you could also try setting max and min based on the actual max min
void SetTempScale() {

  if (DefaultTemp < 0) {
    MinTemp = 25;
    MaxTemp = 35;
    Getabcd();
    DrawLegend();
  }
  else {

    val = 0.0;
    for (i = 0; i < 64; i++) {
      val = val + pixels[i];
    }
    val = val / 64.0;

    MaxTemp = val + 2.0;
    MinTemp = val - 2.0;
    Getabcd();
    DrawLegend();
  }

}

// function to get the cutoff points in the temp vs RGB graph
void Getabcd() {

  a = MinTemp + (MaxTemp - MinTemp) * 0.2121;
  b = MinTemp + (MaxTemp - MinTemp) * 0.3182;
  c = MinTemp + (MaxTemp - MinTemp) * 0.4242;
  d = MinTemp + (MaxTemp - MinTemp) * 0.8182;

}

// function to handle screen touches
void ProcessTouch() {

  Display.setRotation(3);
  Touch.read();

  x = Touch.getY();
  y = Touch.getX();
  x  = map(x, 0, 235, 320, 0);
  y  = map(y, 379, 0, 240, 0);


  // yea i know better to have buttons
  if (x > 200) {
    if (y < 80) {
      KeyPad(MaxTemp);
    }
    else if (y > 160) {
      KeyPad(MinTemp);
    }
    else {
      DefaultTemp = DefaultTemp * -1;
      SetTempScale();
    }
  }

  else if (x <= 200) {
    // toggle grid
    ShowGrid = ShowGrid * -1;
    if (ShowGrid > 0) {
      Display.fillRect(15, 15, 210, 210, C_BLACK);
    }
  }

  Display.setRotation(2);
}

// function to draw a cute little legend
void DrawLegend() {

  // my cute little color legend with max and min text
  j = 0;

  float inc = (MaxTemp - MinTemp ) / 160.0;

  for (ii = MinTemp; ii < MaxTemp; ii += inc) {
    Display.drawFastHLine(260, 200 - j++, 30, GetColor(ii));
  }

  Display.setTextSize(2);
  Display.setCursor(245, 20);
  Display.setTextColor(C_WHITE, C_BLACK);
  sprintf(buf, "%2d/%2d", MaxTemp, (int) (MaxTemp * 1.8) + 32);
  Display.fillRect(233, 15, 94, 22, C_BLACK);
  Display.setFont(Arial_14);
  Display.print(buf);

  Display.setTextSize(2);
  // Display.setFont(Arial_24_Bold);
  Display.setCursor(245, 220);
  Display.setTextColor(C_WHITE, C_BLACK);
  sprintf(buf, "%2d/%2d", MinTemp, (int) (MinTemp * 1.8) + 32);
  Display.fillRect(233, 215, 94, 55, C_BLACK);
  Display.setFont(Arial_14);
  Display.print(buf);


}

// function to draw a numeric keypad
void KeyPad(uint16_t &TheNumber) {

  int left = KEYPAD_LEFT;
  int top = KEYPAD_TOP;
  int wide = (3 * BUTTON_W ) + (4 * BUTTON_SPACING_X);
  int high = (5 * BUTTON_H) +  (6 * BUTTON_SPACING_Y);
  int TempNum = TheNumber;
  bool KeepIn = true;

  Display.fillRect(left, top, wide , high, C_DKGREY);
  Display.drawRect(left, top, wide , high, C_LTGREY);

  Display.fillRect(left + 10, top + 10, wide - 20 , 30, C_WHITE);
  Display.drawRect(left + 10, top + 10, wide - 20 , 30, C_DKGREY);

  Display.setCursor(left  + 20 , top + 20);
  Display.setTextColor(C_BLACK, C_WHITE);

  Display.print(TheNumber);

  for (row = 0; row < 4; row++) {
    for (col = 0; col < 3; col++) {
      KeyPadBtn[col + row * 3].drawButton();
    }
  }
  delay(300); // debounce
  while (KeepIn) {
    // get the touch point



    // if nothing or user didn't press hard enough, don't do anything
    if (Touch.dataAvailable()) {
      Touch.read();
      x = Touch.getY();
      y = Touch.getX();
      x  = map(x, 0, 235, 320, 0);
      y  = map(y, 379, 0, 240, 0);
      // go thru all the KeyPadBtn, checking if they were pressed
      for (byte b = 0; b < 12; b++) {
        if (PressIt(KeyPadBtn[b], x, y) == true) {

          if ((b < 9) | (b == 10)) {

            TempNum *= 10;
            if (TempNum == 0) {
              Display.fillRect(left + 10, top + 10, wide - 20 , 30, C_WHITE);
            }

            if (b != 10) {
              TempNum += (b + 1);
            }

            if (TempNum > 80) {
              Display.fillRect(left + 10, top + 10, wide - 20 , 30, C_WHITE);
              //Display.setCursor(left  + 100 , top + 20);
              //Display.setTextColor(C_RED, C_WHITE);
              //Display.print("Max 100");
              TempNum = 0;
              TempNum += (b + 1);
            }
          }
          // clr button
          if (b == 11) {
            Display.fillRect(left + 10, top + 10, wide - 20 , 30, C_WHITE);
            Display.drawRect(left + 10, top + 10, wide - 20 , 30, C_DKGREY);
            TempNum = 0;
          }
          if (b == 9) {
            KeepIn = false;
          }

          Display.setCursor(left  + 20 , top + 20);
          Display.setTextColor(C_BLACK, C_WHITE);

          Display.print(TempNum);

        }
      }
    }
  }

  // clear screen redraw previous screen
  // update the Current Channel
  TheNumber = TempNum;

  Display.fillRect(0, 0, 319, 239, C_BLACK);
  Display.fillRect(10, 10, 220, 220, C_WHITE);
  Display.fillRect(15, 15, 210, 210, C_BLACK);
  Getabcd();
  DrawLegend();

}

// function to handle color inversion of a pressed button
bool PressIt(Adafruit_GFX_Button TheButton, int x, int y) {


  if (TheButton.contains(x, y)) {
    TheButton.press(true);  // tell the button it is pressed
  } else {
    TheButton.press(false);  // tell the button it is NOT pressed
  }
  if (TheButton.justReleased()) {
    TheButton.drawButton();  // draw normal
  }
  if (TheButton.justPressed()) {
    TheButton.drawButton(true);  // draw invert!
    delay(200); // UI debouncing
    TheButton.drawButton(false);  // draw invert!
    return true;
  }
  return false;
}


// end of code
