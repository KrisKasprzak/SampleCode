/*

  This program is a microphone montitor for a DSLR, it will sit between a mic and DSLR input
  so the user can control the mic levels and see the levels.

  compile 96 mhz, debug or fastest

  rev   data          Description               author
  1.0   4/8/2018      initial creation          Kasprzak
  2.0   4/14/2018     added filters             Kasprzak
  3.0   5/12/2018     added 30 band FFT         Kasprzak
  4.0   5/20/2018     added cute splash screen  Kasprzak
  5.0   6/2/2018      added mic level reminder  Kasprzak

  connection map

  Teensy 3.2     Display

  0              Touch pin
  1              Touch pin
  2              Touch pin
  3              Touch pin
  4              Touch pin
  5              LED pin (PWM to control brightness)
  6
  7              MOSI
  8
  9
  10
  11
  12             MISO
  13
  14             SCK
  A1
  A2
  A3
  A4
  A5
  A6             Chip Select
  A7             DC
  A8
  A9

*/

// #define Debug

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#include <ILI9341_t3.h>
#include <Colors.h>
#include "UTouch.h"
#include <font_ArialBold.h>   // custom fonts that ships with ILI9341_t3.h
#include <font_ArialBlack.h>  // custom fonts that ships with ILI9341_t3.h
#include <font_Arial.h>       // custom fonts that ships with ILI9341_t3.h
#include <EEPROM.h>

// some audip defines and pin defines
#define FILTER_Q      .5
#define MIC_BAR_GAIN  20.0
#define LED_PIN       5
#define DC_PIN        21
#define BAT_PIN       16
#define CS_PIN        20
#define PIN_RST       255  // 255 = unused, connect to 3.3V
#define MO_PIN        7
#define Display_SCLK  14
#define MI_PIN        12
#define WIDTH         7
#define OFFSET        25
#define HIGHPASS      60
#define LOWPASS       8000

// variables for the locations of the keypad buttons
#define BUTTON_X 100
#define BUTTON_Y 80
#define BUTTON_W 60
#define BUTTON_H 30
#define BUTTON_SPACING_X 10
#define BUTTON_SPACING_Y 10
#define BUTTON_TEXTSIZE 2

// GUItool: automatically generated code from
// https://www.pjrc.com/teensy/gui/index.html
AudioInputI2S            i2s1;
AudioMixer4              mixer1;
AudioFilterBiquad        biquad1;
AudioAnalyzeFFT1024      fft1024_1;
AudioOutputI2S           i2s2;
AudioConnection          patchCord1(i2s1, 0, mixer1, 0);
AudioConnection          patchCord2(i2s1, 1, mixer1, 1);
AudioConnection          patchCord3(mixer1, biquad1);
AudioConnection          patchCord4(biquad1, 0, i2s2, 1);
AudioConnection          patchCord5(biquad1, 0, i2s2, 0);
AudioConnection          patchCord6(biquad1, fft1024_1);
AudioControlSGTL5000     sgtl5000_1;

// create the display object
// note I've modifed the library to accept the screen size, that way I can drive different size displays
// from the same lib--i just pass in the size

ILI9341_t3 Display = ILI9341_t3(CS_PIN, DC_PIN, 240, 320, PIN_RST, MO_PIN, Display_SCLK, MI_PIN);

// some more varibales
int left,   top, wide, high;
unsigned long curtime, pretime;

int TempNum;
int HighPassF = 0;
int LowPassF = 0;
int MicGain = 5;
float BatVolts = 0.0;
byte ShowHistory = 0;
byte Background = 0;
byte Brightness = 255;
byte HighPass = 0;
byte LowPass = 0;
uint16_t BackColor = C_BLACK; // i have a custom "Colors.h" that lists all the colors i use
uint16_t ForeColor = C_WHITE;
unsigned long Showit;
int oButtonLocation;
int ButtonLocation;

char FilterText[2][5] = {"Off", "On" };
char HistoryText[2][5] = {"No", "Yes" };

