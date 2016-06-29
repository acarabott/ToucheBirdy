#include "Button.h"


//****************************************************************************************
// Illutron take on Disney style capacitive touch sensor using only passives and Arduino
// Dzl 2012
//****************************************************************************************


//                              10n
// PIN 9 --[10k]-+-----10mH---+--||-- OBJECT
//               |            |
//              3.3k          |
//               |            V 1N4148 diode
//              GND           |
//                            |
//Analog 0 ---+------+--------+
//            |      |
//          100pf   1MOmhm
//            |      |
//           GND    GND

#define SET(x,y) (x |=(1<<y))       //-Bit set/clear macros
#define CLR(x,y) (x &= (~(1<<y)))           // |
#define CHK(x,y) (x & (1<<y))               // |
#define TOG(x,y) (x^=(1<<y))                //-+

#define N 200  //How many frequencies
const float maxDist = 163840; // 160 * 1024

float results[N];                 //-Filtered result buffer

#define RESTING 0
#define FOOD 1
#define GRAB 2
#define NUM_GESTURES 3

Button buttons[NUM_GESTURES] = { Button(10), Button(11), Button(12) };
float gesturePoints[NUM_GESTURES][2] = {{0.0, 0.0}, {0.0, 0.0}};

uint8_t currentGesture = RESTING;

#define TONE_PIN 7

void setup()
{
  TCCR1A = 0b10000010;            //-Set up frequency generator
  TCCR1B = 0b00011001;            //-+
  ICR1 = 110;
  OCR1A = 55;

  pinMode(9,OUTPUT);              //-Signal generator pin
  pinMode(8,OUTPUT);              //-Sync (test) pin



  Serial.begin(115200);

  for(int i = 0; i < N; i++) {    //-Preset results
    results[i] = 0;               //-+
  }
}

float getDistance(float x1, float y1, float x2, float y2) {
  return abs(x1 - x2) + abs(y1 - y2);
}

uint8_t getCurrentBeat(uint32_t time, uint8_t numBeats, uint16_t beatDur) {
  uint16_t totalDur = numBeats * beatDur;
  return (time % totalDur) / beatDur;
}

void foodResponse() {
  const uint8_t currentBeat = getCurrentBeat(millis(), 24, 62);

  if(currentBeat == 0 || currentBeat == 4){
    tone(TONE_PIN, random(2114, 2162), 100);
  } else if(currentBeat == 1 || currentBeat == 5){
    tone(TONE_PIN, random(2269, 2319), 100);
  } else {
    noTone(TONE_PIN);
  }
}

void grabResponse() {
  const uint8_t currentBeat = getCurrentBeat(millis(), 6, 130);

  if(currentBeat % 2 == 0) {
    tone(TONE_PIN, random(2500, 3000), 100);
  } else {
    noTone(TONE_PIN);
  }
}

void loop()
{
  // read data
  unsigned int maxFreq = 0;
  float maxResult = 0;
  for(unsigned int i = 0; i < N; i++)
  {
    const int v = analogRead(0);  //-Read response signal
    CLR(TCCR1B, 0);               //-Stop generator
    TCNT1 = 0;                    //-Reload new frequency
    ICR1 = i;                     // |
    OCR1A = i / 2;                //-+
    SET(TCCR1B, 0);               //-Restart generator

    results[i] = results[i] * 0.5 + (float)(v) * 0.5;   //Filter results

    if(results[i] > maxResult) {
      maxFreq = i;
      maxResult = results[i];
    }
  }

  // gesture recognition
  float closestGestureDistance = maxDist;
  for(unsigned int i = 0; i < NUM_GESTURES; i++) {
    // update gesture
    buttons[i].update();
    if(buttons[i].getState() == HIGH) {
      gesturePoints[i][0] = maxFreq;
      gesturePoints[i][1] = maxResult;
    }

    // calculate closest gesture
    const float dist = getDistance(maxFreq, maxResult,
                                   gesturePoints[i][0], gesturePoints[i][1]);

    if(dist < closestGestureDistance){
      closestGestureDistance = dist;
      currentGesture = i;
    }
  }

  // response
  switch (currentGesture) {
    case RESTING: noTone(TONE_PIN);   break;
    case FOOD:    foodResponse();     break;
    case GRAB:    grabResponse();     break;
    default:      noTone(TONE_PIN);   break;
  }

  TOG(PORTB, 0);            //-Toggle pin 8 after each sweep (good for scope)
}
