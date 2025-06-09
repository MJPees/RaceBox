#include "config.h"
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <ArduinoWebsockets.h> //ArduinoWebsockets 0.5.4
#include <ArduinoJson.h> // Tested V0.9.5 https://github.com/schnoog/Joystick_ESP32S2
#include "src/Joystick_BLE/Joystick_BLE.h"

/* configuration */
#define WIFI_AP_SSID "RaceBox-GhostCar-Config"
#define PREFERENCES_NAMESPACE "racebox"

Joystick_BLE_ Joystick_BLE;

#ifndef ESP32C3
  #include <Joystick_ESP32S2.h>
  Joystick_ Joystick(JOYSTICK_DEFAULT_REPORT_ID,JOYSTICK_TYPE_GAMEPAD,
                   14, 2,true, false, false, false, false,
                   false,false, false, true, true, false);
#endif

#define WIFI_CONNECT_ATTEMPTS 20
#define WIFI_CONNECT_DELAY_MS 500
#define WEBSOCKET_PING_INTERVAL 5000


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

bool activeLaunchControl = false;
bool isBraking = false;
bool isDriving = false;
unsigned long brakeTime = 0;
unsigned long lastJoystickUpdate = 0;

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

int speed = DEFAULT_SPEED; // ghostcar speed %

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

  preferences.putInt("speed", speed);
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
  speed = preferences.getInt("speed", DEFAULT_SPEED);

  if (config_target_system == "smart_race") {
    websocket_server = config_smart_race_websocket_server;
    websocket_ca_cert = config_smart_race_websocket_ca_cert;
    #ifdef ESP32C3
      Serial.println("\nConfiguration: SmartRace loaded");
    #endif
  }

  if (config_target_system == "ch_racing_club") {
    websocket_server = config_ch_racing_club_websocket_server;
    websocket_ca_cert = config_ch_racing_club_websocket_ca_cert;
    #ifdef ESP32C3
      Serial.println("\nConfiguration: CH Racing Club loaded");
    #endif
  }
}

void handleNotFound() {
  server.sendHeader("Location", "/");
  server.send(302, "text/plain", "redirect to configuration page");
}

void handleRoot() {
  String html = "<!DOCTYPE html><html><head><title>RaceBox-GhostCar</title>";
  html += "<meta charset='UTF-8'>"; // Specify UTF-8 encoding
  html += "<style>*, *::before, *::after {box-sizing: border-box;}body {min-height: 100vh;margin: 0;}form {max-width: 535px;margin: 0 auto;}label {margin-bottom: 5px;display:block;}input[type=text],input[type=password],input[type=number],select,textarea {width: 100%;padding: 8px;border: 1px solid #ccc;border-radius: 4px;display: block;}input[type=submit] {width: 100%;background-color: #4CAF50;color: white;padding: 10px 15px;border: none;border-radius: 4px;}</style>";
  html += "<script src='https://unpkg.com/alpinejs' defer></script>";
  html += "</head><body>";
  html += "<form x-data=\"{ targetSystem: '" + config_target_system + "', smartRaceWebsocketServer: '" + config_smart_race_websocket_server + "', racingClubWebsocketServer: '" + config_ch_racing_club_websocket_server + "' }\" action='/config' method='POST'>";
  html += "<h1 align=center>RaceBox-GhostCar</h1>";

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
    html += "<label for='speed'>GhostCar speed (%):</label>";
    html += "<input type='number' id='speed' name='speed' value='" + String(speed) + "'><br>";
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
      speed = server.arg("speed").toInt();
      if (speed > 100) {
        speed = 100;
      }
      else if (speed < 10) {
        speed = 10;
      }
      if(isDriving) {
        Joystick_BLE.setAccelerator(speed);
        Joystick_BLE.setAccelerator(speed);
        Joystick_BLE.sendState();
        #ifndef ESP32C3
          Joystick.setAccelerator(speed);
          Joystick.setAccelerator(speed);
          Joystick.sendState();
        #endif
      }
    }
    configuration_save();
    server.send(200, "text/html", "<!DOCTYPE html><html><head><title>RaceBox-GhostCar</title></head><body><h1>Configuration saved!</h1><p>You will be redirected in 2 seconds.</p><script>setTimeout(function() { window.location.href = 'http://" + config_wifi_hostname + "'; }, 2000);</script></body></html>");
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
  } else {
    server.send(200, "text/html", "<!DOCTYPE html><html><head><title>RaceBox-GhostCar</title></head><body><h1>Invalid request!</h1><p>You will be redirected in 2 seconds.</p><script>setTimeout(function() { window.location.href = 'http://" + config_wifi_hostname + "'; }, 2000);</script></body></html>");
  }
}

