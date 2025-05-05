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

unsigned int dataAdd = 0;
unsigned int rfidSerialByte = 0;
unsigned int parState = 0;
unsigned int codeState = 0;
unsigned int rssi = 0;
unsigned int pc = 0;
unsigned int parameter_length = 0;
unsigned int crc = 0;
unsigned int checksum = 0;
unsigned int dataCheckSum = 0;

byte epc_bytes[12] = {0,0,0,0,0,0,0,0,0,0,0,0};
String last_epc_string = "";
unsigned long last_epc_read = 0;

#define DEBUG
#define MIN_LAP_MS 3000 //min time between laps

void wait(unsigned long wait_time) {
  unsigned long start_wait_time = millis();
  while((start_wait_time + wait_time) < millis()) {
    ;
  }
}

void init_rfid() {
  Serial.println("Starting RFID reader...");
  Serial2.begin(115200,SERIAL_8N1, 16, 17);
  wait(2000);
  delay(2000);
  while(Serial2.available()) {
    Serial2.read();
  }
  Serial.print("\nset europe: ");
  Serial2.write(Europe,8);
  while(Serial2.available() == 0) {delay(1);}
  while(Serial2.available()) {
    Serial.print(" 0x");
    Serial.print(Serial2.read(), HEX);
  }
  Serial.print("\nset dense reader: ");
  Serial2.write(DenseReader,8);
  while(Serial2.available() == 0) {delay(1);}
  while(Serial2.available()) {
    Serial.print(" 0x");
    Serial.print(Serial2.read(), HEX);
  }
  Serial.print("\nno module sleep time: ");
  Serial2.write(NoModuleSleepTime,8);
  while(Serial2.available() == 0) {delay(1);}
  while(Serial2.available()) {
    Serial.print(" 0x");
    Serial.print(Serial2.read(), HEX);
  }
  Serial.print("\nset power level: ");
  Serial2.write(Power26dbm,9);
  while(Serial2.available() == 0) {delay(1);}
    while(Serial2.available()) {
    Serial.print(" 0x");
    Serial.print(Serial2.read(), HEX);
  }
  Serial2.write(ReadMulti,10);
  Serial.println("\nR200 RFID-reader started...");
}

void read_rfid() {
  if(Serial2.available() > 0)
  {
    rfidSerialByte = Serial2.read();
    if((rfidSerialByte == 0x02) & (parState == 0))
    {
      parState = 1;
      dataCheckSum = rfidSerialByte;
    }
    else if((parState == 1) & (rfidSerialByte == 0x22) & (codeState == 0)){  
        codeState = 1;
        dataAdd = 3;
        dataCheckSum += rfidSerialByte;
    }
    else if(codeState == 1){
      dataAdd ++;
      switch (dataAdd) {
        case 4:
          parameter_length = int(rfidSerialByte << 8);
          dataCheckSum += rfidSerialByte;
          break;
        case 5:
          parameter_length = parameter_length + int(rfidSerialByte);
          dataCheckSum += rfidSerialByte;
          #ifdef DEBUG
            Serial.println("#########################################");
            Serial.print("Parameter length: ");
            Serial.println(parameter_length);
          #endif
          break;
        case 6:
          rssi = rfidSerialByte;
          dataCheckSum += rfidSerialByte;
          #ifdef DEBUG
            Serial.print("RSSI: 0x"); 
            Serial.println(rssi, HEX);
          #endif
          break;
        case 7:
          pc = rfidSerialByte << 8;
          dataCheckSum +=  rfidSerialByte;
          break;
        case 8:
          pc = pc + rfidSerialByte;
          dataCheckSum +=  rfidSerialByte;
          #ifdef DEBUG
            Serial.print("PC: 0x"); 
            Serial.println(pc, HEX);
          #endif
          break;
        default:
          if((dataAdd >= 9)&(dataAdd <= parameter_length + 3)) {
            #ifdef DEBUG
              if(dataAdd == 9){
                Serial.print("EPC:"); 
              }        
              Serial.print(rfidSerialByte, HEX);
            #endif
            epc_bytes[dataAdd -9] = rfidSerialByte;
            dataCheckSum +=  rfidSerialByte;
          }
          else if(dataAdd == parameter_length + 4) {
            crc = rfidSerialByte << 8;
            dataCheckSum +=  rfidSerialByte;
          }
          else if (dataAdd == parameter_length + 5) {
            crc = crc + rfidSerialByte;
            dataCheckSum +=  rfidSerialByte;
            #ifdef DEBUG
              Serial.println("");
              Serial.print("CRC: 0x");
              Serial.println(crc, HEX);
            #endif
          }
          else if (dataAdd == parameter_length + 6) {
            checksum = rfidSerialByte;
            dataCheckSum = (dataCheckSum & 0xFF);
            #ifdef DEBUG
              Serial.print("Checksum: 0x");
              Serial.println(checksum, HEX);
              Serial.print("dataCheckSum (checksum): 0x");
              Serial.println(dataCheckSum, HEX);
            #endif
          }
          else {
            if(rfidSerialByte == 0xDD && dataCheckSum == checksum) {
              #ifdef DEBUG
                Serial.println("got valid data frame");
              #endif
              check_rfid(epc_bytes);
            }
            else {
              #ifdef DEBUG
                Serial.println("got invalid data frame");
              #endif
            }
            dataAdd= 0;
            parState = 0;
            codeState = 0;
            crc = 0;
            rssi = 0;
            pc = 0;
            parameter_length = 0;
            dataCheckSum = 0;
          }
          break;
      }
    }
    else{
      dataAdd= 0;
      parState = 0;
      codeState = 0;
      crc = 0;
      rssi = 0;
      pc = 0;
      parameter_length = 0;
      dataCheckSum = 0;
    }
  }
  else {
    #ifdef DEBUG
      //Serial.println("No data available -> Restart ReadMulti");
    #endif
    Serial2.write(StopReadMulti,7);
    Serial2.write(ReadMulti,10);
  }
}

void check_rfid(byte epc_bytes[]) {
  char buffer[25]; // Genug Platz für 8 Hex-Ziffern + Nullterminator
  // Konvertiere die Byte-Werte in hexadezimale Zeichen und speichere sie in epc_bytes
  for (int i = 0; i < 12; i++) {
    sprintf(buffer + (i * 2), "%02X", epc_bytes[i]);
  }
  buffer[24] = '\0'; // Nullterminator am Ende hinzufügen
  String epc_string(buffer);
  if(epc_string != last_epc_string || (last_epc_read + MIN_LAP_MS) < millis()) {
    Serial.println(epc_string);
    //send_finish_line_event(epc_string, millis());
    last_epc_string = epc_string;
    last_epc_read = millis();
  }
}

void setup() {
  Serial.begin(115200);
  wait(2000);
  init_rfid();
}

void loop() {
  delay(2);
  read_rfid();
}
