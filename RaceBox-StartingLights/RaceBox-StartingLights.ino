#include "StartingLights.h" //Needs FastLED by Daniel Garcia

#define NUM_LEDS 15
#define LED_ROWS 3
#define COUNTDOWN_TIME 5000

StartingLights startingLights(NUM_LEDS, LED_ROWS);

void setup() {
  startingLights.begin();
}

void loop() {
  startingLights.setAllLights(CRGB::Red);
  delay(2000);
  startingLights.setAllLightsOff();
  delay(500);
  startingLights.setAllLights(CRGB::Yellow);
  delay(2000);
  startingLights.setAllLights(CRGB::Green);
  delay(2000);
  startingLights.setRowLights(1, CRGB::Red);
  delay(2000);
  startingLights.setRowLights(3, CRGB::Yellow);
  delay(2000);
  startingLights.setRowLights(2,CRGB::Green);
  delay(2000);
  startingLights.setRowLights(3, CRGB::Black);
  delay(2000);
  startingLights.setAllLights(CRGB::Red);
  delay(2000);
  startingLights.countDownLights(COUNTDOWN_TIME);
  delay(1000);
  startingLights.setRowLights(3, CRGB::Green);
  startingLights.setAllLightsOff();
  delay(500);
}