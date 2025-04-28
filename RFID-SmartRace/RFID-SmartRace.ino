#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>

//ArduinoWebsockets 0.5.4
#include <ArduinoWebsockets.h>

using namespace websockets;

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

String websocket_server = "";
bool ap_mode = false;
bool websocket_connected = false;
unsigned long last_ping = 0;

WebsocketsClient client;

const char* AP_SSID = "RFID-SmartRace-Config";
WebServer server(80);
DNSServer dnsServer;
Preferences preferences;

String ssid, password, serverAddress, serverPort;

const int max_rfid_cnt=6;
const int storage_rfid_cnt = 2;

struct rfid_data {
  String id[storage_rfid_cnt];
  String name;
  unsigned long last;
};
rfid_data rfids[max_rfid_cnt];

String rfid_string = "";

unsigned long lastResetTime = 0;

void saveConfig() {
  preferences.putString("ssid", ssid);
  preferences.putString("password", password);
  preferences.putString("serverAddress", serverAddress);
  preferences.putString("serverPort", serverPort);
  for(int i=0; i<max_rfid_cnt;i++) {
    String key = "RFID" + String(i);
    preferences.putString(key.c_str(), rfids[i].name);
    for(int j=0; j<storage_rfid_cnt;j++) {
      preferences.putString((key + "_" + String(j)).c_str(), rfids[i].id[j]); // Store RFId
    }
  }
}

void loadConfig() {
  ssid = preferences.getString("ssid", "");
  password = preferences.getString("password", "");
  serverAddress = preferences.getString("serverAddress", "");
  serverPort = preferences.getString("serverPort", "");
  for(int i=0; i<max_rfid_cnt;i++) {
    String key = "RFID" + String(i);
    rfids[i].name = preferences.getString(key.c_str(), "Controller " + String(i+1));
    for(int j=0; j<storage_rfid_cnt;j++) {
      rfids[i].id[j] = preferences.getString((key + "_" + String(j)).c_str(), "");
    }
  }
}

void handleRoot() {
  String html = "<!DOCTYPE html><html><head><title>RFID-SmartRace</title>";
  html += "<style>body{display:flex;justify-content:center;align-items:center;min-height:100vh;margin:0;}form{display:flex;flex-direction:column;max-width:100vw;}label{margin-bottom:5px;}input[type=text],input[type=password],input[type=number]{width:100%;padding:8px;margin-bottom:10px;border:1px solid #ccc;border-radius:4px;}input[type=submit]{background-color:#4CAF50;color:white;padding:10px 15px;border:none;border-radius:4px;}</style>";
  html += "</head><body>";
  html += "<form action='/config' method='POST'>";

  //html += "<div>";
  html += "<h1 align=center>RFID-SmartRace</h1>";
  html += "<label for='ssid'>SSID:</label>";
  html += "<input type='text' style='width:auto;' id='ssid' name='ssid' value='" + ssid + "'><br>";
  html += "<label for='password'>Passwort:</label>";
  html += "<input type='password' style='width:auto;' id='password' name='password' value='" + password + "'><br>";
  html += "<label for='serverAddress'>Server Adresse:</label>";
  html += "<input type='text' style='width:auto;' id='serverAddress' name='serverAddress' value='" + serverAddress + "'><br>";
  html += "<label for='serverPort'>Server Port:</label>";
  html += "<input type='number' style='width:auto;' id='serverPort' name='serverPort' value='" + serverPort + "'><br>";
  html += "<input type='submit' style='margin-bottom:20px;' value='Speichern'>";
  //html += "</div>";

  html += "<div style='display: grid; grid-template-columns: 1fr; gap: 5px;'>"; // Äußerer Grid-Container
  for (int i = 0; i < max_rfid_cnt; i++) {
      html += "<div style='border: 3px solid blue; padding: 10px; margin: 5px; display: grid; grid-template-columns: 1fr; gap: 5px;'>"; // Rahmen mit Grid-Spalte
      html += "<div style='display: grid; grid-template-columns: auto 1fr; align-items: center;'>"; // Grid fÃ¼r Name
      html += "<label for='name" + String(i) + "'>Name:</label>";
      html += "<input type='text' maxlength='100' style='width:auto; margin-left: 4px;' id='name" + String(i) + "' name='name" + String(i) + "' value='" + rfids[i].name + "'>";
      html += "</div>";
      html += "<div style='display: grid; grid-template-columns: repeat(" + String(storage_rfid_cnt * 2) + ", auto); align-items: center;'>"; // Grid fÃ¼r horizontale IDs
      for(int j = 0; j < storage_rfid_cnt; j++){
        if(j>0) {
          html += "<label style='margin-left: 4px;' for='id" + String(i) + "_" + String(j) + "'>ID" + String(j+1) + ":</label>";
        }
        else {
          html += "<label for='id" + String(i) + "_" + String(j) + "'>ID" + String(j+1) + ":</label>";
        }
        html += "<input type='text' style='width:auto; margin-left: 4px;' id='id" + String(i) + "_" + String(j) + "' name='id" + String(i) + "_" + String(j) + "' value='" + rfids[i].id[j] + "'>";
      }
      html += "</div>"; // Ende Container fÃ¼r horizontale IDs
      html += "</div>";
  }
  html += "</div>";
  html += "</form></body></html>";
  server.send(200, "text/html", html);
}