void wifi_reload() {
  #ifdef ESP32C3
    Serial.println("WiFi: Initiating reload...");
  #endif
  if (WiFi.getMode() != WIFI_OFF) {
    #ifdef ESP32C3
      Serial.println("WiFi: Disconnecting existing connections...");
    #endif
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
  #ifdef ESP32C3
    Serial.print("WiFi: Attempting to connect to AP '");
    Serial.print(config_wifi_ssid);
    Serial.println("'...");
  #endif
  WiFi.mode(WIFI_STA);
  WiFi.begin(config_wifi_ssid.c_str(), config_wifi_password.c_str());

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < WIFI_CONNECT_ATTEMPTS) {
    wait(WIFI_CONNECT_DELAY_MS);
    #ifdef ESP32C3
      Serial.print(".");
    #endif
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    #ifdef ESP32C3
      Serial.print("\nWiFi: connected ");
      Serial.println(WiFi.localIP());
      Serial.print("WiFi: Hostname: ");
      Serial.println(WiFi.getHostname());
    #endif
    wifi_ap_mode = false;
  } else {
    #ifdef ESP32C3
      Serial.println("\nWiFi: connect failed, starting AP mode");
    #endif
    WiFi.disconnect(true);
    wait(500);
    WiFi.softAP(WIFI_AP_SSID);
    dnsServer.start();
    wifi_ap_mode = true;
    #ifdef ESP32C3
      Serial.print("\nWiFi: AP started, IP: ");
      Serial.println(WiFi.softAPIP());
    #endif
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
  #ifdef ESP32C3
    Serial.print("Websocket: connecting ... ");
    Serial.println(websocket_server);
  #endif

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

    if(config_target_system == "ch_racing_club") {
      doc["command"] = "ghostcar_connect";
      doc["data"]["api_key"] = config_ch_racing_club_api_key;
      doc["data"]["ip"] = WiFi.localIP().toString();
      // doc["data"]["version"] = VERSION;
    }

    char output[256];
    serializeJson(doc, output);
    client.ping();
    client.send(output);
    #ifdef ESP32C3
      Serial.println("Websocket: connected.");
    #endif
    websocket_backoff = 1000; // reset backoff time on successful connection
  } else {
    #ifdef ESP32C3
      Serial.println("Websocket: connection failed.");
    #endif
    if (config_target_system == "ch_racing_club") {
      websocket_backoff = min(websocket_backoff * 2, websocket_max_backoff); // Exponentielles Backoff
    }
    else {
      websocket_backoff = 3000; // set backoff time for SmartRace
    }
  }
}

void onMessageCallback(WebsocketsMessage message) {
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, message.data());
  if (error) {
    #ifdef ESP32C3
      Serial.print("Websocket: JSON deserialization failed: ");
      Serial.println(error.c_str());
    #endif
    return;
  }

  #if defined(DEBUG) && defined(ESP32C3)
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
      #ifdef ESP32C3
        Serial.println("Received message without 'command' key: " + payload.as<String>());
      #endif
    }
  } else {
    if (doc.containsKey("type")) {
      handleSmartRaceUpdateEvent(doc["type"].as<String>(), doc);
    } else {
      #ifdef ESP32C3
        Serial.println("Received message without 'type' key: " + message.data());
      #endif
    }
  }
}

void handleRacingClubUpdateEvent(String command, JsonDocument doc) {
  if (doc["api_key"].as<String>() != config_ch_racing_club_api_key) {
    #if defined(DEBUG) && defined(ESP32C3)
      Serial.println("INFO - received message with invalid API key: " + api_key);
    #endif

    return;
  }

  if (command == "launchcontrol") {
    #if defined(DEBUG) && defined(ESP32C3)
      Serial.println("INFO - starting");
    #endif
    activateLaunchControl();
  } else if (command == "drive") {
    #if defined(DEBUG) && defined(ESP32C3)
      Serial.println("INFO - running");
    #endif
    drive();
  } else if (command == "stop") {
    #if defined(DEBUG) && defined(ESP32C3)
      Serial.println("INFO - stop");
    #endif
    stop();
  }
}

