#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <ArduinoWebsockets.h> //ArduinoWebsockets 0.5.4

/* configuration */
#define DEFAULT_POWER_LEVEL 12 // default power level
#define DEFAULT_MIN_LAP_TIME 3000 //min time between laps in ms
#define AP_SSID "RFID-SmartRace-Config"
#define DEFAULT_HOSTNAME "RFID-SmartRace"

using namespace websockets;

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

#define WIFI_CONNECT_ATTEMPTS 20
#define WIFI_CONNECT_DELAY_MS 500
#define WEBSOCKET_PING_INTERVAL 5000
#define LED_ON_TIME 200
#define RFID_REPEAT_TIME 3000
#define RFID_RESTART_TIME 300000

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

int minLapTime = DEFAULT_MIN_LAP_TIME;

String websocket_server = "";
bool ap_mode = false;
bool websocket_connected = false;
unsigned long last_ping = 0;
unsigned long lastWebsocketAttempt = 0;
unsigned long websocketBackoff = 1000; // Start mit 1 Sekunde
const unsigned long maxWebsocketBackoff = 30000; // Maximal 30 Sekunden

WebsocketsClient client;

String ssl_ca_cert = "";

WebServer server(80);
DNSServer dnsServer;
String hostName = "";
Preferences preferences;

String ssid, password, serverAddress, apiKey;

const int max_rfid_cnt=8;
const int storage_rfid_cnt = 2;

struct rfid_data {
  String id[storage_rfid_cnt];
  String name;
  unsigned long last;
};
rfid_data rfids[max_rfid_cnt];

String rfid_string = "";

unsigned long lastResetTime = 0;

int powerLevel = DEFAULT_POWER_LEVEL;

String targetSystem = "smart_race";

void saveConfig() {
  preferences.putString("ssid", ssid);
  preferences.putString("password", password);
  preferences.putString("serverAddress", serverAddress);
  preferences.putString("hostName", hostName);
  preferences.putInt("powerLevel", powerLevel);
  preferences.putInt("minLapTime", minLapTime);
  preferences.putString("apiKey", apiKey);
  preferences.putString("ssl_ca_cert", ssl_ca_cert);
  preferences.putString("targetSystem", targetSystem);
  char key[20];
  for(int i=0; i<max_rfid_cnt; i++) {
    snprintf(key, sizeof(key), "RFID%d", i);
    preferences.putString(key, rfids[i].name);
    for(int j=0; j<storage_rfid_cnt; j++) {
      snprintf(key, sizeof(key), "RFID%d_%d", i, j);
      preferences.putString(key, rfids[i].id[j]);
    }
  }
}

