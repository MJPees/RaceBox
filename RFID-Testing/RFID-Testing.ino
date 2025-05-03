unsigned char ReadMulti[10] = {0XAA,0X00,0X27,0X00,0X03,0X22,0XFF,0XFF,0X4A,0XDD};
unsigned char Power10dbm[9] = {0XAA,0X00,0XB6,0X00,0X02,0X03,0XE8,0XA3,0XDD};
unsigned char Power16dbm[9] ={0XAA,0X00,0XB6,0X00,0X02,0X06,0X40,0XFE,0XDD};
unsigned char Power20dbm[9] ={0XAA,0X00,0XB6,0X00,0X02,0X07,0XD0,0X8F,0XDD};
unsigned char Power25dbm[9] ={0XAA,0X00,0XB6,0X00,0X02,0X09,0XC4,0X85,0XDD};
unsigned char Power26dbm[9] ={0XAA,0X00,0XB6,0X00,0X02,0X0A,0X28,0XEA,0XDD};
unsigned char Europe[8] = {0XAA,0X00,0X07,0X00,0X01,0X03,0X0B,0XDD};
unsigned char HighDensitiy[8] = {0XAA,0X00,0XF5,0X00,0X01,0X00,0XF6,0XDD};
unsigned char DenseReader[8] = {0XAA,0X00,0XF5,0X00,0X01,0X01,0XF7,0XDD};

unsigned int timeSec = 0;
unsigned int timemin = 0;
unsigned int dataAdd = 0;
unsigned int incomedate = 0;
unsigned int parState = 0;
unsigned int codeState = 0;

byte epc_bytes[12] = {0,0,0,0,0,0,0,0,0,0,0,0};
String last_epc_string = "";
unsigned long last_epc_read = 0;

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
  Serial2.write(Europe,8);
  delay(100);
  //Serial.write(HighDensitiy,8);
  Serial2.write(DenseReader,8);
  delay(100);
  while(Serial2.available()) {
    Serial2.read();
  }
  //Serial2.write(Power16dbm,9);
  Serial2.write(Power26dbm,9);
  delay(100);
  while(Serial2.available()) {
    Serial2.read();
  }
  Serial2.write(ReadMulti,10);
  Serial.println("R200 RFID-reader started...");
}

void read_rfid() { 
  if(Serial2.available() > 0)
  {
    incomedate = Serial2.read();
    if((incomedate == 0x02)&(parState == 0))
    {
      parState = 1;
    }
    else if((parState == 1)&(incomedate == 0x22)&(codeState == 0)){  
        codeState = 1;
        dataAdd = 3;
    }
    else if(codeState == 1){
      dataAdd ++;
      if(dataAdd == 6)
      {
        #ifdef DEBUG)
          Serial.print("RSSI:"); 
          Serial.println(incomedate, HEX);
        #endif 
        }
       #ifdef DEBUG
        else if((dataAdd == 7)|(dataAdd == 8)){
          
          if(dataAdd == 7){
            Serial.print("PC:"); 
            Serial.print(incomedate, HEX);
        }
        else {
           Serial.println(incomedate, HEX);
        }
       }
       #endif
       else if((dataAdd >= 9)&(dataAdd <= 20)){
        if(dataAdd == 9){
          Serial.print("EPC:"); 
        }        
        Serial.print(incomedate, HEX);
        epc_bytes[dataAdd -9] = incomedate;
       }
       else if(dataAdd >= 21){
        Serial.println();
        check_rfid(epc_bytes);
        dataAdd= 0;
        parState = 0;
        codeState = 0;
        }
    }
     else{
      dataAdd= 0;
      parState = 0;
      codeState = 0;
    }
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
  read_rfid();
}
