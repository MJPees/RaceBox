#include "../../config.h"
#if defined(ESP32_BLE)

#include "Joystick_BLE.h"

Joystick_::Joystick_() {
  _autoSend = false;
  _config = new XboxOneSControllerDeviceConfiguration();
  //_config = new XboxSeriesXControllerDeviceConfiguration();
  _hostConfig = _config->getIdealHostConfiguration();
  _gamepad = new XboxGamepadDevice(_config);
  //_gamepad = new XboxGamepadDevice(_config);
  
  _compositeHID = new BleCompositeHID((String("CH-GhostCar-SmartRace ") + String(ESP.getEfuseMac())).c_str(), "MJPees", 100);
  _compositeHID->addDevice(_gamepad);
  _compositeHID->begin(_hostConfig);
}

void Joystick_::begin(bool initAutoSendState) {
  _autoSend = initAutoSendState;
}

void Joystick_::setXAxis(int32_t value) {
  if (value > _maxSteeringDigit) value = _maxSteeringDigit;
  else if (value < _minSteeringDigit) value = _minSteeringDigit;
  int32_t x = map(value, _minSteeringDigit, _maxSteeringDigit, XBOX_STICK_MIN, XBOX_STICK_MAX);
  int32_t y = 0;
  _gamepad->setLeftThumb(x, y);
  if (_autoSend) sendState();
}

void Joystick_::setAccelerator(int32_t value) {
  if (value > _maxAcceleratorDigit) value = _maxAcceleratorDigit;
  else if (value < _minAcceleratorDigit) value = _minAcceleratorDigit;
  int32_t controlValue = map(value, _minAcceleratorDigit, _maxAcceleratorDigit, XBOX_TRIGGER_MIN, XBOX_TRIGGER_MAX);
  _gamepad->setRightTrigger(controlValue);
  if (_autoSend) sendState();
}

void Joystick_::setBrake(int32_t value) {
  if (value > _maxBrakeDigit) value = _maxBrakeDigit;
  else if (value < _minBrakeDigit) value = _minBrakeDigit;
  int32_t controlValue = map(value, _minBrakeDigit, _maxBrakeDigit, XBOX_TRIGGER_MIN, XBOX_TRIGGER_MAX);
  _gamepad->setLeftTrigger(controlValue);
  if (_autoSend) sendState();
}

void Joystick_::setButton(int num, int32_t value) {
  int32_t button = _xboxButton[num];
  if (value > 0) _gamepad->press(button);
  else _gamepad->release(button);
  if (_autoSend) sendState();
}

void Joystick_::pressButton(int32_t num) {
  int32_t button = _xboxButton[num];
  _gamepad->press(button);
  if (_autoSend) sendState();
}

void Joystick_::releaseButton(int32_t num) {
  int32_t button = _xboxButton[num];
  _gamepad->release(button);
  if (_autoSend) sendState();
}

void Joystick_::sendState() {
  if (_compositeHID->isConnected()) {
    _gamepad->sendGamepadReport();
  }
}

void Joystick_::setXAxisRange(int32_t minimum, int32_t maximum) {
  _minSteeringDigit = minimum;
  _maxSteeringDigit = maximum;
}

void Joystick_::setAcceleratorRange(int32_t minimum, int32_t maximum) {
  _minAcceleratorDigit = minimum;
  _maxAcceleratorDigit = maximum;
}

void Joystick_::setBrakeRange(int32_t minimum, int32_t maximum) {
  _minBrakeDigit = minimum;
  _maxBrakeDigit = maximum;
}

void Joystick_::end() {
  // Add any necessary cleanup code here
}

#endif //ESP32_BLE