void loadConfig() {
  ssid = preferences.getString("ssid", "");
  password = preferences.getString("password", "");
  serverAddress = preferences.getString("serverAddress", "");
  apiKey = preferences.getString("apiKey", "");
  hostName = preferences.getString("hostName", DEFAULT_HOSTNAME);
  powerLevel = preferences.getInt("powerLevel", DEFAULT_POWER_LEVEL);
  minLapTime = preferences.getInt("minLapTime", DEFAULT_MIN_LAP_TIME);
  ssl_ca_cert = preferences.getString("ssl_ca_cert", "");
  targetSystem = preferences.getString("targetSystem", "smart_race");
  if (ssl_ca_cert == "") {
    ssl_ca_cert =
      "-----BEGIN CERTIFICATE-----\n"
      "MIIEVzCCAj+gAwIBAgIRAIOPbGPOsTmMYgZigxXJ/d4wDQYJKoZIhvcNAQELBQAw\n"
      "TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh\n"
      "cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMjQwMzEzMDAwMDAw\n"
      "WhcNMjcwMzEyMjM1OTU5WjAyMQswCQYDVQQGEwJVUzEWMBQGA1UEChMNTGV0J3Mg\n"
      "RW5jcnlwdDELMAkGA1UEAxMCRTUwdjAQBgcqhkjOPQIBBgUrgQQAIgNiAAQNCzqK\n"
      "a2GOtu/cX1jnxkJFVKtj9mZhSAouWXW0gQI3ULc/FnncmOyhKJdyIBwsz9V8UiBO\n"
      "VHhbhBRrwJCuhezAUUE8Wod/Bk3U/mDR+mwt4X2VEIiiCFQPmRpM5uoKrNijgfgw\n"
      "gfUwDgYDVR0PAQH/BAQDAgGGMB0GA1UdJQQWMBQGCCsGAQUFBwMCBggrBgEFBQcD\n"
      "ATASBgNVHRMBAf8ECDAGAQH/AgEAMB0GA1UdDgQWBBSfK1/PPCFPnQS37SssxMZw\n"
      "i9LXDTAfBgNVHSMEGDAWgBR5tFnme7bl5AFzgAiIyBpY9umbbjAyBggrBgEFBQcB\n"
      "AQQmMCQwIgYIKwYBBQUHMAKGFmh0dHA6Ly94MS5pLmxlbmNyLm9yZy8wEwYDVR0g\n"
      "BAwwCjAIBgZngQwBAgEwJwYDVR0fBCAwHjAcoBqgGIYWaHR0cDovL3gxLmMubGVu\n"
      "Y3Iub3JnLzANBgkqhkiG9w0BAQsFAAOCAgEAH3KdNEVCQdqk0LKyuNImTKdRJY1C\n"
      "2uw2SJajuhqkyGPY8C+zzsufZ+mgnhnq1A2KVQOSykOEnUbx1cy637rBAihx97r+\n"
      "bcwbZM6sTDIaEriR/PLk6LKs9Be0uoVxgOKDcpG9svD33J+G9Lcfv1K9luDmSTgG\n"
      "6XNFIN5vfI5gs/lMPyojEMdIzK9blcl2/1vKxO8WGCcjvsQ1nJ/Pwt8LQZBfOFyV\n"
      "XP8ubAp/au3dc4EKWG9MO5zcx1qT9+NXRGdVWxGvmBFRAajciMfXME1ZuGmk3/GO\n"
      "koAM7ZkjZmleyokP1LGzmfJcUd9s7eeu1/9/eg5XlXd/55GtYjAM+C4DG5i7eaNq\n"
      "cm2F+yxYIPt6cbbtYVNJCGfHWqHEQ4FYStUyFnv8sjyqU8ypgZaNJ9aVcWSICLOI\n"
      "E1/Qv/7oKsnZCWJ926wU6RqG1OYPGOi1zuABhLw61cuPVDT28nQS/e6z95cJXq0e\n"
      "K1BcaJ6fJZsmbjRgD5p3mvEf5vdQM7MCEvU0tHbsx2I5mHHJoABHb8KVBgWp/lcX\n"
      "GWiWaeOyB7RP+OfDtvi2OsapxXiV7vNVs7fMlrRjY1joKaqmmycnBvAq14AEbtyL\n"
      "sVfOS66B8apkeFX2NY4XPEYV4ZSCe8VHPrdrERk2wILG3T/EGmSIkCYVUMSnjmJd\n"
      "VQD9F6Na/+zmXCc=\n"
      "-----END CERTIFICATE-----\n";
  }
  char key[20];
  char defaultName[20];
  for(int i=0; i<max_rfid_cnt; i++) {
    snprintf(key, sizeof(key), "RFID%d", i);
    snprintf(defaultName, sizeof(defaultName), "Controller %d", i+1);
    rfids[i].name = preferences.getString(key, defaultName);
    for(int j=0; j<storage_rfid_cnt; j++) {
      snprintf(key, sizeof(key), "RFID%d_%d", i, j);
      rfids[i].id[j] = preferences.getString(key, "");
    }
  }
}

void handleNotFound() {
  server.sendHeader("Location", "/");
  server.send(302, "text/plain", "redirect to configuration page");
}