// keypad code taken from Adafruit examples
char KeyPadBtnText[12][5] = {"1", "2", "3", "4", "5", "6", "7", "8", "9", "Done", "0", "Clr" };
uint16_t KeyPadBtnColor[12] = {C_BLUE, C_BLUE, C_BLUE,
                               C_BLUE, C_BLUE, C_BLUE,
                               C_BLUE, C_BLUE, C_BLUE,
                               C_DKGREEN, C_BLUE, C_DKRED
                              };
byte Qty, j, i, row, col, b;
int x, y, h, w, loval, hival, Count;
int line1;

volatile float MixerGain;
// An array to hold the 32 frequency bands
volatile float level[32];
volatile float Max[32];
float BatLevel;

// create the touch object
UTouch  Touch( 0, 1, 2, 3, 4);

Adafruit_GFX_Button KeyPadBtn[12];
Adafruit_GFX_Button SetupBtn;
Adafruit_GFX_Button HistoryBtn;
Adafruit_GFX_Button MicGainBtn;
Adafruit_GFX_Button BackColorBtn;
Adafruit_GFX_Button HighPassFBtn;
Adafruit_GFX_Button LowPassFBtn;
Adafruit_GFX_Button DoneBtn;
Adafruit_GFX_Button HighPassBtn;
Adafruit_GFX_Button LowPassBtn;
Adafruit_GFX_Button OKBtn;

void setup() {

  Serial.begin(9600);

  AudioMemory(60);
  pinMode(BAT_PIN, INPUT);
  Display.begin();
  Display.fillScreen(C_BLACK);
  Display.setRotation(3);

  // I save basic parameters, like background color and filter values to the EEPROM
  GetParameters();

  if (Background == 0) {
    BackColor = C_BLACK;
    ForeColor = C_WHITE;
  }
  else {
    BackColor = C_WHITE;
    ForeColor = C_BLACK;
  }

  // set the display background brightness
  analogWrite(LED_PIN, Brightness);
  Display.fillScreen(BackColor);

  // start setting the audio board parameters
  // i really have no idea if these are the optimal settings
  // after days of trial and error, they seem to be the best
  sgtl5000_1.enable();
  sgtl5000_1.inputSelect(AUDIO_INPUT_LINEIN);
  sgtl5000_1.volume(.85);
  sgtl5000_1.lineInLevel(MicGain);
  sgtl5000_1.lineOutLevel(50);
  sgtl5000_1.audioPreProcessorEnable();

  // setup the FFT
  fft1024_1.windowFunction(AudioWindowHanning1024);

  // initilize the touch screen
  Touch.InitTouch();
  Touch.setPrecision(PREC_EXTREME);

  // create all the buttons
  SetupBtn.initButton(&Display,     289, 18, 60, 29, C_WHITE, C_DKGREY, C_LTGREY, "SET", 2);
  OKBtn.initButton(&Display,        280, 18, 80, 29, C_WHITE, C_DKGREY, C_LTGREY, "OK", 2);
  DoneBtn.initButton(&Display,      270, 18, 80, 29, C_WHITE, C_DKGREY, C_LTGREY, "Done", 2);

  HistoryBtn.initButton(&Display,   225, 70, 50, 25, C_DKGREY, C_GREY, C_BLACK, "Set", 2);
  MicGainBtn.initButton(&Display,   225, 100, 50, 25, C_DKGREY, C_GREY, C_BLACK, "Set", 2);
  HighPassBtn.initButton(&Display,  225, 130, 50, 25, C_DKGREY, C_GREY, C_BLACK, "Set", 2);
  LowPassBtn.initButton(&Display,   225, 160, 50, 25, C_DKGREY, C_GREY, C_BLACK, "Set", 2);
  HighPassFBtn.initButton(&Display, 280, 130, 50, 25, C_DKGREY, C_GREY, C_BLACK, "Set", 2);
  LowPassFBtn.initButton(&Display,  280, 160, 50, 25, C_DKGREY, C_GREY, C_BLACK, "Set", 2);
  BackColorBtn.initButton(&Display, 225, 190, 50, 25, C_DKGREY, C_GREY, C_BLACK, "Set", 2);

  // create KeyPadBtn
  for (row = 0; row < 4; row++) {
    for (col = 0; col < 3; col++) {
      KeyPadBtn[col + row * 3].initButton(&Display, BUTTON_X + col * (BUTTON_W + BUTTON_SPACING_X),
                                          BUTTON_Y + row * (BUTTON_H + BUTTON_SPACING_Y),
                                          BUTTON_W, BUTTON_H, C_WHITE, KeyPadBtnColor[col + row * 3], C_WHITE,
                                          KeyPadBtnText[col + row * 3], BUTTON_TEXTSIZE);
    }
  }

  // set the initial mic gain location
  ButtonLocation = 140;
  oButtonLocation = ButtonLocation;

  // turn on the mixers
  mixer1.gain(0, MixerGain);
  mixer1.gain(1, MixerGain);
  mixer1.gain(2, 0.0);
  mixer1.gain(3, 0.0);

  // show a cute little splash screen
  SplashScreen();

  // remind the user to set the mic input level very low
  StartupScreen();

  Display.fillScreen(BackColor);

  // draw the main screen
  DrawMainScreen();

  // update the gain (graphic and amplification, and filters)
  UpdateGain();
  ActivateFilters();

  curtime = 70000;
  pretime = 0;

}

