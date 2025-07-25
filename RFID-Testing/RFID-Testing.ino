#define VERSION "1.0.2"
/* configuration */
#define DEFAULT_POWER_LEVEL 26 // default power level
#define RFID_LED_ON_TIME 200
#define RFID_REPEAT_TIME 3000
#define RFID_RESTART_TIME 300000
#define DENSE_SWITCH_PIN 0

//#define DEBUG
#define PRINT_RSSI
#define ESP32C3
//#define ESP32DEV
#ifdef ESP32DEV
  #define SerialRFID Serial2
  #define RX_PIN 16
  #define TX_PIN 17
  #define RFID_LED_PIN 2
#elif defined(ESP32C3)
  HardwareSerial SerialRFID(1);
  #define RX_PIN 5
  #define TX_PIN 6
  #define RFID_LED_PIN 8
#endif

const unsigned char ReadMulti[10] = {0XAA,0X00,0X27,0X00,0X03,0X22,0XFF,0XFF,0X4A,0XDD};
const unsigned char StopReadMultiResponse[8] = {0xAA,0x01,0x28,0x00,0x01,0x00,0x2A,0xDD};
const unsigned char StopReadMulti[7] = {0XAA,0X00,0X28,0X00,0X00,0X28,0XDD};
const unsigned char Power10dbm[9] = {0XAA,0X00,0XB6,0X00,0X02,0X03,0XE8,0XA3,0XDD};
const unsigned char Power11dbm[9] = {0XAA,0X00,0XB6,0X00,0X02,0X04,0X4C,0X08,0XDD};
const unsigned char Power12dbm[9] = {0XAA,0X00,0XB6,0X00,0X02,0X04,0XB0,0X6C,0XDD};
const unsigned char Power13dbm[9] = {0XAA,0X00,0XB6,0X00,0X02,0X05,0X14,0XD1,0XDD};
const unsigned char Power14dbm[9] = {0XAA,0X00,0XB6,0X00,0X02,0X05,0X78,0X35,0XDD};
const unsigned char Power15dbm[9] = {0XAA,0X00,0XB6,0X00,0X02,0X05,0XDC,0X99,0XDD};
const unsigned char Power16dbm[9] = {0XAA,0X00,0XB6,0X00,0X02,0X06,0X40,0XFE,0XDD};
const unsigned char Power17dbm[9] = {0XAA,0X00,0XB6,0X00,0X02,0X06,0XA4,0X62,0XDD};
const unsigned char Power18dbm[9] = {0XAA,0X00,0XB6,0X00,0X02,0X07,0X08,0XC7,0XDD};
const unsigned char Power19dbm[9] = {0XAA,0X00,0XB6,0X00,0X02,0X07,0X6C,0X2B,0XDD};
const unsigned char Power20dbm[9] = {0XAA,0X00,0XB6,0X00,0X02,0X07,0XD0,0X8F,0XDD};
const unsigned char Power21dbm[9] = {0XAA,0X00,0XB6,0X00,0X02,0X08,0X34,0XF4,0XDD};
const unsigned char Power22dbm[9] = {0XAA,0X00,0XB6,0X00,0X02,0X08,0X98,0X58,0XDD};
const unsigned char Power23dbm[9] = {0XAA,0X00,0XB6,0X00,0X02,0X08,0XFC,0XBC,0XDD};
const unsigned char Power24dbm[9] = {0XAA,0X00,0XB6,0X00,0X02,0X09,0X60,0X21,0XDD};
const unsigned char Power25dbm[9] = {0XAA,0X00,0XB6,0X00,0X02,0X09,0XC4,0X85,0XDD};
const unsigned char Power26dbm[9] = {0XAA,0X00,0XB6,0X00,0X02,0X0A,0X28,0XEA,0XDD};
const unsigned char PowerLevelResponse[] = {0xAA,0x01,0xB6,0x00,0x01,0x00,0xB8,0xDD};
const unsigned char Europe[8] = {0XAA,0X00,0X07,0X00,0X01,0X03,0X0B,0XDD};
const unsigned char RegionResponse[] = {0xAA,0x01,0x07,0x00,0x01,0x00,0x09,0xDD};
const unsigned char HighSensitivity[8] = {0XAA,0X00,0XF5,0X00,0X01,0X00,0XF6,0XDD};
const unsigned char HighSensitivityResponse[8] = {0XAA,0X01,0XF5,0X00,0X01,0X00,0XF7,0XDD};
const unsigned char DenseReader[8] = {0XAA,0X00,0XF5,0X00,0X01,0X01,0XF7,0XDD};
const unsigned char DenseReaderResponse[8] = {0XAA,0X01,0XF5,0X00,0X01,0X00,0XF7,0XDD};
const unsigned char NoModuleSleepTime[8] = {0XAA,0X00,0X1D,0x00,0x01,0x00,0x1E,0xDD};
const unsigned char NoModuleSleepTimeResponse[] = {0XAA,0X01,0X1D,0x00,0x01,0x00,0x1F,0xDD};