void handleRoot() {
  String html = "<!DOCTYPE html><html><head><title>RFID-SmartRace</title>";
  html += "<meta charset='UTF-8'>"; // Specify UTF-8 encoding
  html += "<style>*, *::before, *::after {box-sizing: border-box;}body {min-height: 100vh;margin: 0;}form {max-width: 535px;margin: 0 auto;}label {margin-bottom: 5px;display:block;}input[type=text],input[type=password],input[type=number],select,textarea {width: 100%;padding: 8px;border: 1px solid #ccc;border-radius: 4px;display: block;}input[type=submit] {width: 100%;background-color: #4CAF50;color: white;padding: 10px 15px;border: none;border-radius: 4px;}</style>";
  html += "<script src='https://unpkg.com/alpinejs' defer></script>";
  html += "</head><body>";
  html += "<form x-data=\"{ targetSystem: '" + targetSystem + "', serverAddress: '" + serverAddress + "' }\" action='/config' method='POST'>";
  html += "<h1 align=center>RFID-SmartRace</h1>";
  html += "<label for='ssid'>SSID:</label>";
  html += "<input type='text' id='ssid' name='ssid' value='" + ssid + "'><br>";
  html += "<label for='password'>Passwort:</label>";
  html += "<input type='password' id='password' name='password' value='" + password + "'><br>";
  html += "<label for='hostName'>Hostname:</label>";
  html += "<input type='text' id='hostName' name='hostName' value='" + hostName + "'><br>";
  html += "<label for='targetSystem'>Target System:</label>";
  html += "<select id='targetSystem' name='targetSystem' x-model='targetSystem'>";
  html += "<option value='smart_race'" + String((targetSystem == "smart_race") ? " selected" : "") + ">SmartRace</option>";
  html += "<option value='ch_racing_club'" + String((targetSystem == "ch_racing_club") ? " selected" : "") + ">CH Racing Club</option>";
  html += "</select><br>";
  html += "<div x-show=\"targetSystem == 'ch_racing_club'\">";
  html += "<label for='apiKey'>ApiKey:</label>";
  html += "<input type='number' id='apiKey' name='apiKey' value='" + apiKey + "'><br>";
  html += "</div>";
  html += "<label for='minLapTime'>Minimum Lap Time (ms):</label>";
  html += "<input type='number' id='minLapTime' name='minLapTime' value='" + String(minLapTime) + "'><br>";
  html += "<label for='powerLevel'>Power Level:</label>";
  html += "<select id='powerLevel' name='powerLevel'>";
  html += "<option value='10'" + String((powerLevel == 10) ? " selected" : "") + ">10 dBm</option>";
  html += "<option value='11'" + String((powerLevel == 11) ? " selected" : "") + ">11 dBm</option>";
  html += "<option value='12'" + String((powerLevel == 12) ? " selected" : "") + ">12 dBm</option>";
  html += "<option value='13'" + String((powerLevel == 13) ? " selected" : "") + ">13 dBm</option>";
  html += "<option value='14'" + String((powerLevel == 14) ? " selected" : "") + ">14 dBm</option>";
  html += "<option value='15'" + String((powerLevel == 15) ? " selected" : "") + ">15 dBm</option>";
  html += "<option value='16'" + String((powerLevel == 16) ? " selected" : "") + ">16 dBm</option>";
  html += "<option value='17'" + String((powerLevel == 17) ? " selected" : "") + ">17 dBm</option>";
  html += "<option value='18'" + String((powerLevel == 18) ? " selected" : "") + ">18 dBm</option>";
  html += "<option value='19'" + String((powerLevel == 19) ? " selected" : "") + ">19 dBm</option>";
  html += "<option value='20'" + String((powerLevel == 20) ? " selected" : "") + ">20 dBm</option>";
  html += "<option value='21'" + String((powerLevel == 21) ? " selected" : "") + ">21 dBm</option>";
  html += "<option value='22'" + String((powerLevel == 22) ? " selected" : "") + ">22 dBm</option>";
  html += "<option value='23'" + String((powerLevel == 23) ? " selected" : "") + ">23 dBm</option>";
  html += "<option value='24'" + String((powerLevel == 24) ? " selected" : "") + ">24 dBm</option>";
  html += "<option value='25'" + String((powerLevel == 25) ? " selected" : "") + ">25 dBm</option>";
  html += "<option value='26'" + String((powerLevel == 26) ? " selected" : "") + ">26 dBm</option>";
  html += "</select><br>";
  html += "<label for='serverAddress'>Websocket Server Adresse:</label>";
  html += "<input type='text' id='serverAddress' name='serverAddress' placeholder='ws:// or wss://' x-model='serverAddress' value='" + serverAddress + "'><br>";
  html += "<div x-show=\"serverAddress.startsWith('wss')\">";
  html += "<label for='ssl_ca_cert'>Websocket SSL CA Certificate (PEM):</label>";
  html += "<textarea id='ssl_ca_cert' name='ssl_ca_cert' rows='12' cols='64' style='font-family:monospace;width:100%;'>" + ssl_ca_cert + "</textarea><br>";
  html += "</div>";
  html += "<input type='submit' style='margin-bottom:20px;' value='Speichern'>";
  html += "<div style='display: grid; grid-template-columns: 1fr; gap: 5px;'>"; // Äußerer Grid-Container
  for (int i = 0; i < max_rfid_cnt; i++) {
      html += "<div style='border: 3px solid black; padding: 10px; margin: 5px; display: grid; grid-template-columns: 1fr; gap: 5px;'>"; // Rahmen mit Grid-Spalte
      html += "<div style='display: grid; grid-template-columns: auto 1fr; align-items: center;'>"; // Grid für Name
      html += "<label for='name" + String(i) + "'>Name:</label>";
      html += "<input type='text' maxlength='100' style='width:auto; margin-left: 4px;' id='name" + String(i) + "' name='name" + String(i) + "' value='" + rfids[i].name + "'>";
      html += "</div>";
      html += "<div style='display: grid; grid-template-columns: repeat(" + String(storage_rfid_cnt * 2) + ", auto); align-items: center;'>"; // Grid für horizontale IDs
      for(int j = 0; j < storage_rfid_cnt; j++){
        if(j>0) {
          html += "<label style='margin-left: 4px;' for='id" + String(i) + "_" + String(j) + "'>ID" + String(j+1) + ":</label>";
        }
        else {
          html += "<label for='id" + String(i) + "_" + String(j) + "'>ID" + String(j+1) + ":</label>";
        }
        html += "<input type='text' style='width:auto; margin-left: 4px;' id='id" + String(i) + "_" + String(j) + "' name='id" + String(i) + "_" + String(j) + "' value='" + rfids[i].id[j] + "'>";
      }
      html += "</div>"; // Ende Container für horizontale IDs
      html += "</div>";
  }
  html += "</div>"; // Ende äußerer Grid-Container
  html += "</form>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}

void handleConfig() {
  if (server.args() > 0) {
    bool reConnectWifi = ssid != server.arg("ssid") || password != server.arg("password") || hostName != server.arg("hostName");
    ssid = server.arg("ssid");
    password = server.arg("password");
    serverAddress = server.arg("serverAddress");
    apiKey = server.arg("apiKey");
    hostName = server.arg("hostName");
    powerLevel = server.arg("powerLevel").toInt(); // Get power level from dropdown
    minLapTime = server.arg("minLapTime").toInt(); // Get minimum lap time from input
    ssl_ca_cert = server.arg("ssl_ca_cert");
    ssl_ca_cert.replace("\r", ""); // Optional: Zeilenumbrüche vereinheitlichen
    targetSystem = server.arg("targetSystem");

    char nameKey[12];
    char idKey[16];
    for (int i = 0; i < max_rfid_cnt; i++) {
      snprintf(nameKey, sizeof(nameKey), "name%d", i);
      rfids[i].name = server.arg(nameKey);
      for (int j = 0; j < storage_rfid_cnt; j++) {
        snprintf(idKey, sizeof(idKey), "id%d_%d", i, j);
        rfids[i].id[j] = server.arg(idKey);
      }
    }
    setPowerLevel(powerLevel); // Set power level
    saveConfig();
    server.send(200, "text/html", "<!DOCTYPE html><html><head><title>RFID-SmartRace</title></head><body><h1>Configuration saved!</h1><p>You will be redirected in 2 seconds.</p><script>setTimeout(function() { window.location.href = '/'; }, 2000);</script></body></html>");

    if(reConnectWifi) {
      if (WiFi.status() == WL_CONNECTED) {
        WiFi.disconnect(true);
        delay(100);
      }
      WiFi.softAPdisconnect(true);
      if(dnsServer.isUp()) {
        dnsServer.stop();
      }

      WiFi.setHostname(hostName.c_str());
      WiFi.begin(ssid.c_str(), password.c_str());

      int attempts = 0;
      while (WiFi.status() != WL_CONNECTED && attempts < WIFI_CONNECT_ATTEMPTS) {
        delay(WIFI_CONNECT_DELAY_MS);
        Serial.print(".");
        attempts++;
      }

      if (WiFi.status() == WL_CONNECTED) {
        Serial.print("\nWLAN verbunden ");
        Serial.println(WiFi.localIP());
        connectWebsocket();
        ap_mode = false;
      } else {
        Serial.println("\nWLAN Verbindung fehlgeschlagen!");
        WiFi.softAP(AP_SSID);
        dnsServer.start();
        ap_mode = true;
      }
    }
  } else {
    server.send(200, "text/html", "<!DOCTYPE html><html><head><title>RFID-SmartRace</title></head><body><h1>Invalid request!</h1><p>You will be redirected in 2 seconds.</p><script>setTimeout(function() { window.location.href = '/'; }, 2000);</script></body></html>");
  }
}

void connectWebsocket() {
  if (millis() - lastWebsocketAttempt < websocketBackoff) {
    return; // wait until backoff time is reached
  }
  lastWebsocketAttempt = millis();

  websocket_server = String(serverAddress);
  Serial.print("Connecting to SmartRace at ");
  Serial.println(websocket_server);

  if (ssl_ca_cert.length() > 0) {
    client.setCACert(ssl_ca_cert.c_str());
  }
  websocket_connected = client.connect(websocket_server);

  if(websocket_connected) {
    Serial.println("Websocket connected.");
    client.send("{\"type\":\"api_version\"}");
    client.ping();
    client.send("{\"type\":\"controller_set\",\"data\":{\"controller_id\":\"Z\"}}");
    websocketBackoff = 1000; // reset backoff time on successful connection
  } else {
    Serial.println("Websocket connection failed.");
    websocketBackoff = min(websocketBackoff * 2, maxWebsocketBackoff); // Exponentielles Backoff
  }
}

void send_finish_line_message(int controller_id, unsigned long timestamp, String rfid_string) {
  rfids[controller_id].last = timestamp;
  String message;
  message.reserve(128); // Reserve enough space for the largest payload

  if (targetSystem == "ch_racing_club") {
    message = "{\"command\":\"lap\",\"data\":{";
    message += "\"timestamp\":" + String(rfids[controller_id].last) + ",";
    message += "\"rfid\":\"" + rfid_string + "\",";
    message += "\"api_key\":\"" + apiKey + "\"";
    message += "}}";
  } else {
    message = "{\"type\":\"analog_lap\",\"data\":{";
    message += "\"timestamp\":" + String(rfids[controller_id].last) + ",";
    message += "\"controller_id\":" + String(controller_id + 1);
    message += "}}";
  }

  Serial.println(message);
  client.send(message);
}

void send_finish_line_event(String rfid_string, unsigned long ms) {
  bool found = false;
  for(int j = 0; j<storage_rfid_cnt;j++) {
    for (int i = 0; i < max_rfid_cnt; i++) {
      if(rfids[i].id[j] == rfid_string) {
        if(rfids[i].last + minLapTime < ms) {
          ledOn();
          send_finish_line_message(i, ms, rfid_string);
        }
        found = true;
        break;
      }
    }
    if (found) {
      break;
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
        ledOn();
        send_finish_line_message(i, ms, rfid_string);
        break;
      }
    }
  }
}

