#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>
#include "./src/R200/R200.h"

//ArduinoWebsockets 0.5.4
#include <ArduinoWebsockets.h>

using namespace websockets;

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
R200 rfid;

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

void send_finish_line_message(int controller_id) {
  unsigned long timestamp = millis();
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

void send_finish_line_event(String rfid_string) {
  bool found = false;
  for (int i = 0; i < max_rfid_cnt; i++) {
    for(int j = 0; j<storage_rfid_cnt;j++) {
      if(rfids[i].id[j] == rfid_string) {
        send_finish_line_message(i);
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
        send_finish_line_message(i);
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

String byteArrayToHexString(const uint8_t arr[], size_t size) {
  String hexString = "";
  for (size_t k = 0; k < size; ++k) {
    if (arr[k] < 16) { // Add leading zero if needed
      hexString += "0";
    }
    hexString += String(arr[k], HEX); // Convert to hex and append
  }
  return hexString;
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
  rfid.begin(&Serial2, 115200, 16, 17);
  Serial.println("R200 RFID-reader started...");
  // Get info
  rfid.dumpModuleInfo();
}

void loop() {
  server.handleClient();
  dnsServer.processNextRequest();
  rfid.loop();
  rfid_string = byteArrayToHexString(rfid.uid, 12);
  if (rfid_string != "") {
    send_finish_line_event(rfid_string);
  }
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
  if (millis() - lastResetTime > 500)
  {
    rfid.poll();
    // rfid.dumpUIDToSerial();
    // rfid.getModuleInfo();
    lastResetTime = millis();
  }
}
