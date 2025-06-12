#ifndef JOYSTICK_BLE_H
#define JOYSTICK_BLE_H

//https://github.com/Mystfit/ESP32-BLE-CompositeHID
#include <Arduino.h>
#include <BleConnectionStatus.h>
#include <BleCompositeHID.h>
#include <XboxGamepadDevice.h>

class Joystick_BLE_ {
  private:
    BleCompositeHID *_compositeHID;
    XboxGamepadDevice *_gamepad;
    XboxOneSControllerDeviceConfiguration *_config;
    BLEHostConfiguration _hostConfig;

    bool _autoSend = true;

    int32_t _minSteeringDigit = 0;
    int32_t _maxSteeringDigit = 0;
    int32_t _minAcceleratorDigit = 0;
    int32_t _maxAcceleratorDigit = 0;
    int32_t _minBrakeDigit = 0;
    int32_t _maxBrakeDigit = 0;

    int32_t _xboxButton[11] = {
        XBOX_BUTTON_A, 
        XBOX_BUTTON_B,
        XBOX_BUTTON_Y,
        XBOX_BUTTON_X, 
        XBOX_BUTTON_LB, 
        XBOX_BUTTON_RB, 
        XBOX_BUTTON_START,
        XBOX_BUTTON_SELECT,
        XBOX_BUTTON_HOME,   
        XBOX_BUTTON_LS, 
        XBOX_BUTTON_RS
    };

  public:
    Joystick_BLE_();
    void begin(bool initAutoSendState = false);
    void setXAxis(int32_t value);
    void setAccelerator(int32_t value);
    void setBrake(int32_t value);
    void setButton(int num, int32_t value);
    void pressButton(int32_t num);
    void releaseButton(int32_t num);
    void sendState();
    void setXAxisRange(int32_t minimum, int32_t maximum);
    void setAcceleratorRange(int32_t minimum, int32_t maximum);
    void setBrakeRange(int32_t minimum, int32_t maximum);
    void end();
};
#endif //JOYSTICK_BLE_H