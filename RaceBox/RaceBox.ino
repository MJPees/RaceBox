#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <ArduinoWebsockets.h> //ArduinoWebsockets 0.5.4
#include <ArduinoJson.h>

/* configuration */
#define RFID_DEFAULT_POWER_LEVEL 26 // default power level
#define DEFAULT_MIN_LAP_TIME 3000 //min time between laps in ms
#define WIFI_AP_SSID "RaceBox-Config"
#define WIFI_DEFAULT_HOSTNAME "racebox"
#define PREFERENCES_NAMESPACE "racebox"

#define VERSION "1.3.0" // dont forget to update the releases.json
//#define DEBUG

//#define ESP32C3
#define ESP32DEV

//#define INVERT_LEDS
//#define RaceBox_PCB_V1_0

#ifdef ESP32DEV
  #define SerialRFID Serial2
  #ifdef RaceBox_PCB_V1_0
    #define RX_PIN 17
    #define TX_PIN 16
    #define RFID_LED_PIN 32
    #define WEBSOCKET_LED_PIN 33
    #define WIFI_AP_LED_PIN 25
    #define PUSH_BUTTON_PIN 23
  #else
    #define RX_PIN 16
    #define TX_PIN 17
    #ifdef INVERT_LEDS
      #define RFID_LED_PIN 32
      #define WEBSOCKET_LED_PIN 33
      #define WIFI_AP_LED_PIN 25
    #else
      #define RFID_LED_PIN 2
      #define WEBSOCKET_LED_PIN 4
      #define WIFI_AP_LED_PIN 25
    #endif
  #endif
#elif defined(ESP32C3)
  HardwareSerial SerialRFID(1);
  #define RX_PIN 5
  #define TX_PIN 6
  #define RFID_LED_PIN 8
  #define WEBSOCKET_LED_PIN 9
  #define WIFI_AP_LED_PIN 10
#endif

#define WIFI_CONNECT_ATTEMPTS 20
#define WIFI_CONNECT_DELAY_MS 500
#define WEBSOCKET_PING_INTERVAL 5000
#define RFID_LED_ON_TIME 200

#define RFID_REPEAT_TIME 3000
#define RFID_RESTART_TIME 300000
#define RFID_MAX_COUNT 8
#define RFID_STORAGE_COUNT 2

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

unsigned char epcBytes[12] = {0,0,0,0,0,0,0,0,0,0,0,0};
String lastEpcString = "";
unsigned long lastEpcRead = 0;
unsigned long lastRestart = 0;
unsigned long rfid_led_on_ms = 0;

int config_min_lap_time = DEFAULT_MIN_LAP_TIME;

bool wifi_ap_mode = false;
bool webserver_running = false;

using namespace websockets;
String websocket_server = "";
String websocket_ca_cert = "";
bool websocket_connected = false;
unsigned long websocket_last_ping = 0;
unsigned long websocket_last_attempt = 0;
unsigned long websocket_backoff = 1000; // Start mit 1 Sekunde
const unsigned long websocket_max_backoff = 30000; // Maximal 30 Sekunden
WebsocketsClient client;

WebServer server(80);
DNSServer dnsServer;

Preferences preferences;

String config_target_system = "smart_race";
String config_wifi_ssid;
String config_wifi_password;
String config_wifi_hostname;

String config_smart_race_websocket_server;
String config_smart_race_websocket_ca_cert;

String config_ch_racing_club_websocket_server;
String config_ch_racing_club_websocket_ca_cert;
String config_ch_racing_club_api_key;

int config_rfid_power_level = RFID_DEFAULT_POWER_LEVEL;
bool config_rfid_dense_mode = true;

struct rfid_data {
  String id[RFID_STORAGE_COUNT];
  String name;
  unsigned long last;
};
rfid_data rfids[RFID_MAX_COUNT];

String rfid_string = "";

unsigned long lastResetTime = 0;


