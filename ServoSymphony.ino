#define TEMPO 1000000
#define R1 1
#define R2 2
#define R4 4
#define R8 8
#define R16 16
#define R32 32

#define N_A0 27.5
#define N_B0 30.87
#define N_C1 32.71
#define N_D1 36.71
#define N_E1 41.2
#define N_F1 43.65
#define N_G1 49
#define N_A1 55
#define N_B1 61.74

#define N_C2 65.41
#define N_D2 73.42
#define N_E2 82.41
#define N_F2 87.31
#define N_G2 98
#define N_A2 100
#define N_B2 123.47

#define N_C3 130.81
#define N_D3 146.83
#define N_E3 164.81
#define N_F3 174.61
#define N_G3 196
#define N_A3 220
#define N_B3F 233.08
#define N_B3 246.94

#define N_C4 261.63
#define N_D4F 277.18
#define N_D4 293.67
#define N_E4 329.63
#define N_F4 349.23
#define N_G4 392
#define N_A4 440
#define N_B4 493.88

#define N_C5 523.25
#define N_D5 587.33
#define N_D5S 622.25
#define N_E5 659.26
#define N_F5 698.46
#define N_F5S 740
#define N_G5 783.99
#define N_G5S 831.61
#define N_A5 880
#define N_B5 987.77

#define N_C6 1046.5
#define N_D6 1174.7
#define N_D6S  1244.5
#define N_E6 1318.5
#define N_F6 1396.9
#define N_F6S 1480
#define N_G6 1568
#define N_G6S 1661.2
#define N_A6 1760
#define N_B6 1975.5

#define N_C7 2093
#define N_D7 2349.3
#define N_E7 2637
#define N_F7 2793
#define N_G7 3136
#define N_A7 3520
#define N_B7 3951.1

#define N_C8 2093

#define XMOTOR_DIR    0
#define XMOTOR_STEP   1
#define XMOTOR_EN     5

int i;
unsigned long TheStart;

void setup() {

  Serial.begin(9600);
  while (!Serial) {}
  pinMode(XMOTOR_EN,    OUTPUT);
  pinMode(XMOTOR_DIR,    OUTPUT);
  pinMode(XMOTOR_STEP,   OUTPUT);

  pinMode(2,    OUTPUT);
  pinMode(3,    OUTPUT);
  pinMode(4,   OUTPUT);


  digitalWrite(XMOTOR_DIR,    LOW);
  digitalWrite(XMOTOR_STEP,   HIGH);
  digitalWrite(XMOTOR_EN,    HIGH);

  digitalWrite(2,    LOW);
  digitalWrite(3,   LOW);
  digitalWrite(4,    LOW);


}

void loop() {

  ThunderStruck();
  // SmokeOnTheWater();
}



void SlideNote(long FirstNote, long SecondNote, unsigned long Delay) {

  digitalWrite(XMOTOR_EN,    LOW);
  digitalWrite(XMOTOR_DIR, LOW);
  unsigned int  Steps;

  if (FirstNote < SecondNote) {
    Steps = SecondNote - FirstNote;
    for (i = FirstNote; i < SecondNote; i++) {
      TheStart = micros();
      while ((micros() - TheStart) < (TEMPO / (Steps * Delay))) {
        digitalWrite(XMOTOR_STEP, HIGH);
        delayMicroseconds(1000000.0 / i);
        digitalWrite(XMOTOR_STEP, LOW);
        delayMicroseconds(1);
      }
    }
  }
  else {
    Steps = FirstNote - SecondNote;
    for (i = FirstNote; i >  SecondNote; i--) {
      TheStart = micros();
      while ((micros() - TheStart) < (TEMPO / (Steps * Delay))) {
        digitalWrite(XMOTOR_STEP, HIGH);
        delayMicroseconds(1000000.0 / i);
        digitalWrite(XMOTOR_STEP, LOW);
        delayMicroseconds(1);
      }
    }
  }
  digitalWrite(XMOTOR_EN,    HIGH);

}


void Rest(unsigned int val) {

  delayMicroseconds(TEMPO / val);
}

void PlayNote(long Note, unsigned long Delay) {

  digitalWrite(XMOTOR_EN,    LOW);
  digitalWrite(XMOTOR_DIR, LOW);
  TheStart = micros();

  while ((micros() - TheStart) <= (TEMPO / Delay)) {
    digitalWrite(XMOTOR_STEP, HIGH);
    delayMicroseconds(1000000.0 / Note);
    digitalWrite(XMOTOR_STEP, LOW);
    delayMicroseconds(1);

  }


  digitalWrite(XMOTOR_EN,    HIGH);

}

void Scale() {

  PlayNote(N_C3, 100);
  PlayNote(N_D3, 100);
  PlayNote(N_E3, 100);
  PlayNote(N_F3, 100);
  PlayNote(N_G3, 100);
  PlayNote(N_A3, 100);
  PlayNote(N_B3, 100);

  PlayNote(N_C4, 100);
  PlayNote(N_D4, 100);
  PlayNote(N_E4, 100);
  PlayNote(N_F4, 100);
  PlayNote(N_G4, 100);
  PlayNote(N_A4, 100);
  PlayNote(N_B4, 100);

  PlayNote(N_C5, 100);
  PlayNote(N_D5, 100);
  PlayNote(N_E5, 100);
  PlayNote(N_F5, 100);
  PlayNote(N_G5, 100);
  PlayNote(N_A5, 100);
  PlayNote(N_B5, 100);

  PlayNote(N_C6, 100);
  PlayNote(N_D6, 100);
  PlayNote(N_E6, 100);
  PlayNote(N_F6, 100);
  PlayNote(N_G6, 100);
  PlayNote(N_A6, 100);
  PlayNote(N_B6, 100);

}