void loop() {

  // I have everything in the fft read loop, so far no issues, but i should have some catch code outside that if

  if (fft1024_1.available()) {

    // read the batter level every minute
    if (curtime - pretime > 60000) {

      x  = 5;
      y = 8;
      w = 60;
      h = 15;
      loval = 0;
      hival = 100.0;
      pretime = curtime;
      BatVolts = 0.0;

      for (i = 0; i < 100; i++) {
        BatVolts = BatVolts + analogRead(BAT_PIN);
      }

      BatVolts = BatVolts / 100.0;
      //Serial.println(BatVolts);
      BatVolts = BatVolts * (3.3 / 1024.0);

      BatVolts = (BatVolts * (1000.0 + 4700.0)) / 1000.0;

      BatVolts  = (map(BatVolts * 10, 70, 90, 0, 1000)) / 10.0;

      if (BatVolts > 100.0) {
        BatVolts = 100.0;
      }
      if (BatVolts < 0.0) {
        BatVolts = 0.0;
      }

      // draw the border, scale, and label once
      // avoid doing this on every update to minimize flicker
      // draw the border and scale

      Display.drawRect(x, y, w, h, ForeColor);
      Display.setTextColor(ForeColor, BackColor);

      // compute level of bar graph that is scaled to the width and the hi and low vals
      // this is needed to accompdate for +/- range capability
      // draw the bar graph
      // write a upper and lower bar to minimize flicker cause by blanking out bar and redraw on update
      BatLevel = (w * (((BatVolts - loval) / (hival - loval))));

      if (BatLevel < 25.0) {
        Display.fillRect(x + BatLevel + 1, y + 1, w - BatLevel - 2, h - 2,  C_DKRED);
        Display.fillRect(x + 1, y + 1 , BatLevel - 0,  h - 2, C_RED);
      }
      else {
        Display.fillRect(x + BatLevel + 1, y + 1, w - BatLevel - 2, h - 2,  C_DKGREEN);
        Display.fillRect(x + 1, y + 1 , BatLevel - 0,  h - 2, C_GREEN);
      }
    }


    curtime = millis();

    // anyone touch anything?
    ProcessTouch();

    // process the fft spectrum
    level[0] = fft1024_1.read(0, 1) * 1.000 * MIC_BAR_GAIN;
    level[1] = fft1024_1.read(2, 3) * 1.552 * MIC_BAR_GAIN;
    level[2] = fft1024_1.read(4, 5) * 1.904 * MIC_BAR_GAIN;
    level[3] = fft1024_1.read(6, 7) * 2.178 * MIC_BAR_GAIN;
    level[4] = fft1024_1.read(8, 9) * 2.408 * MIC_BAR_GAIN;
    level[5] = fft1024_1.read(10, 12) * 2.702 * MIC_BAR_GAIN;
    level[6] = fft1024_1.read(13, 15) * 2.954 * MIC_BAR_GAIN;
    level[7] = fft1024_1.read(16, 18) * 3.178 * MIC_BAR_GAIN;
    level[8] = fft1024_1.read(19, 22) * 3.443 * MIC_BAR_GAIN;
    level[9] = fft1024_1.read(23, 26) * 3.681 * MIC_BAR_GAIN;
    level[10] = fft1024_1.read(27, 31) * 3.950 * MIC_BAR_GAIN;
    level[11] = fft1024_1.read(32, 37) * 4.239 * MIC_BAR_GAIN;
    level[12] = fft1024_1.read(38, 43) * 4.502 * MIC_BAR_GAIN;
    level[13] = fft1024_1.read(44, 50) * 4.782 * MIC_BAR_GAIN;
    level[14] = fft1024_1.read(51, 58) * 5.074 * MIC_BAR_GAIN;
    level[15] = fft1024_1.read(59, 67) * 5.376 * MIC_BAR_GAIN;
    level[16] = fft1024_1.read(68, 78) * 5.713 * MIC_BAR_GAIN;
    level[17] = fft1024_1.read(79, 90) * 6.049 * MIC_BAR_GAIN;
    level[18] = fft1024_1.read(91, 104) * 6.409 * MIC_BAR_GAIN;
    level[19] = fft1024_1.read(105, 120) * 6.787 * MIC_BAR_GAIN;
    level[20] = fft1024_1.read(121, 138) * 7.177 * MIC_BAR_GAIN;
    level[21] = fft1024_1.read(139, 159) * 7.596 * MIC_BAR_GAIN;
    level[22] = fft1024_1.read(160, 182) * 8.017 * MIC_BAR_GAIN;
    level[23] = fft1024_1.read(183, 209) * 8.473 * MIC_BAR_GAIN;
    level[24] = fft1024_1.read(210, 240) * 8.955 * MIC_BAR_GAIN;
    level[25] = fft1024_1.read(241, 275) * 9.457 * MIC_BAR_GAIN;
    level[26] = fft1024_1.read(276, 315) * 9.984 * MIC_BAR_GAIN;
    level[27] = fft1024_1.read(316, 361) * 10.544 * MIC_BAR_GAIN;
    level[28] = fft1024_1.read(362, 413) * 11.127 * MIC_BAR_GAIN;
    level[29] = fft1024_1.read(414, 473) * 11.747 * MIC_BAR_GAIN;
    level[30] = fft1024_1.read(474, 542) * 12.405 * MIC_BAR_GAIN;

    for (i = 0; i < 31; i++) {  // cycle through the 16 channels

      // scale the level to the bar height and clamp if exceed max
      line1 = level[i] * 170;
      if (line1 > 170) {
        line1 = 170;
      }
      // now it's just a matter of displaying some little boxes for each bar in a certian color
      // get number of bars
      Qty = line1 / 10;
      for (j = Qty; j < 18; j++) {
        Display.fillRect(  (i * WIDTH) + OFFSET, 210 - (j * 10), WIDTH - 1, 9, BackColor);
      }
      if (ShowHistory) {
        if (Qty > Max[i]) {
          Max[i] = Qty - 1;
        }
        if ((millis() - Showit ) > 2000) {
          Showit = millis();
          for (byte k = 0; k < 32; k++) {
            Display.fillRect((i * WIDTH) + OFFSET, 210 - (Max[k] * 10), WIDTH - 1, 9, BackColor);
            Max[k] = 0;
          }
        }
        else {
          if (Max[i] < 11) {
            Display.fillRect( (i * WIDTH) + OFFSET, 210 - (Max[i] * 10), WIDTH - 1, 9, C_GREEN);
          }
          else if (Max[i] < 16) {
            Display.fillRect( (i * WIDTH) + OFFSET, 210 - (Max[i] * 10), WIDTH - 1, 9, C_YELLOW);
          }
          else {
            Display.fillRect( (i * WIDTH) + OFFSET, 210 - (Max[i] * 10), WIDTH - 1, 9, C_RED);
          }
        }
      }
      for ( j = 0; j < Qty; j++) {
        if (j < 11) {
          Display.fillRect(  (i * WIDTH) + OFFSET, 210 - (j * 10), WIDTH - 1, 9, C_GREEN);
        }
        else if (j < 16) {
          Display.fillRect(  (i * WIDTH) + OFFSET, 210 - (j * 10), WIDTH - 1, 9, C_YELLOW);
        }
        else {
          Display.fillRect(  (i * WIDTH) + OFFSET, 210 - (j * 10), WIDTH - 1, 9, C_RED);
        }
      }
    }
  }
}

