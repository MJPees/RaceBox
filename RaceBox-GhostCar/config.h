#ifndef CONFIG_H
#define CONFIG_H

#define VERSION "1.1.0"

#define WIFI_DEFAULT_HOSTNAME "racebox-ghostcar"

#define KEY_TURBO 3 // Turbo-Button at gamepad
#define DEFAULT_SPEED 100 // ghostcar speed %
#define DEFAULT_BRAKING_TIME 5000 // ms
#define JOYSTICK_UPDATE_INTERVAL 100 // ms

// USB CDC on boot should be enabled in the Arduino IDE
// For ESP32C3 or ESP32S3_BLE use Tools -> Partition Scheme -> No OTA (2MB APP/2MB SPIFFS)
//#define ESP32C3 // only Bluetooth
#define ESP32S3 // Bluetooth and USB-Joystik

// Do not change the following lines!
#ifdef ESP32S3
  #define RGB_LED // optional - RGB LED is used for feedback
  #define RGB_LED_PIN 21
#elif defined(ESP32C3)
  #define LED_PIN 8 // optional - LED is used for feedback
#endif

#endif // CONFIG_H
