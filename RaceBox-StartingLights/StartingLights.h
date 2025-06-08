#ifndef STARTINGLIGHTS_H
#define STARTINGLIGHTS_H
#include <FastLED.h>
#include <array>

#define FAST_LED_PIN 5

class StartingLights {
  public:
    StartingLights(int numLeds, int ledRows);
    ~StartingLights();
    void begin();
    void setAllLightsOff();
    void setAllLights(CRGB color);
    void setRowLights(const int* rows, int numRows, CRGB color);
    void runCountDownLights(const int* rows, int numRows, unsigned int countDownTime);
    void runFlashLights(const int* rows, int numRows, unsigned int interval, CRGB color, int maxFlashCount = -1);
    void stopRunningSequence();
    void wait(unsigned long milliseconds);
  private:
    const int numLeds;
    const int ledRows;
    const int ledsPerRow;
    volatile bool sequenceIsRunning;
    CRGB* leds;
};

#endif
