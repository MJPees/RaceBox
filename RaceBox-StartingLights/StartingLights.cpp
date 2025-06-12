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

void StartingLights::runCountDownLights(const int* rows, int numRows, unsigned int countDownTime, CRGB color) {
    setAllLightsOff();
    sequenceIsRunning = true;
    mode = MODE_COUNTDOWN;
    countdownNumRows = numRows > 8 ? 8 : numRows;
    memcpy(countdownRows, rows, countdownNumRows * sizeof(int));
    countdownTime = countDownTime;
    countdownColor = color;
    countdownStep = 0;
    countdownLastMillis = millis();
}

void StartingLights::runFlashLights(const int* rows, int numRows, unsigned int interval, CRGB color, int maxFlashCount) {
    setAllLightsOff();
    sequenceIsRunning = true;
    mode = MODE_FLASH;
    flashNumRows = numRows > 8 ? 8 : numRows;
    memcpy(flashRows, rows, flashNumRows * sizeof(int));
    flashInterval = interval;
    flashColor = color;
    flashMaxCount = maxFlashCount;
    flashCount = 0;
    flashLedIsOn = true;
    flashLastMillis = millis();
}

void StartingLights::stopRunningSequence() {
    sequenceIsRunning = false;
    mode = MODE_NONE;
    setAllLightsOff();
}

void StartingLights::updateLeds() {
    if (!sequenceIsRunning) return;

    unsigned long now = millis();

    if (mode == MODE_COUNTDOWN) {
        if (now - countdownLastMillis >= countdownTime / ledsPerRow) {
          if (countdownStep >= ledsPerRow) {
              //setAllLightsOff();
              sequenceIsRunning = false;
              mode = MODE_NONE;
              return;
          }
          for (int r = 0; r < countdownNumRows; r++) {
              int row = countdownRows[r] - 1;
              if (row >= 0 && row < ledRows) {
                  leds[countdownStep + row * ledsPerRow] = countdownColor;
              }
          }
          FastLED.show();
          countdownStep++;
          countdownLastMillis = now;
      }
    } else if (mode == MODE_FLASH) {
        if (flashMaxCount > 0 && flashCount >= flashMaxCount) {
            setAllLightsOff();
            sequenceIsRunning = false;
            mode = MODE_NONE;
            return;
        }
        if (now - flashLastMillis >= flashInterval) {
            for (int r = 0; r < flashNumRows; r++) {
                int row = flashRows[r] - 1;
                if (row >= 0 && row < ledRows) {
                    for (int i = 0; i < ledsPerRow; i++) {
                        leds[i + row * ledsPerRow] = flashLedIsOn ? flashColor : CRGB::Black;
                    }
                }
            }
            FastLED.show();
            if (flashLedIsOn) flashCount++;
            flashLedIsOn = !flashLedIsOn;
            flashLastMillis = now;
        }
    }
}