void ActivateFilters() {

  if (HighPass == 1) {
    biquad1.setHighpass(0, HighPassF, FILTER_Q);
  }
  else if (HighPass == 0) {
    biquad1.setHighpass(0, 20,  FILTER_Q);
  }

  if (LowPass == 1) {
    biquad1.setLowpass(0, LowPassF,  FILTER_Q);
  }
  else if (LowPass == 0) {
    biquad1.setLowpass(0, 20000, FILTER_Q);
  }

}

void DrawMainScreen() {

  Display.setFont(Arial_18_Bold);

  Display.setTextColor(C_DKGREY, BackColor);
  Display.setCursor(80, 7);
  Display.print(F("Mic Monitor"));

  Display.setTextColor(C_BLUE, BackColor);
  Display.setCursor(78, 5);
  Display.print(F("Mic Monitor"));

  Display.setFont(Arial_14);
  SetupBtn.drawButton();

  Display.setFont(Arial_12_Bold);
  Display.setTextColor(ForeColor, BackColor);

  Display.setCursor(2, 220);
  Display.print(F("-o")) ;
  Display.setCursor(16, 220);
  Display.print(F("o")) ;

  Display.setCursor(2, 100);
  Display.print(F("-12")) ;;

  Display.setCursor(2, 45);
  Display.print(F("+3")) ;;

  Display.drawFastHLine( 27, 220, 215, C_GREY);
  Display.drawFastHLine( 27, 109, 215, C_GREY);
  Display.drawFastHLine( 27, 49, 215, C_GREY);

  Display.setCursor(30, 225);
  Display.print(F("100")) ;;

  Display.setCursor(95, 225);
  Display.print(F("1000")) ;;
  Display.setCursor(195, 225);
  Display.print(F("10000")) ;;

  // Display.fillRoundRect (266, oButtonLocation, 45, 25, 3, BackColor);
  Display.fillRoundRect( 258, 40, 60,   190, 6, C_GREY);
  Display.drawRoundRect( 258, 40, 60,   190, 6, C_DKGREY);

  Display.setFont(Arial_12_Bold);
  Display.setTextColor(C_BLACK, BackColor);
  Display.setCursor(268, 45);
  Display.print(F("Gain")) ;

  Display.setCursor(275, 205);
  Display.print(F("L")) ;
  Display.setCursor(295, 205);
  Display.print(F("R")) ;


}