void configuration_save() {
  preferences.putString("target_system", config_target_system);
  preferences.putInt("power_level", config_rfid_power_level);
  preferences.putBool("rfid_dense_mode", config_rfid_dense_mode);
  preferences.putInt("min_lap_time", config_min_lap_time);

  preferences.putString("wifi_ssid", config_wifi_ssid);
  preferences.putString("wifi_password", config_wifi_password);
  preferences.putString("wifi_hostname", config_wifi_hostname);

  preferences.putString("sr_ws_server", config_smart_race_websocket_server);
  preferences.putString("sr_ws_ca_cert", config_smart_race_websocket_ca_cert);

  preferences.putString("chrc_ws_server", config_ch_racing_club_websocket_server);
  preferences.putString("chrc_ws_ca_cert", config_ch_racing_club_websocket_ca_cert);
  preferences.putString("chrc_api_key", config_ch_racing_club_api_key);

  char key[20];
  for(int i=0; i<RFID_MAX_COUNT; i++) {
    snprintf(key, sizeof(key), "RFID%d", i);
    preferences.putString(key, rfids[i].name);
    for(int j=0; j<RFID_STORAGE_COUNT; j++) {
      snprintf(key, sizeof(key), "RFID%d_%d", i, j);
      preferences.putString(key, rfids[i].id[j]);
    }
  }
}

