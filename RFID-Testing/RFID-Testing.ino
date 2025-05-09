unsigned char ReadMulti[10] = {0XAA,0X00,0X27,0X00,0X03,0X22,0XFF,0XFF,0X4A,0XDD};
unsigned char StopReadMulti[7] = {0XAA,0X00,0X28,0X00,0X00,0X28,0XDD};
unsigned char Power10dbm[9] = {0XAA,0X00,0XB6,0X00,0X02,0X03,0XE8,0XA3,0XDD};
unsigned char Power11dbm[9] = {0XAA,0X00,0XB6,0X00,0X02,0X04,0X4C,0X08,0XDD};
unsigned char Power12dbm[9] = {0XAA,0X00,0XB6,0X00,0X02,0X04,0XB0,0X6C,0XDD};
unsigned char Power13dbm[9] = {0XAA,0X00,0XB6,0X00,0X02,0X05,0X14,0XD1,0XDD};
unsigned char Power14dbm[9] = {0XAA,0X00,0XB6,0X00,0X02,0X05,0X78,0X35,0XDD};
unsigned char Power15dbm[9] = {0XAA,0X00,0XB6,0X00,0X02,0X05,0XDC,0X99,0XDD};
unsigned char Power16dbm[9] = {0XAA,0X00,0XB6,0X00,0X02,0X06,0X40,0XFE,0XDD};
unsigned char Power17dbm[9] = {0XAA,0X00,0XB6,0X00,0X02,0X06,0XA4,0X62,0XDD};
unsigned char Power18dbm[9] = {0XAA,0X00,0XB6,0X00,0X02,0X07,0X08,0XC7,0XDD};
unsigned char Power19dbm[9] = {0XAA,0X00,0XB6,0X00,0X02,0X07,0X6C,0X2B,0XDD};
unsigned char Power20dbm[9] = {0XAA,0X00,0XB6,0X00,0X02,0X07,0XD0,0X8F,0XDD};
unsigned char Power21dbm[9] = {0XAA,0X00,0XB6,0X00,0X02,0X08,0X34,0XF4,0XDD};
unsigned char Power22dbm[9] = {0XAA,0X00,0XB6,0X00,0X02,0X08,0X98,0X58,0XDD};
unsigned char Power23dbm[9] = {0XAA,0X00,0XB6,0X00,0X02,0X08,0XFC,0XBC,0XDD};
unsigned char Power24dbm[9] = {0XAA,0X00,0XB6,0X00,0X02,0X09,0X60,0X21,0XDD};
unsigned char Power25dbm[9] = {0XAA,0X00,0XB6,0X00,0X02,0X09,0XC4,0X85,0XDD};
unsigned char Power26dbm[9] = {0XAA,0X00,0XB6,0X00,0X02,0X0A,0X28,0XEA,0XDD};
unsigned char Europe[8] = {0XAA,0X00,0X07,0X00,0X01,0X03,0X0B,0XDD};
unsigned char HighDensitiy[8] = {0XAA,0X00,0XF5,0X00,0X01,0X00,0XF6,0XDD};
unsigned char DenseReader[8] = {0XAA,0X00,0XF5,0X00,0X01,0X01,0XF7,0XDD};
unsigned char NoModuleSleepTime[8] = {0XAA,0X00,0X1D,0x00,0x01,0x00,0x1E,0xDD};

unsigned int rfidSerialByte = 0;
bool startByte = false;
bool gotMessageType = false;
byte messageType = 0;
byte command = 0;
unsigned int rssi = 0;
unsigned int pc = 0;
unsigned int parameterLength = 0;
unsigned int crc = 0;
unsigned int checksum = 0;
unsigned int dataCheckSum = 0;

byte epcBytes[12] = {0,0,0,0,0,0,0,0,0,0,0,0};
String LastEpcString = "";
unsigned long LastEpcRead = 0;
unsigned long lastRestart = 0;

//#define DEBUG
#define MIN_LAP_MS 3000 //min time between laps

void wait(unsigned long waitTime) {
  unsigned long startWaitTime = millis();
  while((millis() - startWaitTime) < waitTime) {
    delay(1);
  }
}

bool checkResponse(const byte expectedBuffer[], int length) {
  bool ok = true;
  byte buffer[length];
  Serial2.readBytes(buffer, length);
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
    Serial.println("");
  #endif
  return ok;
}

