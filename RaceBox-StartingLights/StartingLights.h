#ifndef STARTINGLIGHTS_H
#define STARTINGLIGHTS_H
#include <FastLED.h>

#define FAST_LED_PIN 5

class StartingLights {
  public:
    StartingLights(int numLeds, int ledRows);
    ~StartingLights();
    void begin();
    void setAllLightsOff();
    void setAllLights(CRGB color);
    void setRowLights(int row, CRGB color);
    void countDownLights(unsigned long countDownTime);
  private:
    const int numLeds;
    const int ledRows;
    CRGB* leds;
    void wait(unsigned long milliseconds);
};

#endif
