#include "StartingLights.h"
#include <FastLED.h>
#include <array>

StartingLights::StartingLights(const int numLeds, int ledRows)
  : numLeds(numLeds), ledRows(ledRows), ledsPerRow(numLeds / ledRows) {
  leds = new CRGB[numLeds];
  sequenceIsRunning = false;
}

StartingLights::~StartingLights() {
  delete[] leds;
}

void StartingLights::begin() {
  FastLED.addLeds<WS2811, FAST_LED_PIN, GRB>(leds, numLeds);
  setAllLightsOff();
}

void StartingLights::setAllLightsOff() {
  for (int i = 0; i < numLeds; i++) {
    leds[i] = CRGB::Black;
  }
  FastLED.show();
}

void StartingLights::setAllLights(CRGB color) {
  for (int i = 0; i < numLeds; i++) {
    leds[i] = color;
  }
  FastLED.show();
}

void StartingLights::setRowLights(const int* rows, int numRows, CRGB color) {
  for (int r = 0; r < numRows; r++) {
    int row = rows[r] - 1;
    if (row >= 0 && row < ledRows) {
      int start = row * ledsPerRow;
      int end = start + ledsPerRow;
      for (int i = start; i < end; i++) {
        leds[i] = color;
      }
    }
  }
  FastLED.show();
}

void StartingLights::runCountDownLights(const int* rows, int numRows, unsigned int countDownTime) {
  setAllLightsOff();
  sequenceIsRunning = true;
  for(int i = 0; i < ledsPerRow; i++) {
    if(!sequenceIsRunning) {
      setAllLightsOff();
      return;
    }
    for(int r = 0; r < numRows; r++) {
      int row = rows[r] - 1;
      if(row >= 0 && row < ledRows) {
        leds[i + row * ledsPerRow] = CRGB::Red;
      }
    }
    FastLED.show();
    wait(countDownTime / ledsPerRow);
  }
  setAllLightsOff();
  sequenceIsRunning = false;
}

void StartingLights::runFlashLights(const int* rows, int numRows, unsigned int interval, CRGB color, int maxFlashCount) {
  setAllLightsOff();
  sequenceIsRunning = true;
  bool ledIsOn = true;
  int flashCount = 0;
  while(sequenceIsRunning && (maxFlashCount <= 0 || flashCount < maxFlashCount)) {
    for(int r = 0; r < numRows; r++) {
      int row = rows[r] - 1;
      if(row >= 0 && row < ledRows) {
        for(int i = 0; i < ledsPerRow; i++) {
          leds[i + row * ledsPerRow] = ledIsOn ? color : CRGB::Black;
        }
      }
    }
    FastLED.show();
    if(ledIsOn) flashCount++;
    ledIsOn = !ledIsOn;
    wait(interval);
  }
  setAllLightsOff();
  sequenceIsRunning = false;
}

void StartingLights::stopRunningSequence() {
  sequenceIsRunning = false;
}

void StartingLights::wait(unsigned long milliseconds) {
  unsigned long start = millis();
  while (millis() - start < milliseconds) {
    delay(1);
  }
}
