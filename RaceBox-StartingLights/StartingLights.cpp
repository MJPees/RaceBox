#include "StartingLights.h"
#include <FastLED.h>

StartingLights::StartingLights(const int numLeds, int ledRows)
  : numLeds(numLeds), ledRows(ledRows) {
  leds = new CRGB[numLeds];
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

void StartingLights::setRowLights(int row, CRGB color) {
  row -= 1; // Adjust for zero-based index
  if (row < 0 || row >= ledRows) return;
  int start = row * (numLeds / ledRows);
  int end = start + (numLeds / ledRows);
  
  for (int i = start; i < end; i++) {
    leds[i] = color;
  }
  FastLED.show();
}

void StartingLights::countDownLights(unsigned long countDownTime) {
  for (int i = 0; i < (numLeds / ledRows); i++) {
    wait(countDownTime / (numLeds / ledRows));
    for(int j = 0; j < ledRows; j++) {
      leds[i + j * (numLeds / ledRows)] = CRGB::Black;
    }
    FastLED.show();
  }
}

void StartingLights::wait(unsigned long milliseconds) {
  unsigned long start = millis();
  while (millis() - start < milliseconds) {
    delay(1);
  }
}