void handleSmartRaceUpdateEvent(String type, JsonDocument doc) {
  if (type == "update_event_status" && doc.containsKey("data")) {
    String data = doc["data"].as<String>();
    if (data == "prepare_for_start") {
      #if defined(DEBUG) && defined(ESP32C3)
        Serial.println("INFO - prepare_for_start");
      #endif
    } else if (data == "starting") {
      #if defined(DEBUG) && defined(ESP32C3)
        Serial.println("INFO - starting");
      #endif
      activateLaunchControl();
    } else if (data == "running") {
      #if defined(DEBUG) && defined(ESP32C3)
        Serial.println("INFO - running");
      #endif
      drive();
    } else if (data == "suspended") {
      #if defined(DEBUG) && defined(ESP32C3)
        Serial.println("INFO - suspended");
      #endif
      stop();
    } else if (data == "ended") {
      #if defined(DEBUG) && defined(ESP32C3)
        Serial.println("INFO - ended");
      #endif
      stop();
    } else {
      #if defined(DEBUG) && defined(ESP32C3)
        Serial.println("Unknown message type: " + type);
      #endif
    }
  } else if (type == "reset") {
    #if defined(DEBUG) && defined(ESP32C3)
      Serial.println("INFO - reset");
    #endif
    stop();
  } else {
    #if defined(DEBUG) && defined(ESP32C3)
      Serial.println("Unknown message type: " + type);
    #endif
  }
}

