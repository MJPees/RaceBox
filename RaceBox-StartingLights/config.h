#ifndef CONFIG_H
#define CONFIG_H

#define VERSION "1.0.0"

#define WIFI_DEFAULT_HOSTNAME "racebox-startinglights"

#define KEY_TURBO 3 // Turbo-Button at gamepad
#define DEFAULT_SPEED 75 // ghostcar speed %
#define DEFAULT_BRAKING_TIME 5000 // ms
#define JOYSTICK_UPDATE_INTERVAL 100 // ms

// USB CDC on boot should be enabled in the Arduino IDE
// For ESP32C3 or ESP32S3_BLE use Tools -> Partition Scheme -> No OTA (2MB APP/2MB SPIFFS)
// Additional board managers (current ESP32 version) => https://espressif.github.io/arduino-esp32/package_esp32_index.json
// Include libraries from the libs folder via Arduino IDE (Zip)

#define ESP32C3 // only Bluetooth
//#define ESP32S3 // Bluetooth and USB-Joystik

// Do not change the following lines!
#ifdef ESP32S3
  #define RGB_LED // optional - RGB LED is used for feedback
  #define RGB_LED_PIN 21
#elif defined(ESP32C3)
  #define LED_PIN 8 // optional - LED is used for feedback
#endif

// define the LED type
#define LED_TYPE WS2811 // LED type, e.g. WS2811
//#define LEDTYPE WS2812B // LED type, e.g. WS2812B

// define the FastLED pin
#define FAST_LED_PIN 5 // Pin for FastLED, e.g. GPIO 5

#define LED_BRIGHTNESS 100 // LED brightness (0-255)

// define colors for the starting lights
#define RED CRGB::Green // Green and Red are swapped to match the starting lights
#define GREEN CRGB::Red
#define YELLOW CRGB::Yellow
#define BLUE CRGB::Blue
#define WHITE CRGB::White

// select the starting lights version
//#define STARTING_LIGHTS_VERSION_1_ROW
//#define STARTING_LIGHTS_VERSION_2_ROWS
//#define STARTING_LIGHTS_VERSION_3_ROWS
#define STARTING_LIGHTS_VERSION_4_ROWS

#ifdef STARTING_LIGHTS_VERSION_1_ROW
  #define NUM_LEDS 5
  #define LED_ROWS 1

  const int websocketLedRows[] = {1};
  #define WEBSOCKET_FLAHS_INTERVAL 200 // ms

  const int wifiLedRows[] =  {1};
  #define WIFI_FLAHS_INTERVAL 200 // ms

  #define CH_RACING_CLUB_LEDS_COUNTDOWN_TIME 5000
  #define CH_RACING_CLUB_LEDS_FLASH_INTERVAL 500
  const int chRacingClubDriveLedRows[] = {1};
  const int chRacingClubStopLedRows[] = {1};
  const int chRacingClubYellowLedRows[] = {1};
  const int chRacingClubCountdownLedRows[] = {1};

  #define SMARTRACE_LEDS_COUNTDOWN_TIME 5000
  #define SMARTRACE_LEDS_FLASH_INTERVAL 600
  const int smartraceDriveLedRows[] = {1};
  const int smartraceStopLedRows[] = {1};
  const int smartraceYellowLedRows[] = {1};
  const int smartraceCountdownLedRows[] = {1};
#endif // STARTING_LIGHTS_VERSION_1_ROW

#ifdef STARTING_LIGHTS_VERSION_2_ROWS
  #define NUM_LEDS 10
  #define LED_ROWS 2

  const int websocketLedRows[] = {1, 2};
  #define WEBSOCKET_FLAHS_INTERVAL 200 // ms

  const int wifiLedRows[] =  {1, 2};
  #define WIFI_FLAHS_INTERVAL 200 // ms

  #define CH_RACING_CLUB_LEDS_COUNTDOWN_TIME 5000
  #define CH_RACING_CLUB_LEDS_FLASH_INTERVAL 500
  const int chRacingClubDriveLedRows[] = {2};
  const int chRacingClubStopLedRows[] = {1};
  const int chRacingClubYellowLedRows[] = {1};
  const int chRacingClubCountdownLedRows[] = {1};

  #define SMARTRACE_LEDS_COUNTDOWN_TIME 5000
  #define SMARTRACE_LEDS_FLASH_INTERVAL 600
  const int smartraceDriveLedRows[] = {2};
  const int smartraceStopLedRows[] = {1};
  const int smartraceYellowLedRows[] = {1};
  const int smartraceCountdownLedRows[] = {1};
#endif // STARTING_LIGHTS_VERSION_2_ROW

#ifdef STARTING_LIGHTS_VERSION_3_ROWS
  #define NUM_LEDS 15
  #define LED_ROWS 3

  const int websocketLedRows[] = {1, 3};
  #define WEBSOCKET_FLAHS_INTERVAL 200 // ms

  const int wifiLedRows[] =  {1, 3};
  #define WIFI_FLAHS_INTERVAL 200 // ms

  #define CH_RACING_CLUB_LEDS_COUNTDOWN_TIME 5000
  #define CH_RACING_CLUB_LEDS_FLASH_INTERVAL 500
  const int chRacingClubDriveLedRows[] = {3};
  const int chRacingClubStopLedRows[] = {1, 2};
  const int chRacingClubYellowLedRows[] = {1};
  const int chRacingClubCountdownLedRows[] = {1, 2};

  #define SMARTRACE_LEDS_COUNTDOWN_TIME 5000
  #define SMARTRACE_LEDS_FLASH_INTERVAL 600
  const int smartraceDriveLedRows[] = {3};
  const int smartraceStopLedRows[] = {1, 2};
  const int smartraceYellowLedRows[] = {1};
  const int smartraceCountdownLedRows[] = {1, 2};
#endif // STARTING_LIGHTS_VERSION_3_ROW

#ifdef STARTING_LIGHTS_VERSION_4_ROWS
  #define NUM_LEDS 20
  #define LED_ROWS 4

  const int websocketLedRows[] = {1, 4};
  #define WEBSOCKET_FLAHS_INTERVAL 200 // ms

  const int wifiLedRows[] =  {1, 4};
  #define WIFI_FLAHS_INTERVAL 200 // ms

  #define CH_RACING_CLUB_LEDS_COUNTDOWN_TIME 5000
  #define CH_RACING_CLUB_LEDS_FLASH_INTERVAL 500
  const int chRacingClubDriveLedRows[] = {3, 4};
  const int chRacingClubStopLedRows[] = {3, 4};
  const int chRacingClubYellowLedRows[] = {1, 2};
  const int chRacingClubCountdownLedRows[] = {3, 4};

  #define SMARTRACE_LEDS_COUNTDOWN_TIME 5000
  #define SMARTRACE_LEDS_FLASH_INTERVAL 600
  const int smartraceDriveLedRows[] = {3, 4};
  const int smartraceStopLedRows[] = {3, 4};
  const int smartraceYellowLedRows[] = {1, 2};
  const int smartraceCountdownLedRows[] = {3, 4};
#endif // STARTING_LIGHTS_VERSION_4_ROWS

#endif // CONFIG_H