unsigned int rfidSerialByte = 0;
bool startByte = false;
bool gotMessageType = false;
unsigned char messageType = 0;
unsigned char command = 0;
unsigned int rssi = 0;
unsigned int pc = 0;
unsigned int parameterLength = 0;
unsigned int crc = 0;
unsigned int dataCheckSum = 0;
int lastDenseSwitchValue = 0;

unsigned char epcBytes[12] = {0,0,0,0,0,0,0,0,0,0,0,0};
String lastEpcString = "";
unsigned long lastEpcRead = 0;
unsigned long lastRestart = 0;
unsigned long rfidLedOnTimeMs = 0;
int powerLevel = DEFAULT_POWER_LEVEL;

void wait(unsigned long waitTime) {
  unsigned long startWaitTime = millis();
  while((millis() - startWaitTime) < waitTime) {
    delay(1);
  }
}

bool checkResponse(const unsigned char expectedBuffer[], int length) {
  bool ok = true;
  unsigned char buffer[length];
  SerialRFID.readBytes(buffer, length);
  #ifdef DEBUG
    Serial.println("\nResponse:\n");
  #endif
  for (int i = 0; i < length; ++i) {
    #ifdef DEBUG
      Serial.print(" 0x");
      Serial.print(buffer[i], HEX);
    #endif
    if (buffer[i] != expectedBuffer[i]) {
      ok = false;
    }
  }
  #ifdef DEBUG
    Serial.println("\nExpected Response:\n");
    for (int i = 0; i < length; ++i) {
        Serial.print(" 0x");
        Serial.print(expectedBuffer[i], HEX);
    }
    Serial.println("");
  #endif
  return ok;
}

bool setReaderSetting(const unsigned char sendBuffer[], int sendLength, const unsigned char expectedResponseBuffer[], int expectedLength) {
  bool ok = false;
  int retries = 0;
  while (!ok && retries < 3) {
    while(SerialRFID.available()) {
      SerialRFID.read();
    }
    SerialRFID.write(sendBuffer, sendLength);
    ok = checkResponse(expectedResponseBuffer, expectedLength);
    retries++;
  }
  return ok;
}

void rfidSetDensitivityMode(bool dense_mode) {
  // Set reader mode based on loaded configuration
  bool ok;

  if(dense_mode) {
    ok = setReaderSetting(DenseReader, 8, DenseReaderResponse, 8);
    if(ok) {
      Serial.println("RFID: set dense reader mode.");
      ledOn(RFID_LED_PIN);
      wait(200);
      ledOff(RFID_LED_PIN);
      wait(100);
      ledOn(RFID_LED_PIN);
      wait(200);
      ledOff(RFID_LED_PIN);
      wait(200);
    } else {
      Serial.println("RFID: failed to set dense reader mode.");
    }
  } else {
    ok = setReaderSetting(HighSensitivity, 8, HighSensitivityResponse, 8);
    if(ok) {
      Serial.println("RFID: set high sensitivity reader mode.");
      ledOn(RFID_LED_PIN);
      wait(200);
      ledOff(RFID_LED_PIN);
      wait(100);
      ledOn(RFID_LED_PIN);
      wait(200);
      ledOff(RFID_LED_PIN);
      wait(100);
      ledOn(RFID_LED_PIN);
      wait(200);
      ledOff(RFID_LED_PIN);
      wait(200);
    } else {
      Serial.println("RFID: failed to set high sensitivity reader mode.");
    }
  }
}