void onMessageCallback(WebsocketsMessage message) {
    #ifdef DEBUG
      Serial.print("Got Message: ");
      Serial.print(message.data());
    #endif
}

void onEventsCallback(WebsocketsEvent event, String data) {
    if(event == WebsocketsEvent::ConnectionOpened) {
        Serial.println("INFO - Connnection Opened");
        websocket_connected = true;
    } else if(event == WebsocketsEvent::ConnectionClosed) {
        Serial.println("INFO - Connnection Closed");
        websocket_connected = false;
    } else if(event == WebsocketsEvent::GotPing) {
        #ifdef DEBUG
          Serial.println("Got a Ping!");
        #endif
        client.pong();
    } else if(event == WebsocketsEvent::GotPong) {
        #ifdef DEBUG
          Serial.println("Got a Pong!");
        #endif
    }
}

void wait(unsigned long waitTime) {
  unsigned long startWaitTime = millis();
  while((millis() - startWaitTime) < waitTime) {
    server.handleClient();
  }
}

void resetRfidStorage() {
  char nameBuf[20];
  for(int i=0; i < max_rfid_cnt; i++) {
    for(int j=0; j<storage_rfid_cnt; j++) {
      rfids[i].id[j] = "";
    }
    snprintf(nameBuf, sizeof(nameBuf), "Controller %d", i+1);
    rfids[i].name = nameBuf;
    rfids[i].last = millis();
  }
}

