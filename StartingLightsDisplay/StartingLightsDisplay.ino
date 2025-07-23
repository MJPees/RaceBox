#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <ArduinoWebsockets.h> //ArduinoWebsockets 0.5.4
#include <ArduinoJson.h>

#include "src/SD_Card.h"
#include "src/Display_ST7789.h"
#include "src/LCD_Image.h"

/* configuration */
#define WIFI_AP_SSID "StartingLights-Config"
#define WIFI_DEFAULT_HOSTNAME "starting-lights"
#define PREFERENCES_NAMESPACE "racebox"

#define VERSION "1.0.3" // dont forget to update the releases.json
//#define DEBUG

#define WIFI_CONNECT_ATTEMPTS 10
#define WIFI_CONNECT_DELAY_MS 1000
#define WEBSOCKET_PING_INTERVAL 5000

#define RGB_LED_PIN 8 // Pin für RGB-LED

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

String currentImage = "";
String requestedImage = "/finish_flag.png";  // soll angezeigt werden
void displayImage(const char* imagePath, bool forceUpdate = false);

String currentStatus = "idle";
unsigned long idleStartTime = 0;

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

// display backlight control
int backlightLevel = 80;                     // Initialer Helligkeitswert
int lastButtonState = HIGH;                  // Vorheriger Tasterzustand
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 200;     // Entprellzeit in ms

void configuration_save() {
  preferences.putString("target_system", config_target_system);
  preferences.putString("wifi_ssid", config_wifi_ssid);
  preferences.putString("wifi_password", config_wifi_password);
  preferences.putString("wifi_hostname", config_wifi_hostname);

  preferences.putString("sr_ws_server", config_smart_race_websocket_server);
  preferences.putString("sr_ws_ca_cert", config_smart_race_websocket_ca_cert);

  preferences.putString("chrc_ws_server", config_ch_racing_club_websocket_server);
  preferences.putString("chrc_ws_ca_cert", config_ch_racing_club_websocket_ca_cert);
  preferences.putString("chrc_api_key", config_ch_racing_club_api_key);
}

void configuration_load() {
  config_target_system = preferences.getString("target_system", "smart_race");

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

    Serial.println("\nConfiguration: StartingLights loaded");
  }

  if (config_target_system == "ch_racing_club") {
    websocket_server = config_ch_racing_club_websocket_server;
    websocket_ca_cert = config_ch_racing_club_websocket_ca_cert;

    Serial.println("\nConfiguration: CH Racing Club loaded");
  }
}

void handleNotFound() {
  server.sendHeader("Location", "/");
  server.send(302, "text/plain", "redirect to configuration page");
}

void handleRoot() {
  String html = "<!DOCTYPE html><html><head><title>StartingLights</title>";
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
  html += "<h1 align=center>StartingLights</h1>";

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
    html += "</div><br>";
  }

  html += "<input type='submit' style='margin-bottom:20px;' value='Speichern'>";
  html += "</form>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}

void handleConfig() {
  if (server.args() > 0) {
    bool reConnectWifi = config_wifi_ssid != server.arg("config_wifi_ssid") || config_wifi_password != server.arg("config_wifi_password") || config_wifi_hostname != server.arg("config_wifi_hostname");
    bool reConnectWebsocket = false;

    config_wifi_ssid = server.arg("config_wifi_ssid");
    config_wifi_password = server.arg("config_wifi_password");
    config_wifi_hostname = server.arg("config_wifi_hostname");

    if(!wifi_ap_mode) {
      reConnectWebsocket = config_target_system != server.arg("config_target_system");

      config_target_system = server.arg("config_target_system");
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
    }

    configuration_save();

    server.send(200, "text/html", "<!DOCTYPE html><html><head><title>StartingLights</title></head><body><h1>Configuration saved!</h1><p>You will be redirected in 2 seconds.</p><script>setTimeout(function() { window.location.href = 'http://" + config_wifi_hostname + "'; }, 2000);</script></body></html>");
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
    Serial.println("RFID: started ReadMulti.");
  } else {
    server.send(200, "text/html", "<!DOCTYPE html><html><head><title>StartingLights</title></head><body><h1>Invalid request!</h1><p>You will be redirected in 2 seconds.</p><script>setTimeout(function() { window.location.href = 'http://" + config_wifi_hostname + "'; }, 2000);</script></body></html>");
  }
}