void initRfid() {
  Serial.println("Starting RFID reader...");
  Serial2.begin(115200,SERIAL_8N1, 16, 17);
  wait(2000);
  delay(2000);
  while(Serial2.available()) {
    Serial2.read();
  }

  //set region to Europe
  bool ok = false;
  int retries = 0;
  while (!ok && retries < 3) {
    Serial2.write(Europe,8);
    const byte expectedResponse[] = {0xAA,0x01,0x07,0x00,0x01,0x00,0x09,0xDD};
    ok = checkResponse(expectedResponse, 8);
    retries++;
  }
  if (!ok) {
    Serial.println("Failed to set Europe region.");
  }
  else {
    Serial.println("Set Europe region.");
  }
  
  //set dense reader
  ok = false;
  retries = 0;
  while(!ok && retries < 3) {
    Serial2.write(DenseReader,8);
    const byte expectedResponse[] = {0XAA,0X01,0XF5,0X00,0X01,0X00,0XF7,0XDD};
    ok = checkResponse(expectedResponse, 8);
    retries++;
  }
  if (!ok) {
    Serial.println("Failed to set dense reader.");
  }
  else {
    Serial.println("Set dense reader.");
  }

  //no module sleep time
  ok = false;
  retries = 0;
  while(!ok && retries < 3) {
    Serial2.write(NoModuleSleepTime,8);
    const byte expectedResponse[] = {0XAA,0X01,0X1D,0x00,0x01,0x00,0x1F,0xDD};
    ok = checkResponse(expectedResponse, 8);
    retries++;
  }
  if (!ok) {
    Serial.println("Failed to disable module sleep time.");
  }
  else {
    Serial.println("Disabled module sleep time.");
  }
  
  //set power level
  ok = false;
  retries = 0;
  while(!ok && retries < 3) {
    Serial2.write(Power26dbm,9);
    const byte expectedResponse[] = {0xAA,0x01,0xB6,0x00,0x01,0x00,0xB8,0xDD};
    ok = checkResponse(expectedResponse, 8);
    retries++;
  }
  if (!ok) {
    Serial.println("Failed to set power level.");
  }
  else {
    Serial.println("Set power level.");
  }
  
  Serial2.write(ReadMulti,10);
  Serial.println("\nR200 RFID-reader started...");
}

int getParameterLength() {
  byte paramLengthBytes[2];
  Serial2.readBytes(paramLengthBytes, 2);
  parameterLength = paramLengthBytes[0] << 8;
  parameterLength += paramLengthBytes[1];
  dataCheckSum += paramLengthBytes[0] + paramLengthBytes[1];
  #ifdef DEBUG
    Serial.print("Parameter length: ");
    Serial.println(parameterLength);
  #endif
  return parameterLength;
}

void readDataBytes(byte *dataBytes, int dataLength) {
  Serial2.readBytes(dataBytes, dataLength);
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
  if(Serial2.available() > 0)
  {
    rfidSerialByte = Serial2.read();
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
        byte dataBytes[parameterLength];
        readDataBytes(dataBytes, parameterLength);
        byte endBytes[2];
        Serial2.readBytes(endBytes, 2);
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
      Serial2.write(StopReadMulti,7);
      Serial2.write(ReadMulti,10);
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
  checksum = 0;
  command = 0;
  messageType = 0;
}

void processLabelData(byte *dataBytes) {
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

void checkRfid(byte epcBytes[]) {
  char buffer[25]; // Genug Platz für 8 Hex-Ziffern + Nullterminator
  // Konvertiere die Byte-Werte in hexadezimale Zeichen und speichere sie in epcBytes
  for (int i = 0; i < 12; i++) {
    sprintf(buffer + (i * 2), "%02X", epcBytes[i]);
  }
  buffer[24] = '\0'; // Nullterminator am Ende hinzufügen
  String epcString(buffer);
  if(epcString != LastEpcString || (LastEpcRead + MIN_LAP_MS) < millis()) {
    Serial.println(epcString);
    //send_finish_line_event(epcString, millis());
    LastEpcString = epcString;
    LastEpcRead = millis();
  }
}

void setup() {
  Serial.begin(115200);
  wait(2000);
  initRfid();
}

void loop() {
  readRfid();
}