void onEventsCallback(WebsocketsEvent event, String data) {
  if(event == WebsocketsEvent::ConnectionOpened) {
    #ifdef ESP32C3
      Serial.println("Websocket: connnected");
    #endif
    #ifdef RGB_LED
      rgbLedWrite(RGB_LED_PIN, 200, 200, 200);
    #endif
    #ifdef LED_PIN
      digitalWrite(LED_PIN, LOW);
    #endif
    websocket_connected = true;
  } else if(event == WebsocketsEvent::ConnectionClosed) {
    #ifdef ESP32C3
      Serial.println("Websocket: connection closed");
    #endif
    #ifdef RGB_LED
      rgbLedWrite(RGB_LED_PIN, 0, 0, 255);
    #endif
    #ifdef LED_PIN
      digitalWrite(LED_PIN, HIGH);
    #endif
    websocket_connected = false;
  } else if(event == WebsocketsEvent::GotPing) {
     #if defined(DEBUG) && defined(ESP32C3)
      Serial.println("Websocket: got a ping!");
    #endif
    client.pong();
  } else if(event == WebsocketsEvent::GotPong) {
     #if defined(DEBUG) && defined(ESP32C3)
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

void activateLaunchControl() {
   #if defined(DEBUG) && defined(ESP32C3)
    Serial.println("INFO - activate launch control");
  #endif
  isDriving = false;
  activeLaunchControl = true;
  isBraking = false;
  resetJoystickPosition();
  Joystick_BLE.setButton(KEY_TURBO, 1);
  Joystick_BLE.setAccelerator(speed);
  Joystick_BLE.sendState();
  #ifndef ESP32C3
    Joystick.setButton(KEY_TURBO, 1);
    Joystick.setAccelerator(speed);
    Joystick.sendState();
  #endif
  #ifdef RGB_LED
    rgbLedWrite(RGB_LED_PIN, 255, 255, 0);
  #endif
}

void drive() {
  #if defined(DEBUG) && defined(ESP32C3)
    Serial.println("INFO - ghostcar driving");
  #endif
  isBraking = false;
  if(activeLaunchControl) {
    Joystick_BLE.setBrake(0);
    Joystick_BLE.setButton(KEY_TURBO, 0);
    #ifndef ESP32C3
      Joystick.setBrake(0);
      Joystick.setButton(KEY_TURBO, 0);
    #endif
  }
  else {
    resetJoystickPosition();
    Joystick_BLE.setAccelerator(speed);
    #ifndef ESP32C3
      Joystick.setAccelerator(speed);
    #endif
  }
  Joystick_BLE.sendState();
  #ifndef ESP32C3
    Joystick.sendState();
  #endif
  activeLaunchControl = false;
  isDriving = true;
  #ifdef RGB_LED
    rgbLedWrite(RGB_LED_PIN, 255, 0, 0);
  #endif
}

void stop() {
  isDriving = false;
  #if defined(DEBUG) && defined(ESP32C3)
    Serial.println("INFO - ghostcar stopped");
  #endif
  resetJoystickPosition();
  Joystick_BLE.setBrake(100);
  Joystick_BLE.sendState();
  #ifndef ESP32C3
    Joystick.setBrake(100);
    Joystick.sendState();
  #endif
  brakeTime = millis();
  isBraking = true;
  activeLaunchControl = false;
  #ifdef RGB_LED
    rgbLedWrite(RGB_LED_PIN, 0, 255, 0);
  #endif
}

void initializeJoystickMode() {
  #ifdef ESP32S3
    USB.PID(0x8211);
    USB.VID(0x303b);
    USB.productName("RaceBox-GhostCar");
    USB.manufacturerName("CH-Community");
    USB.begin();
    delay(100);
  #endif
  Joystick_BLE.setXAxisRange(-100, 100);
  Joystick_BLE.setAcceleratorRange(0, 100);
  Joystick_BLE.setBrakeRange(0, 100);
  Joystick_BLE.begin(false);
  #ifndef ESP32C3
    Joystick.setXAxisRange(-100, 100);
    Joystick.setAcceleratorRange(0, 100);
    Joystick.setBrakeRange(0, 100);
    Joystick.begin(false);
  #endif
  wait(500);
  resetJoystickPosition();
}

void resetJoystickPosition() {
  Joystick_BLE.setXAxis(0L);
  Joystick_BLE.setAccelerator(0L);
  Joystick_BLE.setBrake(0L);
  Joystick_BLE.setButton(KEY_TURBO, 0L);
  Joystick_BLE.sendState();
  #ifndef ESP32C3
    Joystick.setXAxis(0L);
    Joystick.setAccelerator(0L);
    Joystick.setBrake(0L);
    Joystick.setButton(KEY_TURBO, 0L);
    Joystick.sendState();
  #endif
  isDriving = false;
  isBraking = false;
}

void setup() {
  preferences.begin(PREFERENCES_NAMESPACE, false);

  #ifdef ESP32C3
    Serial.begin(115200);
    wait(2000);
    Serial.print("RaceBox-GhostCar Version: ");
    Serial.print(VERSION);
    Serial.println(" started.");
    Serial.println("############################");
  #endif
  configuration_load();

  #ifdef ESP32C3
    Serial.println();
    Serial.print("Starting ...");
  #endif

  if (config_wifi_ssid != "") {
    WiFi.setHostname(config_wifi_hostname.c_str());
    WiFi.begin(config_wifi_ssid.c_str(), config_wifi_password.c_str());

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < WIFI_CONNECT_ATTEMPTS) {
      wait(WIFI_CONNECT_DELAY_MS);
      #ifdef ESP32C3
        Serial.print(".");
      #endif
      attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
      #ifdef ESP32C3
        Serial.print("\nWiFi:  connected ");
        Serial.println(WiFi.localIP());
        Serial.print("WiFi: Hostname: ");
        Serial.println(WiFi.getHostname());
      #endif
      wifi_ap_mode = false;
    } else {
      #ifdef ESP32C3
        Serial.println("\nWiFi:  connection failed!");
      #endif
      WiFi.softAP(WIFI_AP_SSID);
      dnsServer.start();
      #ifdef ESP32C3
        Serial.println("WiFi: started AP mode");
      #endif
      wifi_ap_mode = true;
    }
  } else {
    WiFi.softAP(WIFI_AP_SSID);
    dnsServer.start();
    #ifdef ESP32C3
      Serial.println("WiFi: started AP mod");
    #endif
    wifi_ap_mode = true;
  }

  #ifdef RGB_LED
    pinMode(RGB_LED_PIN, OUTPUT);
    rgbLedWrite(RGB_LED_PIN, 0, 0, 255);
  #endif
  #ifdef LED_PIN
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
    wait(500);
    digitalWrite(LED_PIN, HIGH);
  #endif

  initializeJoystickMode();

  server.on("/", handleRoot);
  server.on("/config", HTTP_POST, handleConfig);
  // all unknown pages are redirected to configuration page
  server.onNotFound(handleNotFound);

  server.begin();
  #ifdef ESP32C3
    Serial.println("Webserver: running");
  #endif
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
    if(millis() > (lastJoystickUpdate + JOYSTICK_UPDATE_INTERVAL)) {
      lastJoystickUpdate = millis();
      Joystick_BLE.sendState();
      #ifndef ESP32C3
        Joystick.sendState();
      #endif
    }
    if(isBraking && (millis() > (brakeTime + DEFAULT_BRAKING_TIME))) {
      isBraking = false;
      Joystick_BLE.setBrake(0);
      Joystick_BLE.sendState();
      #ifndef ESP32C3
        Joystick.setBrake(0);
        Joystick.sendState();
      #endif
      #ifdef RGB_LED
        rgbLedWrite(RGB_LED_PIN, 200, 200, 200);
      #endif
    }
  }
  else {
    if(!wifi_ap_mode) connectWebsocket();
  }
}