void UpdateGain() {

  MixerGain = (float) ButtonLocation * - 0.020 +  5.0;
  MixerGain =  map(ButtonLocation, 60, 170, 50, 0);
  MixerGain = (float) MixerGain / 10.0;

  if (MixerGain < 0.1) {
    MixerGain = 0.0;
  }

  Display.fillRoundRect (266, oButtonLocation, 45, 25, 3, C_GREY);
  oButtonLocation = ButtonLocation;
  Display.fillRect( 275, 60, 3,   140, C_BLACK);
  Display.fillRect( 298, 60, 3,   140, C_BLACK);

  Display.fillRoundRect (266, ButtonLocation, 45, 25, 3, C_DKBLUE);
  Display.drawRoundRect (266, ButtonLocation, 45, 25, 3, C_DKGREY);

  Display.setCursor(275, ButtonLocation + 7);
  Display.setFont(Arial_10);
  Display.setTextColor(C_WHITE, C_GREY);
  Display.print(MixerGain, 1);

  mixer1.gain(0, MixerGain);
  mixer1.gain(1, MixerGain);
  mixer1.gain(2, 0);
  mixer1.gain(3, 0);
}

void ProcessTouch() {

  if (Touch.dataAvailable()) {
    Touch.read();
    x = Touch.getX();
    y = Touch.getY();
    // depending on your display, you may have to map x y to match screen orientation
#ifdef Debug
    Serial.print(F("x: "));
    Serial.print(x);
    Serial.print(F(" - y: "));
    Serial.println(y);
#endif

    delay(10); // debounce the touch

    if (PressIt(SetupBtn, x, y) == true) {
      SetupScreen();
      return;
    }

    else if ((x > 250) & (y > 53) & (y < 180) & (y > 59)) {
      ButtonLocation = y;
      UpdateGain();
    }

    else if (( x > 10) & (x < 200)) {
      Brightness = map(y, 0, 240, 255, 5);
      analogWrite(LED_PIN, Brightness);
    }

  }

}