void setPowerLevel(int powerLevel) {
  // Set power level based on loaded configuration
  bool ok;

  switch (powerLevel) {
    case 10:
      ok = setReaderSetting(Power10dbm, 9, PowerLevelResponse, 8);
      break;
    case 11:
      ok = setReaderSetting(Power11dbm, 9, PowerLevelResponse, 8);
      break;
    case 12:
      ok = setReaderSetting(Power12dbm, 9, PowerLevelResponse, 8);
      break;
    case 13:
      ok = setReaderSetting(Power13dbm, 9, PowerLevelResponse, 8);
      break;
    case 14:
      ok = setReaderSetting(Power14dbm, 9, PowerLevelResponse, 8);
      break;
    case 15:
      ok = setReaderSetting(Power15dbm, 9, PowerLevelResponse, 8);
      break;
    case 16:
      ok = setReaderSetting(Power16dbm, 9, PowerLevelResponse, 8);
      break;
    case 17:
      ok = setReaderSetting(Power17dbm, 9, PowerLevelResponse, 8);
      break;
    case 18:
      ok = setReaderSetting(Power18dbm, 9, PowerLevelResponse, 8);
      break;
    case 19:
      ok = setReaderSetting(Power19dbm, 9, PowerLevelResponse, 8);
      break;
    case 20:
      ok = setReaderSetting(Power20dbm, 9, PowerLevelResponse, 8);
      break;
    case 21:
      ok = setReaderSetting(Power21dbm, 9, PowerLevelResponse, 8);
      break;
    case 22:
      ok = setReaderSetting(Power22dbm, 9, PowerLevelResponse, 8);
      break;
    case 23:
      ok = setReaderSetting(Power23dbm, 9, PowerLevelResponse, 8);
      break;
    case 24:
      ok = setReaderSetting(Power24dbm, 9, PowerLevelResponse, 8);
      break;
    case 25:
      ok = setReaderSetting(Power25dbm, 9, PowerLevelResponse, 8);
      break;
    default:
      ok = setReaderSetting(Power26dbm, 9, PowerLevelResponse, 8);
      break;
  }

  if(ok) {
    Serial.print("RFID: set power level: ");
    Serial.print(powerLevel);
    Serial.println(" dBm");
    ledOn(RFID_LED_PIN);
    wait(200);
    ledOff(RFID_LED_PIN);
    wait(200);
  } else {
    Serial.println("RFID: failed to set power level.");
  }
}

void initRfid() {
  Serial.println("RFID: starting...");
  SerialRFID.begin(115200,SERIAL_8N1, RX_PIN, TX_PIN);
  wait(500);
  
  if(setReaderSetting(StopReadMulti, 7, StopReadMultiResponse, 8)) {
    #ifdef DEBUG
      Serial.println("RFID: Stopped ReadMulti.");
    #endif
  } else {
    Serial.println("RFID: Failed to stop ReadMulti.");
  }

  //set region to Europe
  if(setReaderSetting(Europe, 8, RegionResponse, 8)) {
    Serial.println("RFID: set Europe region.");
    ledOn(RFID_LED_PIN);
    wait(200);
    ledOff(RFID_LED_PIN);
    wait(200);
  } else {
    Serial.println("RFID: failed to set Europe region.");
  }

  //no module sleep time
  if(setReaderSetting(NoModuleSleepTime, 8, NoModuleSleepTimeResponse, 8)) {
    Serial.println("RFID: disabled module sleep time.");
    ledOn(RFID_LED_PIN);
    wait(200);
    ledOff(RFID_LED_PIN);
    wait(200);
  } else {
    Serial.println("RFID: failed to disable module sleep time.");
  }

  //set sensitivity mode
  rfidSetDensitivityMode(digitalRead(DENSE_SWITCH_PIN) == LOW);

  //set power level and start ReadMulti
  setPowerLevel(powerLevel);
  SerialRFID.write(ReadMulti,10);
  Serial.println("RFID: started ReadMulti.");
  Serial.println("RFID: running");
}