void wifi_reload() {
  Serial.println("WiFi: Initiating reload...");

  displayImage("/connect_wifi.png");

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
    waitForWifi(WIFI_CONNECT_DELAY_MS);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("\nWiFi: connected ");
    Serial.println(WiFi.localIP());
    Serial.print("WiFi: Hostname: ");
    Serial.println(WiFi.getHostname());
    wifi_ap_mode = false;
  } else {
    Serial.println("\nWiFi: connect failed, starting AP mode");
    WiFi.disconnect(true);
    wait(500);
    WiFi.softAP(WIFI_AP_SSID);
    dnsServer.start();
    wifi_ap_mode = true;
    Serial.print("\nWiFi: AP started, IP: ");
    Serial.println(WiFi.softAPIP());
    displayImage("/ap_mode.png", true); // red LED on for AP mode
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
      doc["data"]["controller_id"] = "1";
    }
    else if(config_target_system == "ch_racing_club") {
      doc["command"] = "starting_light_connect";
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

    displayImage("/carrera_hybrid.png");
  } else {
    Serial.println("Websocket: connection failed.");
    if (config_target_system == "ch_racing_club") {
      websocket_backoff = min(websocket_backoff * 2, websocket_max_backoff); // Exponentielles Backoff
    }
    else {
      websocket_backoff = 3000; // set backoff time for SmartRace
    }

    displayImage("/connect_websocket.png");
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

  if (config_target_system == "ch_racing_club") {
    JsonDocument payload;
    DeserializationError error = deserializeJson(payload, doc["message"].as<String>());

    //uses samge message format as SmartRace so far...
    if (payload.containsKey("command")) {
      handleRacingClubUpdateEvent(payload["command"].as<String>(), payload);
    } else {
      #ifdef DEBUG
        Serial.println("Received message without 'command' key: " + payload.as<String>());
      #endif
    }
  } else {
    if (doc.containsKey("type")) {
      handleSmartRaceUpdateEvent(doc["type"].as<String>(), doc);
    } else {
      #ifdef DEBUG
        Serial.println("Received message without 'type' key: " + message.data());
      #endif
    }
  }
}


void handleRacingClubUpdateEvent(String command, JsonDocument doc) {
  String api_key = doc["api_key"].as<String>();
  String status = doc["status"].as<String>();

  // received message with invalid API key
  if (api_key != config_ch_racing_club_api_key) {
    return;
  }

  // set status
  currentStatus = status;

  if (command == "launchcontrol") { // starting
    displayImage("/off.png");
    return;
  }

  if (command == "drive") { // running
    displayImage("/green_5.png");
    return;
  }

  if (command == "stop") { // stop
    if (status == "ended") {
      stopRace();
    } else if (status == "finished") {
      finishRace();
    } else if (status == "suspended") {
      yellowFlag();
    } else if (status == "false_start") {
      falseStart();
    } else if (status == "idle") {
      displayImage("/carrera_hybrid.png", true);
    } else {
      displayImage("/off.png");
    }
  }

  // handle starting lights
  if (command == "starting_lights") {
    String imagePath = "/red_" + doc["count"].as<String>() + ".png";
    displayImage(imagePath.c_str());
    return;
  }
}

void handleSmartRaceUpdateEvent(String type, JsonDocument doc) {
  if (type == "update_event_status" && doc.containsKey("data")) {
    String data = doc["data"].as<String>();
    currentStatus = data;
    if (data == "prepare_for_start") {
      displayImage("/off.png");
    } else if (data == "starting") {
      wait(1000);
      for(int i=1; i < 6; i++) {
        String imagePath = "/red_" + String(i) + ".png";
        displayImage(imagePath.c_str());
        wait(900);
      }
    } else if (data == "running") {
      displayImage("/green_5.png");
    } else if (data == "suspended") {
      redFlag();
    } else if (data == "ended") {
      finishRace();
    } else {
      displayImage("/off.png");
    }
  } else if(type == "update_vsc_status") {
    String data = doc["data"].as<String>();
    if (data == "active") {
      currentStatus = "suspended";
      yellowFlag();
    } else if (data == "off") {
      currentStatus = "running";
      displayImage("/green_5.png");
    }
  } else if (type == "reset") {
    displayImage("/carrera_hybrid.png");
    currentStatus = "idle";
  } else {
    #ifdef DEBUG
      Serial.println("Unknown message type: " + type);
    #endif
  }
}