void configuration_load() {
  config_target_system = preferences.getString("target_system", "smart_race");
  config_rfid_dense_mode = preferences.getBool("rfid_dense_mode", true);
  config_rfid_power_level = preferences.getInt("power_level", RFID_DEFAULT_POWER_LEVEL);
  config_min_lap_time = preferences.getInt("min_lap_time", DEFAULT_MIN_LAP_TIME);

  config_wifi_ssid = preferences.getString("wifi_ssid", "");
  config_wifi_password = preferences.getString("wifi_password", "");
  config_wifi_hostname = preferences.getString("wifi_hostname", WIFI_DEFAULT_HOSTNAME);

  config_smart_race_websocket_server = preferences.getString("sr_ws_server", "");
  config_smart_race_websocket_ca_cert = preferences.getString("sr_ws_ca_cert", "");

  config_ch_racing_club_websocket_server = preferences.getString("chrc_ws_server", "");
  config_ch_racing_club_websocket_ca_cert = preferences.getString("chrc_ws_ca_cert", "");
  config_ch_racing_club_api_key = preferences.getString("chrc_api_key", "");

  if (config_target_system == "smart_race") {
    websocket_server = config_smart_race_websocket_server;
    websocket_ca_cert = config_smart_race_websocket_ca_cert;

    Serial.println("\nConfiguration: RaceBox loaded");
  }

  if (config_target_system == "ch_racing_club") {
    websocket_server = config_ch_racing_club_websocket_server;
    websocket_ca_cert = config_ch_racing_club_websocket_ca_cert;

    Serial.println("\nConfiguration: CH Racing Club loaded");
  }

  char key[20];
  char defaultName[20];
  for(int i=0; i<RFID_MAX_COUNT; i++) {
    snprintf(key, sizeof(key), "RFID%d", i);
    snprintf(defaultName, sizeof(defaultName), "Controller %d", i+1);
    rfids[i].name = preferences.getString(key, defaultName);
    for(int j=0; j<RFID_STORAGE_COUNT; j++) {
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
  String html = "<!DOCTYPE html><html><head><title>RaceBox</title>";
  html += "<meta charset='UTF-8'>"; // Specify UTF-8 encoding
  html += "<style>";
  html += "*, *::before, *::after {box-sizing: border-box;}";
  html += "body {min-height: 100vh;margin: 0;}";
  html += "form {max-width: 535px;margin: 0 auto;}";
  html += "label {margin-bottom: 5px;display:block;}";
  html += "input[type=text],input[type=password],input[type=number],select,textarea {width: 100%;padding: 8px;border: 1px solid #ccc;border-radius: 4px;display: block;}";
  html += "input[type=submit] {width: 100%;background-color: #4CAF50;color: white;padding: 10px 15px;border: none;border-radius: 4px;}";
  html += ".checkbox-container { display: flex; align-items: center; gap: 5px; }";
  html += "</style>";
  html += "<script src='https://unpkg.com/alpinejs' defer></script>";
  html += "</head><body>";
  html += "<form x-data=\"{ targetSystem: '" + config_target_system + "', smartRaceWebsocketServer: '" + config_smart_race_websocket_server + "', racingClubWebsocketServer: '" + config_ch_racing_club_websocket_server + "' }\" action='/config' method='POST'>";
  html += "<h1 align=center>RaceBox</h1>";

  html += "<label for='config_wifi_ssid'>SSID:</label>";
  html += "<input type='text' id='config_wifi_ssid' name='config_wifi_ssid' value='" + config_wifi_ssid + "'><br>";

  html += "<label for='config_wifi_password'>Passwort:</label>";
  html += "<input type='password' id='config_wifi_password' name='config_wifi_password' value='" + config_wifi_password + "'><br>";

  html += "<label for='config_wifi_hostname'>Hostname:</label>";
  html += "<input type='text' id='config_wifi_hostname' name='config_wifi_hostname' value='" + config_wifi_hostname + "'><br>";

  if (!wifi_ap_mode) {
    html += "<label for='config_target_system'>Target System:</label>";
    html += "<select id='config_target_system' name='config_target_system' x-model='targetSystem'>";
    html += "<option value='smart_race'" + String((config_target_system == "smart_race") ? " selected" : "") + ">SmartRace</option>";
    html += "<option value='ch_racing_club'" + String((config_target_system == "ch_racing_club") ? " selected" : "") + ">CH Racing Club</option>";
    html += "</select><br>";

    html += "<div x-show=\"targetSystem == 'ch_racing_club'\">";
    html += "  <label for='config_ch_racing_club_websocket_server'>Websocket Server:</label>";
    html += "  <input type='text' id='config_ch_racing_club_websocket_server' name='config_ch_racing_club_websocket_server' placeholder='ws:// or wss://' x-model='racingClubWebsocketServer' value='" + config_ch_racing_club_websocket_server + "'><br>";
    html += "  <div x-show=\"racingClubWebsocketServer.startsWith('wss')\">";
    html += "    <label for='config_ch_racing_club_websocket_ca_cert'>Websocket SSL CA Certificate (PEM):</label>";
    html += "    <textarea id='config_ch_racing_club_websocket_ca_cert' name='config_ch_racing_club_websocket_ca_cert' rows='12' cols='64' style='font-family:monospace;width:100%;'>" + config_ch_racing_club_websocket_ca_cert + "</textarea><br>";
    html += "  </div>";
    html += "  <label for='config_ch_racing_club_api_key'>ApiKey:</label>";
    html += "  <input type='text' id='config_ch_racing_club_api_key' name='config_ch_racing_club_api_key' value='" + config_ch_racing_club_api_key + "'><br>";
    html += "</div>";

    html += "<div x-show=\"targetSystem == 'smart_race'\">";
    html += "  <label for='config_smart_race_websocket_server'>Websocket Server:</label>";
    html += "  <input type='text' id='config_smart_race_websocket_server' name='config_smart_race_websocket_server' placeholder='ws:// or wss://' x-model='smartRaceWebsocketServer' value='" + config_smart_race_websocket_server + "'><br>";
    html += "  <div x-show=\"smartRaceWebsocketServer.startsWith('wss')\">";
    html += "    <label for='config_smart_race_websocket_ca_cert'>Websocket SSL CA Certificate (PEM):</label>";
    html += "    <textarea id='config_smart_race_websocket_ca_cert' name='config_smart_race_websocket_ca_cert' rows='12' cols='64' style='font-family:monospace;width:100%;'>" + config_smart_race_websocket_ca_cert + "</textarea><br>";
    html += "  </div>";
    html += "</div>";

    html += "<label for='config_min_lap_time'>Minimum Lap Time (ms):</label>";
    html += "<input type='number' id='config_min_lap_time' name='config_min_lap_time' value='" + String(config_min_lap_time) + "'><br>";

    html += "<label for='config_rfid_power_level'>Power Level:</label>";
    html += "<select id='config_rfid_power_level' name='config_rfid_power_level'>";
    html += "<option value='10'" + String((config_rfid_power_level == 10) ? " selected" : "") + ">10 dBm</option>";
    html += "<option value='11'" + String((config_rfid_power_level == 11) ? " selected" : "") + ">11 dBm</option>";
    html += "<option value='12'" + String((config_rfid_power_level == 12) ? " selected" : "") + ">12 dBm</option>";
    html += "<option value='13'" + String((config_rfid_power_level == 13) ? " selected" : "") + ">13 dBm</option>";
    html += "<option value='14'" + String((config_rfid_power_level == 14) ? " selected" : "") + ">14 dBm</option>";
    html += "<option value='15'" + String((config_rfid_power_level == 15) ? " selected" : "") + ">15 dBm</option>";
    html += "<option value='16'" + String((config_rfid_power_level == 16) ? " selected" : "") + ">16 dBm</option>";
    html += "<option value='17'" + String((config_rfid_power_level == 17) ? " selected" : "") + ">17 dBm</option>";
    html += "<option value='18'" + String((config_rfid_power_level == 18) ? " selected" : "") + ">18 dBm</option>";
    html += "<option value='19'" + String((config_rfid_power_level == 19) ? " selected" : "") + ">19 dBm</option>";
    html += "<option value='20'" + String((config_rfid_power_level == 20) ? " selected" : "") + ">20 dBm</option>";
    html += "<option value='21'" + String((config_rfid_power_level == 21) ? " selected" : "") + ">21 dBm</option>";
    html += "<option value='22'" + String((config_rfid_power_level == 22) ? " selected" : "") + ">22 dBm</option>";
    html += "<option value='23'" + String((config_rfid_power_level == 23) ? " selected" : "") + ">23 dBm</option>";
    html += "<option value='24'" + String((config_rfid_power_level == 24) ? " selected" : "") + ">24 dBm</option>";
    html += "<option value='25'" + String((config_rfid_power_level == 25) ? " selected" : "") + ">25 dBm</option>";
    html += "<option value='26'" + String((config_rfid_power_level == 26) ? " selected" : "") + ">26 dBm</option>";
    html += "</select><br>";
    html += "<div class='checkbox-container'>";
    html += "<label for='config_dense_mode'>Use RFID-Dense-Mode:</label>";
    if (config_rfid_dense_mode) {
      html += "<input type='checkbox' id='config_dense_mode' name='config_dense_mode' checked>";
    } else {
      html += "<input type='checkbox' id='config_dense_mode' name='config_dense_mode'>";
    }
    html += "</div><br>";
  }

  html += "<input type='submit' style='margin-bottom:20px;' value='Speichern'>";
  if (!wifi_ap_mode) {
    html += "<div style='display: grid; grid-template-columns: 1fr; gap: 5px;'>"; // Äußerer Grid-Container
    for (int i = 0; i < RFID_MAX_COUNT; i++) {
        html += "<div style='border: 3px solid black; padding: 10px; margin: 5px; display: grid; grid-template-columns: 1fr; gap: 5px;'>"; // Rahmen mit Grid-Spalte
        html += "<div style='display: grid; grid-template-columns: auto 1fr; align-items: center;'>"; // Grid für Name
        html += "<label for='name" + String(i) + "'>Name:</label>";
        html += "<input type='text' maxlength='100' style='width:auto; margin-left: 4px;' id='name" + String(i) + "' name='name" + String(i) + "' value='" + rfids[i].name + "'>";
        html += "</div>";
        html += "<div style='display: grid; grid-template-columns: repeat(" + String(RFID_STORAGE_COUNT * 2) + ", auto); align-items: center;'>"; // Grid für horizontale IDs
        for(int j = 0; j < RFID_STORAGE_COUNT; j++){
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
  }
  html += "</form>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}

void handleConfig() {
  if (server.args() > 0) {
    if(setReaderSetting(StopReadMulti, 7, StopReadMultiResponse, 8)) {
      Serial.println("RFID: stopped ReadMulti.");
    } else {
      Serial.println("RFID: failed to stop ReadMulti.");
    }
    bool reConnectWifi = config_wifi_ssid != server.arg("config_wifi_ssid") || config_wifi_password != server.arg("config_wifi_password") || config_wifi_hostname != server.arg("config_wifi_hostname");
    bool reConnectWebsocket = false;

    config_wifi_ssid = server.arg("config_wifi_ssid");
    config_wifi_password = server.arg("config_wifi_password");
    config_wifi_hostname = server.arg("config_wifi_hostname");


    if(!wifi_ap_mode) {
      reConnectWebsocket = config_target_system != server.arg("config_target_system");

      if (server.hasArg("config_dense_mode")) {
        config_rfid_dense_mode = true;
      } else {
          config_rfid_dense_mode = false;
      }
      config_target_system = server.arg("config_target_system");
      config_rfid_power_level = server.arg("config_rfid_power_level").toInt();
      config_min_lap_time = server.arg("config_min_lap_time").toInt();
      if (config_target_system == "smart_race") {
        if (config_smart_race_websocket_server != server.arg("config_smart_race_websocket_server")) {
          reConnectWebsocket = true; // Reconnect if the server has changed
        }
        config_smart_race_websocket_server = server.arg("config_smart_race_websocket_server");
        config_smart_race_websocket_ca_cert = server.arg("config_smart_race_websocket_ca_cert");
        config_smart_race_websocket_ca_cert.replace("\r", "");

        websocket_server = config_smart_race_websocket_server;
        websocket_ca_cert = config_smart_race_websocket_ca_cert;
      }

      if (config_target_system == "ch_racing_club") {
        if (config_ch_racing_club_websocket_server != server.arg("config_ch_racing_club_websocket_server")) {
          reConnectWebsocket = true; // Reconnect if the server has changed
        }
        config_ch_racing_club_websocket_server = server.arg("config_ch_racing_club_websocket_server");
        config_ch_racing_club_websocket_ca_cert = server.arg("config_ch_racing_club_websocket_ca_cert");
        config_ch_racing_club_websocket_ca_cert.replace("\r", "");
        config_ch_racing_club_api_key = server.arg("config_ch_racing_club_api_key");

        websocket_server = config_ch_racing_club_websocket_server;
        websocket_ca_cert = config_ch_racing_club_websocket_ca_cert;
      }

      char nameKey[12];
      char idKey[16];
      for (int i = 0; i < RFID_MAX_COUNT; i++) {
        snprintf(nameKey, sizeof(nameKey), "name%d", i);
        rfids[i].name = server.arg(nameKey);
        for (int j = 0; j < RFID_STORAGE_COUNT; j++) {
          snprintf(idKey, sizeof(idKey), "id%d_%d", i, j);
          rfids[i].id[j] = server.arg(idKey);
        }
      }
    }

    configuration_save();

    server.send(200, "text/html", "<!DOCTYPE html><html><head><title>RaceBox</title></head><body><h1>Configuration saved!</h1><p>You will be redirected in 2 seconds.</p><script>setTimeout(function() { window.location.href = 'http://" + config_wifi_hostname + "'; }, 2000);</script></body></html>");
    wait(500);
    if (reConnectWebsocket) {
      if(websocket_connected) {
        client.close();
        wait(500);
      }
      websocket_last_attempt = 0;
      connectWebsocket();
    }

    if(reConnectWifi) {
      wifi_reload();
    }
    rfidSetDensitivityMode(config_rfid_dense_mode);
    rfid_set_power_level(config_rfid_power_level);
    SerialRFID.write(ReadMulti,10);
    Serial.println("RFID: started ReadMulti.");
  } else {
    server.send(200, "text/html", "<!DOCTYPE html><html><head><title>RaceBox</title></head><body><h1>Invalid request!</h1><p>You will be redirected in 2 seconds.</p><script>setTimeout(function() { window.location.href = 'http://" + config_wifi_hostname + "'; }, 2000);</script></body></html>");
  }
}

void wifi_reload() {
  Serial.println("WiFi: Initiating reload...");
  if (WiFi.getMode() != WIFI_OFF) {
    Serial.println("WiFi: Disconnecting existing connections...");
    if (WiFi.status() == WL_CONNECTED) {
      WiFi.disconnect(true);
      wait(500);
    }
    WiFi.softAPdisconnect(true);
    wait(500);
  }
  if(dnsServer.isUp()) {
    dnsServer.stop();
    wait(100);
  }

  WiFi.setHostname(config_wifi_hostname.c_str());
  Serial.print("WiFi: Attempting to connect to AP '");
  Serial.print(config_wifi_ssid);
  Serial.println("'...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(config_wifi_ssid.c_str(), config_wifi_password.c_str());

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < WIFI_CONNECT_ATTEMPTS) {
    wait(WIFI_CONNECT_DELAY_MS);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("\nWiFi: connected ");
    Serial.println(WiFi.localIP());
    Serial.print("WiFi: Hostname: ");
    Serial.println(WiFi.getHostname());
    wifi_ap_mode = false;
    ledOff(WIFI_AP_LED_PIN);
  } else {
    Serial.println("\nWiFi: connect failed, starting AP mode");
    WiFi.disconnect(true);
    wait(500);
    WiFi.softAP(WIFI_AP_SSID);
    dnsServer.start();
    wifi_ap_mode = true;
    ledOn(WIFI_AP_LED_PIN);
    Serial.print("\nWiFi: AP started, IP: ");
    Serial.println(WiFi.softAPIP());
  }
}

void connectWebsocket() {
  if (millis() - websocket_last_attempt < websocket_backoff) {
    return; // wait until backoff time is reached
  }
  websocket_last_attempt = millis();

  client = WebsocketsClient(); // Überschreibt das alte Objekt

  client.onMessage(onMessageCallback);
  client.onEvent(onEventsCallback);

  websocket_server = String(websocket_server);
  Serial.print("Websocket: connecting ... ");
  Serial.println(websocket_server);

  if (websocket_ca_cert.length() > 0) {
    client.setCACert(websocket_ca_cert.c_str());
  }
  websocket_connected = client.connect(websocket_server);

  if(websocket_connected) {
    JsonDocument doc;
    if(config_target_system == "smart_race") {
      doc["type"] = "controller_set";
      doc["data"]["controller_id"] = "Z";
    }

    if(config_target_system == "ch_racing_club") {
      doc["command"] = "racebox_connect";
      doc["data"]["api_key"] = config_ch_racing_club_api_key;
      doc["data"]["name"] = config_wifi_hostname.c_str();
      doc["data"]["ip"] = WiFi.localIP().toString();
      doc["data"]["version"] = VERSION;

    }

    char output[256];
    serializeJson(doc, output);
    client.ping();
    client.send(output);

    websocket_backoff = 1000; // reset backoff time on successful connection
  } else {
    Serial.println("Websocket: connection failed.");
    if (config_target_system == "ch_racing_club") {
      websocket_backoff = min(websocket_backoff * 2, websocket_max_backoff); // Exponentielles Backoff
    }
    else {
      websocket_backoff = 3000; // set backoff time for SmartRace
    }
  }
}

void send_finish_line_message(int controller_id, unsigned long timestamp, String rfid_string) {
  int last_timestamp = rfids[controller_id].last;
  int lap_time = last_timestamp > 0 ? timestamp - last_timestamp : timestamp;

  // set last timestamp for the controller
  rfids[controller_id].last = timestamp;

  // build the message
  JsonDocument doc;
  if (config_target_system == "ch_racing_club") {
    doc["command"] = "lap";
    doc["data"]["api_key"] = config_ch_racing_club_api_key;
    doc["data"]["rfid"] = rfid_string;
    doc["data"]["duration"] = lap_time;
  } else {
    doc["type"] = "analog_lap";
    doc["data"]["timestamp"] = rfids[controller_id].last;
    doc["data"]["controller_id"] = controller_id + 1;
  }

  // send the message via websocket
  char output[256];
  serializeJson(doc, output);
  client.send(output);

  // print the message to the serial console
  Serial.print("Websocket: ");
  Serial.println(output);
}

void send_finish_line_event(String rfid_string, unsigned long ms) {
  bool found = false;
  for(int j = 0; j<RFID_STORAGE_COUNT;j++) {
    for (int i = 0; i < RFID_MAX_COUNT; i++) {
      if(rfids[i].id[j] == rfid_string) {
        if(rfids[i].last + config_min_lap_time < ms) {
          ledOn(RFID_LED_PIN);
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
    for(int i=0; i < RFID_MAX_COUNT; i++) {
      if(rfids[i].id[0] == "") {
        rfids[i].id[0] = rfid_string;
        Serial.print("RFID: new car at controller id: ");
        Serial.print(i+1);
        Serial.print(rfids[i].id[0]);
        Serial.println();
        ledOn(RFID_LED_PIN);
        send_finish_line_message(i, ms, rfid_string);
        break;
      }
    }
  }
}

void onMessageCallback(WebsocketsMessage message) {
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, message.data());
  if (error) {
    Serial.print("Websocket: JSON deserialization failed: ");
    Serial.println(error.c_str());
    return;
  }

  #ifdef DEBUG
    Serial.print("Websocket: received message: ");
    serializeJsonPretty(doc, Serial);
    Serial.println();
  #endif
}

void onEventsCallback(WebsocketsEvent event, String data) {
  if(event == WebsocketsEvent::ConnectionOpened) {
      Serial.println("Websocket: connected");
      websocket_connected = true;
      ledOn(WEBSOCKET_LED_PIN);
  } else if(event == WebsocketsEvent::ConnectionClosed) {
    Serial.println("Websocket: connection closed");
    websocket_connected = false;
    ledOff(WEBSOCKET_LED_PIN);
  } else if(event == WebsocketsEvent::GotPing) {
    #ifdef DEBUG
      Serial.println("Websocket: got a ping!");
    #endif
    client.pong();
  } else if(event == WebsocketsEvent::GotPong) {
    #ifdef DEBUG
      Serial.println("Websocket: got a pong!");
    #endif
  }
}

void wait(unsigned long waitTime) {
  unsigned long startWaitTime = millis();
  while((millis() - startWaitTime) < waitTime) {
    if (webserver_running) server.handleClient();
    else delay(1);
    client.poll();
  }
}

void resetRfidStorage() {
  char nameBuf[20];
  for(int i=0; i < RFID_MAX_COUNT; i++) {
    for(int j=0; j<RFID_STORAGE_COUNT; j++) {
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

void rfid_set_power_level(int config_rfid_power_level) {
  // Set power level based on loaded configuration
  bool ok;

  switch (config_rfid_power_level) {
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
    Serial.print(config_rfid_power_level);
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

  // set sensitivity mode
  rfidSetDensitivityMode(config_rfid_dense_mode);
  
  //set power level and start ReadMulti
  rfid_set_power_level(config_rfid_power_level);
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
  if (epcString != lastEpcString || (lastEpcRead + RFID_REPEAT_TIME) < now) {
    send_finish_line_event(epcString, now);
    lastEpcString = epcString;
    lastEpcRead = now;
  }
}

bool isLedOn(int led_pin) {
  #ifdef INVERT_LEDS
    if (digitalRead(led_pin) == HIGH) {
      return true;
    }
  #else
    if (digitalRead(led_pin) == LOW) {
      return true;
    }
  #endif
  return false;
}

void ledOn(int led_pin) {
  #ifdef INVERT_LEDS
    digitalWrite(led_pin, HIGH);
  #else
    digitalWrite(led_pin, LOW);
  #endif
  rfid_led_on_ms = millis();
}

void ledOff(int led_pin) {
  #ifdef INVERT_LEDS
    digitalWrite(led_pin, LOW);
  #else
    digitalWrite(led_pin, HIGH);
  #endif
}

void setup() {
  preferences.begin(PREFERENCES_NAMESPACE, false);

  pinMode(RFID_LED_PIN, OUTPUT);
  ledOff(RFID_LED_PIN);

  pinMode(WEBSOCKET_LED_PIN, OUTPUT);
  ledOff(WEBSOCKET_LED_PIN);

  pinMode(WIFI_AP_LED_PIN, OUTPUT);
  ledOff(WIFI_AP_LED_PIN);

  #ifdef PUSH_BUTTON_PIN
    pinMode(PUSH_BUTTON_PIN, INPUT_PULLUP);
  #endif

  //init rfid storage
  resetRfidStorage();
  delay(2000);

  Serial.begin(115200);
  wait(2000);

  Serial.print("RaceBox Version: ");
  Serial.println(VERSION);
  Serial.println("############################");

  configuration_load();

  Serial.println();
  Serial.println("Starting ...");

  if (config_wifi_ssid != "") {
    WiFi.setHostname(config_wifi_hostname.c_str());
    WiFi.begin(config_wifi_ssid.c_str(), config_wifi_password.c_str());

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < WIFI_CONNECT_ATTEMPTS) {
      wait(WIFI_CONNECT_DELAY_MS);
      Serial.print(".");
      attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.print("\nWiFi: connected ");
      Serial.println(WiFi.localIP());
      Serial.print("WiFi: Hostname: ");
      Serial.println(WiFi.getHostname());
      wifi_ap_mode = false;
      ledOff(WIFI_AP_LED_PIN);
    } else {
      Serial.println("\nWiFi: connection failed!");
      WiFi.softAP(WIFI_AP_SSID);
      dnsServer.start();
      Serial.println("WiFi: started AP mode");
      wifi_ap_mode = true;
      ledOn(WIFI_AP_LED_PIN);
    }
  } else {
    WiFi.softAP(WIFI_AP_SSID);
    dnsServer.start();
    Serial.println("WiFi: started AP mode");
    wifi_ap_mode = true;
    ledOn(WIFI_AP_LED_PIN);
  }

  server.on("/", handleRoot);
  server.on("/config", HTTP_POST, handleConfig);
  // all unknown pages are redirected to configuration page
  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("Webserver: running");
  webserver_running = true;

  // Start RFID reader
  initRfid();
}

void loop() {
  server.handleClient();
  readRfid();
  client.poll();
  if(websocket_connected) {
    if(millis() > (websocket_last_ping + WEBSOCKET_PING_INTERVAL)) {
      websocket_last_ping = millis();
      client.ping();
    }
  }
  else {
    if(!wifi_ap_mode) connectWebsocket();
  }
  if(isLedOn(RFID_LED_PIN) && (rfid_led_on_ms + RFID_LED_ON_TIME) < millis()) {
    ledOff(RFID_LED_PIN);
  }
  #ifdef PUSH_BUTTON_LED_PIN
    if(digitalRead(PUSH_BUTTON_PIN) == LOW) {
      resetRfidStorage();
      while(digitalRead(PUSH_BUTTON_PIN) == LOW) {
        ledOn(RFID_LED_PIN);
        wait(100);
      }
      ledOff(RFID_LED_PIN);
    }
  #endif
}