bool checkResponse(const unsigned char expectedBuffer[], int length) {
  bool ok = true;
  unsigned char buffer[length];
  SerialRFID.readBytes(buffer, length);
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
    if ((lastRestart + RFID_RESTART_TIME) < millis()) {
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
  if(epcString != lastEpcString) {
    send_finish_line_event(epcString, millis());
    lastEpcString = epcString;
  }
  else if ((lastEpcRead + RFID_REPEAT_TIME) < millis()) {
    send_finish_line_event(epcString, millis());
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
  //init rfid storage
  resetRfidStorage();
  delay(2000);

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
    WiFi.setHostname(hostName.c_str());
    WiFi.begin(ssid.c_str(), password.c_str());

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < WIFI_CONNECT_ATTEMPTS) {
      delay(WIFI_CONNECT_DELAY_MS);
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
      dnsServer.start();
      Serial.println("AP started ");
      ap_mode = true;
    }
  } else {
    WiFi.softAP(AP_SSID);
    dnsServer.start();
    Serial.println("AP started ");
    ap_mode = true;
  }

  server.on("/", handleRoot);
  server.on("/config", HTTP_POST, handleConfig);
  // all unknown pages are redirected to configuration page
  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("Webserver gestartet...");

  // Start RFID reader
  initRfid();
  Serial.print("RFID-SmartRace Version: ");
  Serial.print(VERSION);
  Serial.println(" started.");
  Serial.println("############################");
}

void loop() {
  server.handleClient();
  readRfid();
  if(websocket_connected) {
    client.poll();
    if(millis() > (last_ping + WEBSOCKET_PING_INTERVAL)) {
      last_ping = millis();
      client.ping();
    }
  }
  else {
    if(!ap_mode) connectWebsocket();
  }
  if(isLedOn() && (ledOnTime + LED_ON_TIME) < millis()) {
    ledOff();
  }
}