int getParameterLength() {
  unsigned char paramLengthBytes[2];
  SerialRFID.readBytes(paramLengthBytes, 2);
  parameterLength = paramLengthBytes[0] << 8;
  parameterLength += paramLengthBytes[1];
  dataCheckSum += paramLengthBytes[0] + paramLengthBytes[1];
  #ifdef DEBUG
    Serial.print("Parameter length: ");
    Serial.println(parameterLength);
  #endif
  return parameterLength;
}

void readDataBytes(unsigned char *dataBytes, int dataLength) {
  SerialRFID.readBytes(dataBytes, dataLength);
  #ifdef DEBUG
    Serial.print("Data Bytes:");
  #endif
  for(int i = 0; i < dataLength; i++) {
    dataCheckSum += dataBytes[i];
    #ifdef DEBUG
      Serial.print(" 0x");
      Serial.print(dataBytes[i], HEX);
    #endif
  }
  #ifdef DEBUG
    Serial.println("");
  #endif
  dataCheckSum = (dataCheckSum & 0xFF);
}

void readRfid() {
  parameterLength = 0;
  if(SerialRFID.available() > 0)
  {
    rfidSerialByte = SerialRFID.read();
    if(!startByte && (rfidSerialByte == 0xAA)) {
      startByte = true;
      #ifdef DEBUG
        Serial.println("Got Start Byte");
      #endif
    }
    else if(startByte && !gotMessageType)
    {
      gotMessageType = true;
      messageType = rfidSerialByte;
      #ifdef DEBUG
        Serial.print("Got Message Type: 0x");
        Serial.println(messageType, HEX);
      #endif
      dataCheckSum = rfidSerialByte;
    }
    else if(gotMessageType) {
      command = rfidSerialByte;
      #ifdef DEBUG
        Serial.print("Command: 0x");
        Serial.println(command, HEX);
      #endif
      dataCheckSum += rfidSerialByte;
      if (getParameterLength() > 0) {
        unsigned char dataBytes[parameterLength];
        readDataBytes(dataBytes, parameterLength);
        unsigned char endBytes[2];
        SerialRFID.readBytes(endBytes, 2);
        bool validData = endBytes[0] == dataCheckSum && endBytes[1] == 0xDD;
        if(validData) {
          if(messageType == 0x01) {
            if(command == 0xFF) {
              #ifdef DEBUG
                Serial.println("No label detected.");
              #endif
            }
          }
          else if(messageType == 0x02) {
            if(command == 0x22) {
              processLabelData(dataBytes);
            }
          }
          #ifdef DEBUG
            Serial.println("Got valid data frame");
            Serial.println("############################");
          #endif
        }
        else {
          #ifdef DEBUG
            Serial.println("Got invalid data frame");
            Serial.println("############################");
          #endif
        }
      }
      resetRfidData();
    }
    else{
      resetRfidData();
    }
  }
  else {
    if ((lastRestart + RFID_RESTART_TIME) < millis()) {
      lastRestart = millis();
      #ifdef DEBUG
        Serial.println("Restart ReadMulti");
      #endif
      if(setReaderSetting(StopReadMulti, 7, StopReadMultiResponse, 8)) {
        #ifdef DEBUG
          Serial.println("RFID: stopped ReadMulti.");
        #endif
      } else {
        #ifdef DEBUG
          Serial.println("RFID: failed to stop ReadMulti.");
        #endif
      }
      SerialRFID.write(ReadMulti,10);
      #ifdef DEBUG
        Serial.println("RFID: started ReadMulti.");
      #endif
    }
  }
}