void SetupScreen() {

  bool KeepIn = true;

  DrawSetupScreen();

  while (KeepIn) {

    if (Touch.dataAvailable()) {

      Touch.read();

      x = Touch.getX();
      y = Touch.getY();

      if (PressIt(HistoryBtn, x, y) == true) {
        if (ShowHistory == 1) {
          ShowHistory = 0;
        }
        else if (ShowHistory == 0) {
          ShowHistory = 1;
        }
        DrawSetupScreen();
      }

      if (PressIt(MicGainBtn, x, y) == true) {
        KeyPad(MicGain, 0, 15);
        sgtl5000_1.lineInLevel(MicGain);
      }

      if (PressIt(BackColorBtn, x, y) == true) {
        if (Background == 0) {
          Background = 1;
          BackColor = C_WHITE;
          ForeColor = C_BLACK;
          DrawSetupScreen();
        }

        else {
          Background = 0;
          BackColor = C_BLACK;
          ForeColor = C_WHITE;
          DrawSetupScreen();
        }
        // save back color
      }

      if (PressIt(HighPassBtn, x, y) == true) {
        if (HighPass == 0) {
          HighPass = 1;
        }
        else if (HighPass == 1) {
          HighPass = 0;
        }
        ActivateFilters();
        DrawSetupScreen();
      }

      if (PressIt(LowPassBtn, x, y) == true) {
        if (LowPass == 0) {
          LowPass = 1;
        }
        else if (LowPass == 1) {
          LowPass = 0;
        }
        ActivateFilters();
        DrawSetupScreen();
      }

      if (PressIt(HighPassFBtn, x, y) == true) {
        KeyPad(HighPassF, 20, 400);
        ActivateFilters();
        DrawSetupScreen();
      }

      if (PressIt(LowPassFBtn, x, y) == true) {
        KeyPad(LowPassF, 400, 20000);
        ActivateFilters();
        DrawSetupScreen();
      }

      if (PressIt(DoneBtn, x, y) == true) {
        KeepIn = false;
      }
    }
  }

  // save data and exit
  EEPROM.put(10, ShowHistory);
  EEPROM.put(20, Background);
  EEPROM.put(30, MicGain);
  EEPROM.put(40, HighPass);
  EEPROM.put(50, LowPass);
  EEPROM.put(60, HighPassF);
  EEPROM.put(70, LowPassF);
  Display.fillScreen(BackColor);
  curtime = 70000;
  pretime = 0;
  DrawMainScreen();
  UpdateGain();
}


void DrawSetupScreen() {

  Display.fillScreen(BackColor);
  Display.setFont(Arial_18_Bold);
  Display.setTextColor(C_DKGREY, BackColor);
  Display.setCursor(5, 7);
  Display.print(F("Setup"));
  Display.setTextColor(C_BLUE, BackColor);
  Display.setCursor(3, 5);
  Display.print(F("Setup"));
  Display.setTextColor(ForeColor, BackColor);
  Display.setFont(Arial_12_Bold);
  HistoryBtn.drawButton();
  MicGainBtn.drawButton();
  HighPassBtn.drawButton();
  LowPassBtn.drawButton();

  if (HighPass) {
    HighPassFBtn.drawButton();
  }

  if (LowPass) {
    LowPassFBtn.drawButton();
  }

  BackColorBtn.drawButton();

  Display.setFont(Arial_14);
  DoneBtn.drawButton();

  Display.setTextColor(ForeColor, BackColor);
  Display.setFont(Arial_12);
  Display.setCursor(10 , 65);
  Display.print(F("Show bar history: ") + String(HistoryText[ShowHistory]));
  Display.setCursor(10 , 95);
  Display.print(F("Microphone gain: ") + String(MicGain));
  Display.setCursor(10 , 125);
  if (HighPass == 1) {
    Display.print(F("High Pass: ") + String(FilterText[HighPass]) + F(", ") + String(HighPassF) + F(" hz"));
  }
  else {
    Display.print(F("High Pass: ") + String(FilterText[HighPass]));
  }
  Display.setCursor(10 , 155);
  if (LowPass == 1) {
    Display.print(F("Low Pass: ") + String(FilterText[LowPass]) + F(", ") + String(LowPassF) + F(" hz"));
  }
  else {
    Display.print(F("Low Pass: ") + String(FilterText[LowPass]));
  }
  Display.setCursor(10 , 185);
  Display.print(F("Background color"));

}

