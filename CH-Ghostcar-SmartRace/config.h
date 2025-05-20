#ifndef CONFIG_H
#define CONFIG_H

#define VERSION "1.0.0"

// Turbo-Button at gamepad
#define KEY_TURBO 3

//For ESP32C3 or ESP32S3_BLE use Tools -> Partition Scheme -> No OTA (2MB APP/2MB SPIFFS)
#define ESP32C3 // only Bluetooth
//#define ESP32S3 // USB-Controller
//#define ESP32S3_BLE // only Bluetooth

// Do not change the following lines!
#ifdef ESP32S3
  #define RGB_LED // optional - RGB LED is used for feedback
  #define RGB_LED_PIN 21
#elif defined(ESP32S3_BLE)
  #define ESP32_BLE //For ESP32_BLE use Tools -> Partition Scheme -> No OTA (2MB APP/2MB SPIFFS)
  #define RGB_LED // optional - RGB LED is used for feedback
  #define RGB_LED_PIN 21
#elif defined(ESP32C3)
  #define ESP32_BLE //For ESP32_BLE use Tools -> Partition Scheme -> No OTA (2MB APP/2MB SPIFFS)
  #define LED_PIN 5 // optional - LED is used for feedback
#endif

#endif // CONFIG_H