void ThunderStruck() {


  PlayNote(N_B5, R8);
  for (i = 0; i < 8; i++) {
    PlayNote(N_D6S, R8);
    PlayNote(N_B5, R8);
    PlayNote(N_F6S, R8);
    PlayNote(N_B5, R8);
  }
  for (i = 0; i < 8; i++) {
    PlayNote(N_E6, R8);
    PlayNote(N_B5, R8);
    PlayNote(N_G6, R8);
    PlayNote(N_B5, R8);
  }
  for (i = 0; i < 8; i++) {
    PlayNote(N_D6S, R8);
    PlayNote(N_B5, R8);
    PlayNote(N_F6S, R8);
    PlayNote(N_B5, R8);
  }
  for (i = 0; i < 8; i++) {
    PlayNote(N_E6, R8);
    PlayNote(N_B5, R8);
    PlayNote(N_G6, R8);
    PlayNote(N_B5, R8);
  }

  //0000000000000000000000000000000000000000
  for (i = 0; i < 8; i++) {
    //1
    PlayNote(N_B6, R8);
    PlayNote(N_B5, R8);
    PlayNote(N_A6, R8);
    PlayNote(N_B5, R8);
    //2
    PlayNote(N_G6S, R8);
    PlayNote(N_B5, R8);
    PlayNote(N_A6, R8);
    PlayNote(N_B5, R8);
    //3
    PlayNote(N_G6S, R8);
    PlayNote(N_B5, R8);
    PlayNote(N_F6S, R8);
    PlayNote(N_B5, R8);
    // 4
    PlayNote(N_G6S, R8);
    PlayNote(N_B5, R8);
    PlayNote(N_E6, R8);
    PlayNote(N_B5, R8);
    // 5
    PlayNote(N_F6S, R8);
    PlayNote(N_B5, R8);
    PlayNote(N_D6S, R8);
    PlayNote(N_B5, R8);
    // 6
    PlayNote(N_E6, R8);
    PlayNote(N_B5, R8);
    PlayNote(N_D6S, R8);
    PlayNote(N_B5, R8);
    // 7
    PlayNote(N_E6, R8);
    PlayNote(N_B5, R8);
    PlayNote(N_D6S, R8);
    PlayNote(N_B5, R8);
    // 8
    PlayNote(N_E6, R8);
    PlayNote(N_B5, R8);
    PlayNote(N_D6S, R8);
    PlayNote(N_B5, R8);
  }
}
void SmokeOnTheWater() {

  PlayNote(N_G3, R4);
  PlayNote(N_B3F, R4);
  PlayNote(N_C4, R2);

  PlayNote(N_G3, R4);
  PlayNote(N_B3F, R4);
  PlayNote(N_D4F, R8);
  PlayNote(N_C4, R4);
  PlayNote(N_C4, R8);

  PlayNote(N_G3, R4);
  PlayNote(N_B3F, R4);
  PlayNote(N_C4, R2);
  PlayNote(N_B3F, R4);
  Rest(R32);
  PlayNote(N_G3, R8);
  Rest(R32);
  PlayNote(N_G3, R8);
  SlideNote(N_G3, N_E3, R2);

  Rest(R2);

  PlayNote(N_G3, R4);
  PlayNote(N_B3F, R4);
  PlayNote(N_C4, R2);

  PlayNote(N_G3, R4);
  PlayNote(N_B3F, R4);
  PlayNote(N_D4F, R8);
  PlayNote(N_C4, R4);
  PlayNote(N_C4, R8);

  PlayNote(N_G3, R4);
  PlayNote(N_B3F, R4);
  PlayNote(N_C4, R2);
  PlayNote(N_B3F, R4);
  Rest(R32);
  PlayNote(N_G3, R8);
  Rest(R32);
  PlayNote(N_G3, R8);
  SlideNote(N_G3, N_E3, R4);

  Rest(R4);

  PlayNote(N_G3, R4);
  PlayNote(N_B3F, R4);
  Rest(R32);
  PlayNote(N_B3F, R4);
  Rest(R32);
  PlayNote(N_G3, R4);
  PlayNote(N_B3F, R4);
  Rest(R32);
  PlayNote(N_B3F, R4);
  SlideNote(N_B3F, N_C4, R4);

  Rest(R4);

  PlayNote(N_G3, R4);
  PlayNote(N_B3F, R4);
  Rest(R32);
  PlayNote(N_B3F, R4);
  PlayNote(N_G3, R4);
  PlayNote(N_B3F, R4);

  SlideNote(N_B3F, N_G3, R4);
  Rest(R4);

  PlayNote(N_G3, R4);
  PlayNote(N_B3F, R4);
  PlayNote(N_C4, R2);

  PlayNote(N_G3, R4);
  PlayNote(N_B3F, R4);
  PlayNote(N_D4F, R8);
  PlayNote(N_C4, R4);
  PlayNote(N_C4, R8);

  PlayNote(N_G3, R4);
  PlayNote(N_B3F, R4);
  PlayNote(N_C4, R2);
  PlayNote(N_B3F, R4);
  Rest(R32);
  PlayNote(N_G3, R8);
  Rest(R32);
  PlayNote(N_G3, R8);
  SlideNote(N_G3, N_E3, R2);


}