void KeyPad(int & TheNumber, int MinVal, int MaxVal) {

  Display.setFont(Arial_14);

  left = BUTTON_X - (BUTTON_W);
  top = BUTTON_Y - (2 * BUTTON_H);
  wide = (3 * BUTTON_W ) + (8 * BUTTON_SPACING_X);
  high = 5 * (BUTTON_H +  BUTTON_SPACING_Y);
  TempNum = TheNumber;
  bool KeepIn = true;

  Display.fillRect(left, top, wide , high, C_DKGREY);
  Display.drawRect(left, top, wide , high, C_LTGREY);

  Display.fillRect(left + 10, top + 10, wide - 20 , 30, C_GREY);
  Display.drawRect(left + 10, top + 10, wide - 20 , 30, C_WHITE);

  Display.setCursor(left  + 110 , top + 18);
  Display.setTextColor(C_BLACK, 80);
  Display.print(F("(") + String(MinVal) + F(" - ") + String(MaxVal) + F(")"));

  Display.setCursor(left  + 20 , top + 18);
  Display.setTextColor(C_BLACK, C_GREY);
  Display.print(TheNumber);

  for (row = 0; row < 4; row++) {
    for (col = 0; col < 3; col++) {
      KeyPadBtn[col + row * 3].drawButton();
    }
  }

  while (KeepIn) {
    // get the touch point
    if (Touch.dataAvailable()) {

      Touch.read();

      x = Touch.getX();
      y = Touch.getY();

      // go thru all the KeyPadBtn, checking if they were pressed
      for (b = 0; b < 12; b++) {
        if (PressIt(KeyPadBtn[b], x, y) == true) {
          if ((b < 9) | (b == 10)) {
            TempNum *= 10;
            if (TempNum == 0) {
              Display.fillRect(left + 10, top + 10, 80 , 30, C_GREY);
            }


            if (b != 10) {
              TempNum += (b + 1);
            }
            if (TempNum > MaxVal) {
              Display.fillRect(left + 10, top + 10, 80 , 30, C_GREY);
              TempNum = TempNum / 10;
              //  TempNum -= (b + 1);
            }
          }
          // clr button! delete char
          if (b == 11) {
            Display.fillRect(left + 10, top + 10, 80 , 30, C_GREY);
            Display.drawRect(left + 10, top + 10, 80 , 30, C_WHITE);
            TempNum = 0;
          }
          if (b == 9) {
            if (TempNum >= MinVal) {
              KeepIn = false;
            }
          }
          Display.setCursor(left  + 20 , top + 18);
          Display.setTextColor(C_BLACK, C_GREY);
          Display.print(TempNum);
        }
      }
    }
  }

  TheNumber = TempNum;
  DrawSetupScreen();

}

void GetParameters() {

  byte i, test;

  EEPROM.get(0, test);
  if (test == 255) {
    // new programmer reset the whole thing
    for (i = 0; i < 255; i++) {
      EEPROM.put(i, 0);
    }
    // set some defaulTouch
    // and populate the eeprom

    ShowHistory = 0;
    EEPROM.get(10, ShowHistory);
    Background = 0;
    EEPROM.put(20, Background);
    MicGain = 5;
    EEPROM.put(30, MicGain);
    HighPass = 0;
    EEPROM.put(40, HighPass);
    LowPass = 0;
    EEPROM.put(50, LowPass);
    HighPassF = 4000;
    EEPROM.put(40, HighPassF);
    LowPassF = 40;
    EEPROM.put(50, LowPassF);

  }

  EEPROM.get(10, ShowHistory);
  EEPROM.get(20, Background);
  EEPROM.get(30, MicGain);
  EEPROM.get(40, HighPass);
  EEPROM.get(50, LowPass);
  EEPROM.get(60, HighPassF);
  EEPROM.get(70, LowPassF);
}

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