void onEventsCallback(WebsocketsEvent event, String data) {
  if(event == WebsocketsEvent::ConnectionOpened) {
      Serial.println("Websocket: connected");
      websocket_connected = true;
      displayImage("/carrera_hybrid.png");
  } else if(event == WebsocketsEvent::ConnectionClosed) {
    Serial.println("Websocket: connection closed");
    websocket_connected = false;
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


void falseStart() {
  for (int i = 0; i < 2; ++i) {
    displayImage("/red_5.png", true);
    wait(300);
    displayImage("/off.png", true);
    wait(300);
  }
  displayImage("/false_start.png");
}

void stopRace() {
  displayImage("/red_5.png");
}

void yellowFlag() {
  for (int i = 0; i < 4; ++i) {
    if(currentStatus != "suspended") return;
    displayImage("/yellow_5.png", true);
    wait(300);
    if(currentStatus != "suspended") return;
    displayImage("/off.png", true);
    wait(300);
  }
  if(currentStatus != "suspended") return;
  displayImage("/yellow_5.png");
}

void redFlag() {
  for (int i = 0; i < 4; ++i) {
    if(currentStatus != "suspended") return;
    displayImage("/red_5.png", true);
    wait(300);
    if(currentStatus != "suspended") return;
    displayImage("/off.png", true);
    wait(300);
  }
  if(currentStatus != "suspended") return;
  displayImage("/red_5.png");
}

void finishRace() {
  displayImage("/finish_flag.png", true);
  wait(1000);
  displayImage("/finish.png");
}

void displayImage(const char* imagePath, bool forceUpdate) {
  if (imagePath != nullptr && imagePath[0] != '\0') {
    if (forceUpdate || requestedImage != imagePath) {
      Show_Image(imagePath);
    }

    requestedImage = imagePath;

    if (strcmp(imagePath, "/carrera_hybrid.png") == 0) {
      rgbLedWrite(RGB_LED_PIN, 0, 0, 255); // blue LED for Carrera Hybrid
    } else if (strcmp(imagePath, "/off.png") == 0) {
      rgbLedWrite(RGB_LED_PIN, 0, 0, 0); // off
    } else if (
      strcmp(imagePath, "/red_1.png") == 0 ||
      strcmp(imagePath, "/red_2.png") == 0 ||
      strcmp(imagePath, "/red_3.png") == 0 ||
      strcmp(imagePath, "/red_4.png") == 0 ||
      strcmp(imagePath, "/red_5.png") == 0
    ) {
      rgbLedWrite(RGB_LED_PIN, 0, 255, 0); // red LED for red lights
    } else if (strcmp(imagePath, "/green_5.png") == 0) {
      rgbLedWrite(RGB_LED_PIN, 255, 0, 0); // green LED for green lights
    } else if (strcmp(imagePath, "/yellow_5.png") == 0) {
      rgbLedWrite(RGB_LED_PIN, 207, 255, 0); // yellow LED for yellow lights
    } else if (
      strcmp(imagePath, "/finish_flag.png") == 0 ||
      strcmp(imagePath, "/finish.png") == 0
    ) {
      rgbLedWrite(RGB_LED_PIN, 255, 255, 255); // white LED for finish
    } else if (strcmp(imagePath, "/connect_wifi.png") == 0) {
      rgbLedWrite(RGB_LED_PIN, 255, 0, 0); // green LED for WiFi connection
    } else if (strcmp(imagePath, "/connect_websocket.png") == 0) {
      rgbLedWrite(RGB_LED_PIN, 0, 0, 255); // blue LED for websocket connection
    } else if (strcmp(imagePath, "/ap_mode.png") == 0) {
      rgbLedWrite(RGB_LED_PIN, 0, 255, 0); // red LED for AP mode
    } else {
      rgbLedWrite(RGB_LED_PIN, 0, 0, 0); // off for unknown images
    }

  } else {
    Serial.println("Error: Invalid image path provided.");
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

void waitForWifi(unsigned long waitTime) {
  unsigned long startWaitTime = millis();
  while((millis() - startWaitTime) < waitTime) {
    if(WiFi.status() == WL_CONNECTED) return; // exit if WiFi is connected
    delay(10);
  }
}

void setup() {
  Serial.begin(115200);
  wait(2000);

  pinMode(BOOT_KEY_PIN, INPUT_PULLUP);
  pinMode(RGB_LED_PIN, OUTPUT);

  LCD_Init();
  SD_Init();

  // Set initial backlight level
  Set_Backlight(backlightLevel);

  // white LED on for startup indication
  rgbLedWrite(RGB_LED_PIN, 255, 255, 255);

  preferences.begin(PREFERENCES_NAMESPACE, false);
  configuration_load();

  if (config_target_system == "smart_race") {
    displayImage("/system_smartrace.png", true);
  } else {
    displayImage("/system_racingclub.png", true);
  }

  wait(2000);

  Serial.print("StartingLights Version: ");
  Serial.println(VERSION);
  Serial.println("############################");

  Serial.println();
  Serial.println("Starting ...");

  if (config_wifi_ssid != "") {
    WiFi.setHostname(config_wifi_hostname.c_str());
    WiFi.begin(config_wifi_ssid.c_str(), config_wifi_password.c_str());

    displayImage("/connect_wifi.png", true);

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < WIFI_CONNECT_ATTEMPTS) {
      waitForWifi(WIFI_CONNECT_DELAY_MS);
      Serial.print(".");
      attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.print("\nWiFi: connected ");
      Serial.println(WiFi.localIP());
      Serial.print("WiFi: Hostname: ");
      Serial.println(WiFi.getHostname());
      wifi_ap_mode = false;
    } else {
      WiFi.disconnect(true);
      wait(500);
      Serial.println("\nWiFi: connection failed!");
      WiFi.softAP(WIFI_AP_SSID);
      dnsServer.start();
      Serial.println("WiFi: started AP mode");
      wifi_ap_mode = true;
      displayImage("/ap_mode.png", true);
    }
  } else {
    WiFi.softAP(WIFI_AP_SSID);
    dnsServer.start();
    Serial.println("WiFi: started AP mode");
    wifi_ap_mode = true;
    displayImage("/ap_mode.png", true);
  }

  server.on("/", handleRoot);
  server.on("/config", HTTP_POST, handleConfig);
  // all unknown pages are redirected to configuration page
  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("Webserver: running");
  webserver_running = true;
}

void loop() {
  server.handleClient();
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

  // --- if state idle, show sponsoring after 30 seconds ---
  if (websocket_connected && currentStatus == "idle") {
    if (idleStartTime == 0) {
      idleStartTime = millis();
    }

    if (millis() - idleStartTime >= 30000) {
      Image_Next_Loop("/sponsor", ".png", 3000, RGB_LED_PIN);
    }
  } else {
    idleStartTime = 0;
  }

  // --- Helligkeit regeln bei Tastendruck ---
  int reading = digitalRead(BOOT_KEY_PIN);
  if (reading == LOW && lastButtonState == HIGH && (millis() - lastDebounceTime) > debounceDelay) {
    backlightLevel += 10;
    if (backlightLevel > 100) backlightLevel = 10; // Von 100 auf 10 zurückspringen
    Set_Backlight(backlightLevel);

    lastDebounceTime = millis();
  }

  // Bild nur bei Änderung anzeigen
  if (requestedImage != currentImage) {
    Show_Image(requestedImage.c_str());
    currentImage = requestedImage;
  }

  lastButtonState = reading;
  delay(10);
}