void handleConfig() {
  if (server.args() > 0) {
    ssid = server.arg("ssid");
    password = server.arg("password");
    serverAddress = server.arg("serverAddress");
    serverPort = server.arg("serverPort");
    for (int i = 0; i < max_rfid_cnt; i++) {
      rfids[i].name = server.arg("name" + String(i));
      for(int j = 0; j < storage_rfid_cnt; j++) {
        rfids[i].id[j] = server.arg("id" + String(i) + "_" + String(j));
      }
    }

    saveConfig();
    //server.send(200, "text/plain", "Konfiguration gespeichert!");
    server.send(200, "text/html", "<!DOCTYPE html><html><head><title>RFID-SmartRace</title></head><body><h1>Konfiguration gespeichert!</h1><p>Sie werden in 2 Sekunden zur Startseite weitergeleitet.</p><script>setTimeout(function() { window.location.href = '/'; }, 2000);</script></body></html>");
  

    WiFi.begin(ssid.c_str(), password.c_str());

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
      delay(500);
      Serial.print(".");
      attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.print("\nWLAN verbunden ");
      Serial.println(WiFi.localIP());
      WiFi.softAPdisconnect(true);
      dnsServer.stop();
      connectWebsocket();
      ap_mode = false;
    }
    else {
      Serial.println("\nWLAN Verbindung fehlgeschlagen!");
      WiFi.softAP(AP_SSID);
      dnsServer.start(53, "*", WiFi.softAPIP());
      ap_mode = true;
    }
  } else {
    server.send(200, "text/html", "<!DOCTYPE html><html><head><title>RFID-SmartRace</title></head><body><h1>Ungültige Anfrage</h1><p>Sie werden in 2 Sekunden zur Startseite weitergeleitet.</p><script>setTimeout(function() { window.location.href = '/'; }, 2000);</script></body></html>");
  }
}

void connectWebsocket() {
  websocket_server = String("ws://" + serverAddress + ":" + serverPort);
  Serial.print("Connecting to SmartRace at ");
  Serial.println(websocket_server);
  int attempts = 0;
  while(websocket_connected == 0 && attempts < 3) {
    Serial.print(".");
    websocket_connected = client.connect(websocket_server);
    delay(1000);
    attempts++;
  }
  Serial.println("");
  if(websocket_connected) {
    client.send("{\"type\":\"api_version\"}");
    client.ping();
    client.send("{\"type\":\"controller_set\",\"data\":{\"controller_id\":\"Z\"}}");
  }
}

void send_finish_line_message(int controller_id, unsigned long timestamp) {
  if(timestamp > rfids[controller_id].last + 1000) {
    rfids[controller_id].last = timestamp;
    String message = "{\"type\":\"analog_lap\",\"data\":{\"timestamp\":";
    message += rfids[controller_id].last;
    message += ",\"controller_id\":";
    message += controller_id +1;
    message += "}}";
    Serial.println(message);
    client.send(message);
  }
}

