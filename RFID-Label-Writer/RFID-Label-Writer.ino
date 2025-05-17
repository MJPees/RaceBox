#define VERSION "1.0.1"
//#define DEBUG
//#define ESP32C3
#define ESP32DEV
#ifdef ESP32DEV
  #define SerialRFID Serial2
  #define RX_PIN 16
  #define TX_PIN 17
  #define LAP_LED_PIN 2
#elif defined(ESP32C3)
  HardwareSerial SerialRFID(1);
  #define RX_PIN 5
  #define TX_PIN 6
  #define LAP_LED_PIN 8
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
const unsigned char HighDensitiy[8] = {0XAA,0X00,0XF5,0X00,0X01,0X00,0XF6,0XDD};
const unsigned char DenseReader[8] = {0XAA,0X00,0XF5,0X00,0X01,0X01,0XF7,0XDD};
const unsigned char DenseReaderResponse[8] = {0XAA,0X01,0XF5,0X00,0X01,0X00,0XF7,0XDD};
const unsigned char NoModuleSleepTime[8] = {0XAA,0X00,0X1D,0x00,0x01,0x00,0x1E,0xDD};
const unsigned char NoModuleSleepTimeResponse[] = {0XAA,0X01,0X1D,0x00,0x01,0x00,0x1F,0xDD};

const unsigned char DefaultLabelEPC[16] = {0x30,0x08,0x33,0xB2,0xDD,0xD9,0x01,0x40,0x00,0x00,0x00,0x00};
unsigned char nextEPCId = 0x01;

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

unsigned char epcBytes[12] = {0,0,0,0,0,0,0,0,0,0,0,0};
String lastEpcString = "";
unsigned long lastEpcRead = 0;
unsigned long lastRestart = 0;
unsigned long ledOnTime = 0;

int powerLevel = 26; //default power level

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
    SerialRFID.write(sendBuffer, sendLength);
    ok = checkResponse(expectedResponseBuffer, expectedLength);
    retries++;
  }
  return ok;
}

void setPowerLevel(int powerLevel) {
  // Set power level based on loaded configuration
  bool ok;
  while(SerialRFID.available()) {
    SerialRFID.read();
  }
  if(setReaderSetting(StopReadMulti, 7, StopReadMultiResponse, 8)) {
    Serial.println("Stopped ReadMulti.");
  } else {
    Serial.println("Failed to stop ReadMulti.");
  }
  
  while(SerialRFID.available()) {
    SerialRFID.read();
    delay(1);
  }

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
    Serial.println("Set power level.");
    ledOn();
    delay(200);
    ledOff();
    delay(500);
  } else {
    Serial.println("Failed to set power level.");
  }
  SerialRFID.write(ReadMulti,10);
}

