#include "StartingLights.h" //Needs FastLED by Daniel Garcia

#define NUM_LEDS 20
#define LED_ROWS 4
#define COUNTDOWN_TIME 5000
#define SHORT_DELAY 500
#define LONG_DELAY 2000
#define FLASH_INTERVAL 1000

StartingLights startingLights(NUM_LEDS, LED_ROWS);

const int row1[] = {1};
const int row2[] = {2};
const int row3[] = {3};
const int row1_2[] = {1, 2};
const int row3_4[] = {3, 4};

void setup() {
  startingLights.begin();
}

void loop() {
  startingLights.setAllLightsOff();
  delay(SHORT_DELAY);
  startingLights.setAllLights(CRGB::Red);
  delay(SHORT_DELAY);
  startingLights.setAllLights(CRGB::Yellow);
  delay(LONG_DELAY);
  startingLights.setAllLights(CRGB::Green);
  delay(LONG_DELAY);
  startingLights.setAllLightsOff();
  delay(SHORT_DELAY);
  startingLights.setRowLights(row1, 1, CRGB::Red);
  delay(LONG_DELAY);
  startingLights.setRowLights(row2, 1, CRGB::Yellow);
  delay(LONG_DELAY);
  startingLights.setRowLights(row3, 1, CRGB::Green);
  delay(LONG_DELAY);
  startingLights.setAllLightsOff();
  delay(LONG_DELAY);
  startingLights.runCountDownLights(row3_4, 2, COUNTDOWN_TIME);
  startingLights.setRowLights(row3_4, 2, CRGB::Green);
  delay(LONG_DELAY);
  startingLights.runFlashLights(row1_2, 2, FLASH_INTERVAL, CRGB::Red, 10);
  startingLights.setAllLightsOff();
  delay(SHORT_DELAY);
}