void StartupScreen() {

  unsigned long LastTime = 0;
  bool KeepIn = true;
  bool LastColor = true;

  analogWrite(LED_PIN, 255);

  Display.fillScreen(C_BLACK);
  Display.setFont(Arial_14);
  OKBtn.drawButton();
  Display.setTextColor(C_WHITE, C_BLACK);

  Display.setCursor(10, 50);
  Display.print(F("Set the CAMERA mic level to 17%"));

  Display.setFont(Arial_10_Bold);
  Display.fillRect(60, 80, 200 , 20, C_DKRED);
  Display.setCursor(65, 85);
  Display.print(F("Sound Recording"));
  Display.setFont(Arial_10);
  Display.setCursor(65, 115);
  Display.print(F("Sound Rec."));
  Display.setCursor(160, 115);
  Display.print(F("Manual"));

  Display.setCursor(65, 147);
  Display.print(F("Rec. level"));

  Display.fillRect(155, 150, 100 , 2, C_WHITE);

  Display.fillRect(155, 145, 2 , 10, C_WHITE);
  Display.fillRect(155 + 25, 145, 2 , 10, C_WHITE);
  Display.fillRect(155 + 50, 145, 2 , 10, C_WHITE);
  Display.fillRect(155 + 75, 145, 2 , 10, C_WHITE);
  Display.fillRect(155 + 100, 145, 2 , 10, C_WHITE);

  Display.fillRect(60, 175, 200 , 60, C_DKGREY);

  Display.setCursor(65, 180);
  Display.print(F("-dB 40"));
  Display.setCursor(180, 180);
  Display.print(F("12"));
  Display.setCursor(240, 180);
  Display.print(F("0"));

  Display.setCursor(65, 200);
  Display.print(F("L"));

  Display.setCursor(65, 220);
  Display.print(F("R"));

  for (int i = 80; i < 248; i += 12) {

    if (i < 188) {
      Display.fillRect(i, 200, 10 , 10, C_WHITE);
      Display.fillRect(i, 220, 10 , 10, C_WHITE);
    }
    else if (i < 212) {
      Display.fillRect(i, 200, 10 , 10, C_YELLOW);
      Display.fillRect(i, 220, 10 , 10, C_YELLOW);
    }
    else {
      Display.fillRect(i, 200, 10 , 10, C_MDGREY);
      Display.fillRect(i, 220, 10 , 10, C_MDGREY);
    }
  }

  Display.drawRect (60, 80, 200, 155, C_GREY);

  while (KeepIn) {

    if ((millis() - LastTime) > 200) {
      LastTime = millis();

      if (LastColor == true) {
        LastColor = false;
        Display.fillTriangle (155 - 4 + 16, 135, 155 + 4 + 16, 135, 155 + 16, 148, C_RED);

      }
      else {
        LastColor = true;
        Display.fillTriangle (155 - 4 + 16, 135, 155 + 4 + 16, 135, 155 + 16, 148, C_GREEN);

      }
    }

    if (Touch.dataAvailable()) {
      Touch.read();
      x = Touch.getX();
      y = Touch.getY();
      if ((PressIt(OKBtn, x, y) == true)) {
        KeepIn = false;
      }
    }
  }
}

void SplashScreen() {

  analogWrite(LED_PIN, 255);

  Display.fillScreen(C_BLACK);
  Display.fillScreenVGradient(0x000A, 0x0000);
  delay(500);
  for (int i = 0; i < 320; i += 2) {
    Display.setTextColor(C_RED, C_BLACK);
    Display.setFont(ArialBlack_40);
    Display.setCursor(11, 40);
    Display.print(F("Canon 7D"));

    Display.setTextColor(C_WHITE, C_BLACK);
    Display.setFont(Arial_20);
    Display.setCursor(30, 120);
    Display.print(F("Microphone Monitor"));

    Display.setFont(Arial_16);
    Display.setCursor(40, 180);
    Display.print(F("v 1.0 Kasprzak (c)"));
    Display.setScroll(i);
  }

  delay(2000);

}