void initRfid() {
  Serial.println("Starting RFID reader...");
  SerialRFID.begin(115200,SERIAL_8N1, RX_PIN, TX_PIN);
  wait(500);
  while(SerialRFID.available()) {
    SerialRFID.read();
  }

  //set region to Europe

  if(setReaderSetting(Europe, 8, RegionResponse, 8)) {
    Serial.println("Set Europe region.");
    ledOn();
    delay(200);
    ledOff();
    delay(500);
  } else {
    Serial.println("Failed to set Europe region.");
  }
  
  //set dense reader
  if(setReaderSetting(DenseReader, 8, DenseReaderResponse, 8)) {
    Serial.println("Set dense reader.");
    ledOn();
    delay(200);
    ledOff();
    delay(500);
  } else {
    Serial.println("Failed to set dense reader.");
  }

  //no module sleep time
  if(setReaderSetting(NoModuleSleepTime, 8, NoModuleSleepTimeResponse, 8)) {
    Serial.println("Disabled module sleep time.");
    ledOn();
    delay(200);
    ledOff();
    delay(500);
  } else {
    Serial.println("Failed to disable module sleep time.");
  }
  
  //set power level and start ReadMulti
  setPowerLevel(powerLevel);
  
  Serial.println("\nR200 RFID-reader started...");
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
    if ((lastRestart + 300000) < millis()) {
      lastRestart = millis();
      #ifdef DEBUG
        Serial.println("Restart ReadMulti");
      #endif
      if(setReaderSetting(StopReadMulti, 7, StopReadMultiResponse, 8)) {
        #ifdef DEBUG
          Serial.println("Stopped ReadMulti.");
        #endif
      } else {
        #ifdef DEBUG
          Serial.println("Failed to stop ReadMulti.");
        #endif
      }
      SerialRFID.write(ReadMulti,10);
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
  pc = dataBytes[1] << 8 + dataBytes[2];
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
  crc = dataBytes[parameterLength-2] << 8 + dataBytes[parameterLength-1];
  #ifdef DEBUG
    Serial.println("");
    Serial.print("CRC: 0x"); 
    Serial.println(crc, HEX);
  #endif
  checkRfid(epcBytes);
}

void checkRfid(unsigned char epcBytes[]) {
  bool isDefaultEPC = true;
  char buffer[25];

  for (int i = 0; i < 12; i++) {
    sprintf(buffer + (i * 2), "%02X", epcBytes[i]);
    if (epcBytes[i] != DefaultLabelEPC[i]) {
      isDefaultEPC = false;
      break;
    }
  }
  buffer[24] = '\0'; // Nullterminator am Ende hinzufügen
  String epcString(buffer);
  if(epcString != lastEpcString || ((lastEpcRead + 500) < millis())) {
    Serial.println(epcString);
    lastEpcString = epcString;
  }
  if (isDefaultEPC) {
    Serial.println("Default EPC detected.");
    Serial.println("Writing new EPC...");
    epcBytes[24] = nextEPCId;
    //Set Select parameter 
    unsigned char selectCommand[26];
    selectCommand[0] = 0xAA; // Header
    selectCommand[1] = 0x00; // Type
    selectCommand[2] = 0x0C; // Command
    selectCommand[3] = 0x00; // PL(MSB)
    selectCommand[4] = 0x13; // PL(LSB)
    selectCommand[5] = 0x01; // Select Parameter
    selectCommand[6] = 0x00; // Ptr MSB
    selectCommand[7] = 0x00; // Ptr
    selectCommand[8] = 0x00; // Ptr
    selectCommand[9] = 0x20; // Ptr LSB
    selectCommand[10] = 0x60; // Masklength
    selectCommand[11] = 0x00; // Truncation
    selectCommand[12] = epcBytes[0]; // EPC MASK
    selectCommand[13] = epcBytes[1];
    selectCommand[14] = epcBytes[2];
    selectCommand[15] = epcBytes[3];
    selectCommand[16] = epcBytes[4];
    selectCommand[17] = epcBytes[5];
    selectCommand[18] = epcBytes[6];
    selectCommand[19] = epcBytes[7];
    selectCommand[20] = epcBytes[8];
    selectCommand[21] = epcBytes[9];
    selectCommand[22] = epcBytes[10];
    selectCommand[23] = epcBytes[11];
    unsigned char checksum = 0x00;
    for(int i = 2; i < 24; i++) {
        checksum += selectCommand[i];
    }
    selectCommand[24] = checksum; // Checksum
    selectCommand[25] = 0xDD;

    setReaderSetting(StopReadMulti, 7, StopReadMultiResponse, 8);
    unsigned char expectedSelectResponse[8] = {0xAA,0x01,0x0C,0x00,0x01,0x00,0x0E,0xDD};
    while(SerialRFID.available()) {
        SerialRFID.read();
    }
    if(setReaderSetting(selectCommand, 26, expectedSelectResponse, 8)) {
        Serial.println("Select command sent.");
        //write new EPC
        unsigned char writeCommand[28]; // Assuming max 32 words (64 bytes) for DT
        writeCommand[0] = 0xAA; // Header
        writeCommand[1] = 0x00; // Type
        writeCommand[2] = 0x49; // Command
        writeCommand[3] = 0x00; // PL(MSB)
        writeCommand[4] = 0x15; // PL(LSB)
        writeCommand[5] = 0x00; // AP(MSB) - Access Password (assuming no password)
        writeCommand[6] = 0x00;
        writeCommand[7] = 0x00;
        writeCommand[8] = 0x00; // AP(LSB)
        writeCommand[9] = 0x01; // MemBank (EPC)
        writeCommand[10] = 0x00; // SA(MSB) - Start Address (word offset, 0x0002 to skip PC bits)
        writeCommand[11] = 0x02; // SA(LSB)
        writeCommand[12] = 0x00; // DL(MSB) - Data Length (6 words = 12 bytes)
        writeCommand[13] = 0x06; // DL(LSB)
        writeCommand[14] = epcBytes[0]; // DT (New EPC Data)
        writeCommand[15] = epcBytes[1];
        writeCommand[16] = epcBytes[2];
        writeCommand[17] = epcBytes[3];
        writeCommand[18] = epcBytes[4];
        writeCommand[19] = epcBytes[5];
        writeCommand[20] = epcBytes[6];
        writeCommand[21] = epcBytes[7];
        writeCommand[22] = epcBytes[8];
        writeCommand[23] = epcBytes[9];
        writeCommand[24] = epcBytes[10];
        writeCommand[25] = nextEPCId & 0xFF; // New EPC ID
        unsigned char checksum = 0x00;
        for(int i = 1; i < 26; i++) {
            checksum += writeCommand[i];
        }
        writeCommand[26] = checksum; // Checksum
        writeCommand[27] = 0xDD; // End
        // Prepare expected response
        unsigned char expectedWriteResponse[23];
        expectedWriteResponse[0] = 0xAA; // Header
        expectedWriteResponse[1] = 0x01; // Type
        expectedWriteResponse[2] = 0x49; // Command
        expectedWriteResponse[3] = 0x00; // PL(MSB)
        expectedWriteResponse[4] = 0x10; // PL(LSB)
        expectedWriteResponse[5] = 0x0E;
        expectedWriteResponse[6] = 0x34; // PC MSB
        expectedWriteResponse[7] = 0x00; // PC LSB
        expectedWriteResponse[8] = epcBytes[0]; // DT (New EPC Data)
        expectedWriteResponse[9] = epcBytes[1];
        expectedWriteResponse[10] = epcBytes[2];
        expectedWriteResponse[11] = epcBytes[3];
        expectedWriteResponse[12] = epcBytes[4];
        expectedWriteResponse[13] = epcBytes[5];
        expectedWriteResponse[14] = epcBytes[6];
        expectedWriteResponse[15] = epcBytes[7];
        expectedWriteResponse[16] = epcBytes[8];
        expectedWriteResponse[17] = epcBytes[9];
        expectedWriteResponse[18] = epcBytes[10];
        expectedWriteResponse[19] = epcBytes[11];
        expectedWriteResponse[20] = 0x00; // Result
        checksum = 0x00;
        for(int i = 1; i < 21; i++) {
            checksum += expectedWriteResponse[i];
        }
        expectedWriteResponse[21] = checksum; // Checksum
        expectedWriteResponse[22] = 0xDD; // End
        while(SerialRFID.available()) {
            SerialRFID.read();
        }
        if(setReaderSetting(writeCommand, 28, expectedWriteResponse, 23)) {
            Serial.println("Wrote EPC to label.");
            ledOn();
            nextEPCId++;
            ledOn();
        } else {
            Serial.println("Could not write EPC to label.");
        }
    } else {
      Serial.println("Failed to send select command.");
    }
    SerialRFID.write(ReadMulti, 10);
  }
  lastEpcRead = millis();
}

bool isLedOn() {
  if (digitalRead(LAP_LED_PIN) == LOW) {
    return true;
  }
  return false;
}

void ledOn() {
  digitalWrite(LAP_LED_PIN, LOW);
  ledOnTime = millis();
}

void ledOff() {
  digitalWrite(LAP_LED_PIN, HIGH);
}

void setup() {
  pinMode(LAP_LED_PIN, OUTPUT);
  digitalWrite(LAP_LED_PIN, HIGH);
  Serial.begin(115200);
  wait(2000);
  initRfid();
  Serial.print("RFID-Label-Writer Version: ");
  Serial.print(VERSION);
  Serial.println(" started.");
  Serial.println("############################");
}

void loop() {
  readRfid();
  if(isLedOn() && (ledOnTime + 500) < millis()) {
    ledOff();
  }
}