void resetRfidData() {
  startByte = false;
  gotMessageType = false;
  crc = 0;
  rssi = 0;
  pc = 0;
  dataCheckSum = 0;
  command = 0;
  messageType = 0;
}

void processLabelData(unsigned char *dataBytes) {
  //RSSI
  rssi = dataBytes[0];
  #ifdef DEBUG
    Serial.print("RSSI: 0x");
    Serial.println(rssi, HEX);
  #endif
  //PC
  pc = (dataBytes[1] << 8) + dataBytes[2];
  #ifdef DEBUG
    Serial.print("PC: 0x");
    Serial.println(pc, HEX);
  #endif
  //EPC
  for(int i = 3; i < parameterLength-2; i++) {
    epcBytes[i-3] = dataBytes[i];
    #ifdef DEBUG
      if(i == 3) {
        Serial.print("EPC: ");
      }
      Serial.print(epcBytes[i-3], HEX);
    #endif
  }
  crc = (dataBytes[parameterLength-2] << 8) + dataBytes[parameterLength-1];
  #ifdef DEBUG
    Serial.println("");
    Serial.print("CRC: 0x");
    Serial.println(crc, HEX);
  #endif
  checkRfid(epcBytes);
}

void checkRfid(unsigned char epcBytes[]) {
  char buffer[25]; // Genug Platz für 8 Hex-Ziffern + Nullterminator
  // Konvertiere die Byte-Werte in hexadezimale Zeichen und speichere sie in epcBytes
  for (int i = 0; i < 12; i++) {
    sprintf(buffer + (i * 2), "%02X", epcBytes[i]);
  }
  buffer[24] = '\0'; // Nullterminator am Ende hinzufügen
  String epcString(buffer);

  unsigned long now = millis();
  if(epcString != lastEpcString || (lastEpcRead + RFID_REPEAT_TIME) < now) {
    Serial.print("EPC: " + epcString + " RSSI: 0x");
    Serial.println(rssi, HEX);
    ledOn(RFID_LED_PIN);
    lastEpcString = epcString;
    lastEpcRead = now;
  }
  else {
    #if defined(PRINT_RSSI)
      Serial.print("RSSI: 0x");
      Serial.println(rssi, HEX);
    #endif
  }
}

bool isLedOn(int ledPin) {
  if (digitalRead(ledPin) == LOW) {
    return true;
  }
  return false;
}

void ledOn(int ledPin) {
  digitalWrite(ledPin, LOW);
  rfidLedOnTimeMs = millis();
}

void ledOff(int ledPin) {
  digitalWrite(ledPin, HIGH);
}

void setup() {
  pinMode(RFID_LED_PIN, OUTPUT);
  ledOff(RFID_LED_PIN);

  pinMode(DENSE_SWITCH_PIN, INPUT_PULLUP);
  lastDenseSwitchValue = digitalRead(DENSE_SWITCH_PIN);
  
  Serial.begin(115200);
  wait(2000);

  Serial.print("RFID-Testing Version: ");
  Serial.print(VERSION);
  Serial.println("############################");
  // Start RFID reader
  initRfid();
}

void loop() {
  readRfid();
  if(isLedOn(RFID_LED_PIN) && (rfidLedOnTimeMs + RFID_LED_ON_TIME) < millis()) {
    ledOff(RFID_LED_PIN);
  }
  if(digitalRead(DENSE_SWITCH_PIN) != lastDenseSwitchValue) {
    rfidSetDensitivityMode(digitalRead(DENSE_SWITCH_PIN) == LOW);
    lastDenseSwitchValue = digitalRead(DENSE_SWITCH_PIN);
  }
}