void send_finish_line_event(String rfid_string, unsigned long ms) {
  bool found = false;
  for (int i = 0; i < max_rfid_cnt; i++) {
    for(int j = 0; j<storage_rfid_cnt;j++) {
      if(rfids[i].id[j] == rfid_string) {
        send_finish_line_message(i, ms);
        found = true;
        break;
      }
    }
  }
  if(!found) {
    for(int i=0; i < max_rfid_cnt; i++) {
      if(rfids[i].id[0] == "") {
        rfids[i].id[0] = rfid_string;
        Serial.print("INFO - new car at controller id: ");
        Serial.print(i+1);
        Serial.print(rfids[i].id[0]);
        Serial.println();
        send_finish_line_message(i, ms);
        break;
      }
    }
  }
}

void onMessageCallback(WebsocketsMessage message) {
    Serial.print("Got Message: ");
    Serial.print(message.data());
}

void onEventsCallback(WebsocketsEvent event, String data) {
    if(event == WebsocketsEvent::ConnectionOpened) {
        Serial.println("INFO - Connnection Opened");
        websocket_connected = true;
    } else if(event == WebsocketsEvent::ConnectionClosed) {
        Serial.println("INFO - Connnection Closed");
        websocket_connected = false;
    } else if(event == WebsocketsEvent::GotPing) {
        //Serial.println("Got a Ping!");
        client.pong();
    } else if(event == WebsocketsEvent::GotPong) {
        //Serial.println("Got a Pong!");
    }
}

void wait(unsigned long wait_time) {
  unsigned long start_wait_time = millis();
  while((start_wait_time + wait_time) < millis()) {
    ;
  }
}

void resetRfidStorage() {
  for(int i=0; i < max_rfid_cnt; i++) {
    for(int j=0; j<storage_rfid_cnt; j++) {
      rfids[i].id[j] = "";
    }
    rfids[i].name = "Controller " + String(i+1);
    rfids[i].last = millis();
  }
}

void init_rfid() {
  Serial.println("Starting RFID reader...");
  Serial2.begin(115200,SERIAL_8N1, 16, 17);
  wait(2000);
  delay(2000);
  Serial2.write(Europe,8);
  delay(100);
  Serial2.write(DenseReader,8);
  delay(100);
  while(Serial2.available()) {
    Serial2.read();
  }
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
        epc_bytes[dataAdd -9] = incomedate;
       }
       else if(dataAdd >= 21){
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
    send_finish_line_event(epc_string, millis());
    last_epc_string = epc_string;
    last_epc_read = millis();
  }
}

void setup() {
  //init rfid storage
  resetRfidStorage();

  // Setup Callbacks
  client.onMessage(onMessageCallback);
  client.onEvent(onEventsCallback);

  Serial.begin(115200);
  wait(2000);
  Serial.println();
  Serial.println("Starting...");
  preferences.begin("wifiConfig");

  loadConfig();

  if (ssid != "") {
    WiFi.begin(ssid.c_str(), password.c_str());

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
      delay(500);
      Serial.print(".");
      attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.print("\nWLAN verbunden ");
      Serial.println(WiFi.localIP());
      ap_mode = false;
    } else {
      Serial.println("\nWLAN Verbindung fehlgeschlagen!");
      WiFi.softAP(AP_SSID);
      dnsServer.start(53, "*", WiFi.softAPIP());
      Serial.println("AP started ");
      ap_mode = true;
    }
  } else {
    WiFi.softAP(AP_SSID);
    dnsServer.start(53, "*", WiFi.softAPIP());
    Serial.println("AP started ");
    ap_mode = true;
  }

  server.on("/", handleRoot);
  server.on("/config", HTTP_POST, handleConfig);

  server.begin();
  Serial.println("Webserver gestartet...");
  // Start RFID reader
  init_rfid();
}

void loop() {
  server.handleClient();
  dnsServer.processNextRequest();
  
  // process RFID data
  read_rfid();
  if(websocket_connected) {
    client.poll();
    if(last_ping < millis() + 5000) {
      last_ping = millis();
      client.ping();
    }
  }
  else {
    if(ap_mode == false) connectWebsocket();
  }
}
