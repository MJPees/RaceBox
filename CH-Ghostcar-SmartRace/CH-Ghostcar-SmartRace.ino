#include "config.h"
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <ArduinoWebsockets.h> //ArduinoWebsockets 0.5.4
#include <ArduinoJson.h>

/* configuration */
#define WIFI_AP_SSID "CH-GhostCar-SmartRace-Config"
#define PREFERENCES_NAMESPACE "smartRace"

#if defined(ESP32_BLE)
  #include "src/Joystick_BLE/Joystick_BLE.h"
  Joystick_ Joystick;
#else
  #include <Joystick_ESP32S2.h> // Tested V0.9.5 https://github.com/schnoog/Joystick_ESP32S2
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

    Serial.println("\nConfiguration: SmartRace loaded");
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
  String html = "<!DOCTYPE html><html><head><title>CH-GhostCar-SmartRace</title>";
  html += "<meta charset='UTF-8'>"; // Specify UTF-8 encoding
  html += "<style>*, *::before, *::after {box-sizing: border-box;}body {min-height: 100vh;margin: 0;}form {max-width: 535px;margin: 0 auto;}label {margin-bottom: 5px;display:block;}input[type=text],input[type=password],input[type=number],select,textarea {width: 100%;padding: 8px;border: 1px solid #ccc;border-radius: 4px;display: block;}input[type=submit] {width: 100%;background-color: #4CAF50;color: white;padding: 10px 15px;border: none;border-radius: 4px;}</style>";
  html += "<script src='https://unpkg.com/alpinejs' defer></script>";
  html += "</head><body>";
  html += "<form x-data=\"{ targetSystem: '" + config_target_system + "', smartRaceWebsocketServer: '" + config_smart_race_websocket_server + "', racingClubWebsocketServer: '" + config_ch_racing_club_websocket_server + "' }\" action='/config' method='POST'>";
  html += "<h1 align=center>CH-GhostCar-SmartRace</h1>";

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
        config_smart_race_websocket_server = server.arg("config_smart_race_websocket_server");
        config_smart_race_websocket_ca_cert = server.arg("config_smart_race_websocket_ca_cert");
        config_smart_race_websocket_ca_cert.replace("\r", "");

        websocket_server = config_smart_race_websocket_server;
        websocket_ca_cert = config_smart_race_websocket_ca_cert;
      }

      if (config_target_system == "ch_racing_club") {
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
        Joystick.setAccelerator(speed);
        Joystick.sendState();
      }
    }
    configuration_save();
    server.send(200, "text/html", "<!DOCTYPE html><html><head><title>CH-GhostCar-SmartRace</title></head><body><h1>Configuration saved!</h1><p>You will be redirected in 2 seconds.</p><script>setTimeout(function() { window.location.href = 'http://" + config_wifi_hostname + "'; }, 2000);</script></body></html>");
    wait(500);
    if (reConnectWebsocket) {
      websocket_connected = false;
      websocket_last_attempt = 0;
      connectWebsocket();
    }

    if(reConnectWifi) {
      wifi_reload();
    }
  } else {
    server.send(200, "text/html", "<!DOCTYPE html><html><head><title>CH-GhostCar-SmartRace</title></head><body><h1>Invalid request!</h1><p>You will be redirected in 2 seconds.</p><script>setTimeout(function() { window.location.href = 'http://" + config_wifi_hostname + "'; }, 2000);</script></body></html>");
  }
}

void wifi_reload() {
  #ifdef ESP32_BLE
    Serial.println("WiFi: Initiating reload...");
  #endif
  if (WiFi.getMode() != WIFI_OFF) {
    #ifdef ESP32_BLE
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
  #ifdef ESP32_BLE
    Serial.print("WiFi: Attempting to connect to AP '");
    Serial.print(config_wifi_ssid);
    Serial.println("'...");
  #endif
  WiFi.mode(WIFI_STA);
  WiFi.begin(config_wifi_ssid.c_str(), config_wifi_password.c_str());

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < WIFI_CONNECT_ATTEMPTS) {
    wait(WIFI_CONNECT_DELAY_MS);
    #ifdef ESP32_BLE
      Serial.print(".");
    #endif
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    #ifdef ESP32_BLE
      Serial.print("\nWiFi: connected ");
      Serial.println(WiFi.localIP());
    #endif
    wifi_ap_mode = false;
  } else {
    #ifdef ESP32_BLE
      Serial.println("\nWiFi: connect failed, starting AP mode");
    #endif
    WiFi.disconnect(true);
    wait(500);
    WiFi.softAP(WIFI_AP_SSID);
    dnsServer.start();
    wifi_ap_mode = true;
    #ifdef ESP32_BLE
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

  websocket_server = String(websocket_server);
  #ifdef ESP32_BLE
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
      //doc["command"] = "connect";
      //doc["data"]["api_key"] = config_ch_racing_club_api_key;
      //doc["data"]["ip"] = WiFi.localIP().toString();
    }

    char output[256];
    serializeJson(doc, output);
    client.ping();
    client.send(output);
    #ifdef ESP32_BLE
      Serial.println("Websocket: connected.");
    #endif
    websocket_backoff = 1000; // reset backoff time on successful connection
  } else {
    #ifdef ESP32_BLE
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
    //handleCHRacingClubUpdateEvent(doc);
  } else {
    if (doc.containsKey("type")) {
      handleSmartRaceUpdateEvent(doc["type"].as<String>(), doc);
    } else {
      Serial.println("Received message without 'type' key: " + message.data());
    }
  }
}

void handleSmartRaceUpdateEvent(String type, JsonDocument doc) {
  if (type == "update_event_status" && doc.containsKey("data")) {
    String data = doc["data"].as<String>();
    if (data == "prepare_for_start") {
      #if defined(DEBUG) && defined(ESP32_BLE)
        Serial.println("INFO - prepare_for_start");
      #endif
    } else if (data == "starting") {
      #if defined(DEBUG) && defined(ESP32_BLE)
        Serial.println("INFO - starting");
      #endif
      activateLaunchControl();
    } else if (data == "running") {
      #if defined(DEBUG) && defined(ESP32_BLE)
        Serial.println("INFO - running");
      #endif
      drive();
    } else if (data == "suspended") {
      #if defined(DEBUG) && defined(ESP32_BLE)
        Serial.println("INFO - suspended");
      #endif
      stop();
    } else if (data == "ended") {
      #if defined(DEBUG) && defined(ESP32_BLE)
        Serial.println("INFO - ended");
      #endif
      stop();
    } else {
      #if defined(DEBUG) && defined(ESP32_BLE)
        Serial.println("Unknown message type: " + type);
      #endif
    }
  } else if (type == "reset") {
    #if defined(DEBUG) && defined(ESP32_BLE)
      Serial.println("INFO - reset");
    #endif
    stop();
  } else {
    #if defined(DEBUG) && defined(ESP32_BLE)
      Serial.println("Unknown message type: " + type);
    #endif
  }
}

void onEventsCallback(WebsocketsEvent event, String data) {
  if(event == WebsocketsEvent::ConnectionOpened) {
    #ifdef ESP32_BLE
      Serial.println("Websocket: connnection opened");
    #endif
    #ifdef RGB_LED
      rgbLedWrite(RGB_LED_PIN, 200, 200, 200);
    #endif
    #ifdef LED_PIN
      digitalWrite(LED_PIN, LOW);
    #endif
    websocket_connected = true;
  } else if(event == WebsocketsEvent::ConnectionClosed) {
    #ifdef ESP32_BLE
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
     #if defined(DEBUG) && defined(ESP32_BLE)
      Serial.println("Websocket: got a ping!");
    #endif
    client.pong();
  } else if(event == WebsocketsEvent::GotPong) {
     #if defined(DEBUG) && defined(ESP32_BLE)
      Serial.println("Websocket: got a pong!");
    #endif
  }
}

void wait(unsigned long waitTime) {
  unsigned long startWaitTime = millis();
  while((millis() - startWaitTime) < waitTime) {
    if (webserver_running) server.handleClient();
    else delay(1);
  }
}

void activateLaunchControl() {
   #if defined(DEBUG) && defined(ESP32_BLE)
    Serial.println("INFO - activate launch control");
  #endif
  isDriving = false;
  activeLaunchControl = true;
  isBraking = false;
  resetJoystickPosition();
  Joystick.setButton(KEY_TURBO, 1);
  Joystick.setAccelerator(speed);
  Joystick.sendState();
  #ifdef RGB_LED
    rgbLedWrite(RGB_LED_PIN, 255, 255, 0);
  #endif
}

void drive() {
  #if defined(DEBUG) && defined(ESP32_BLE)
    Serial.println("INFO - ghostcar driving");
  #endif
  isBraking = false;
  if(activeLaunchControl) {
    Joystick.setBrake(0);
    Joystick.setButton(KEY_TURBO, 0);
  }
  else {
    resetJoystickPosition();
    Joystick.setAccelerator(speed);
  }
  Joystick.sendState();
  activeLaunchControl = false;
  isDriving = true;
  #ifdef RGB_LED
    rgbLedWrite(RGB_LED_PIN, 255, 0, 0);
  #endif
}

void stop() {
  isDriving = false;
  #if defined(DEBUG) && defined(ESP32_BLE)
    Serial.println("INFO - ghostcar stopped");
  #endif
  resetJoystickPosition();
  Joystick.setBrake(100);
  Joystick.sendState();
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
    USB.productName("CH-GhostCar-SmartRace");
    USB.manufacturerName("CH-Community");
    USB.begin();
    delay(100);
  #endif
  Joystick.setXAxisRange(-100, 100);
  Joystick.setAcceleratorRange(0, 100);
  Joystick.setBrakeRange(0, 100);
  Joystick.begin(false);
  wait(500);
  resetJoystickPosition();
}

void resetJoystickPosition() {
  isDriving = false;
  Joystick.setXAxis(0L);
  Joystick.setAccelerator(0L);
  Joystick.setBrake(0L);
  Joystick.setButton(KEY_TURBO, 0L);
  Joystick.sendState();
  isBraking = false;
}

void setup() {
  preferences.begin(PREFERENCES_NAMESPACE, false);
  // Setup Callbacks
  client.onMessage(onMessageCallback);
  client.onEvent(onEventsCallback);

  #ifdef ESP32_BLE
    Serial.begin(115200);
    wait(2000);
    Serial.print("CH-GhostCar-SmartRace Version: ");
    Serial.print(VERSION);
    Serial.println(" started.");
    Serial.println("############################");
  #endif
  configuration_load();

  #ifdef ESP32_BLE
    Serial.println();
    Serial.print("Starting ...");
  #endif

  if (config_wifi_ssid != "") {
    WiFi.setHostname(config_wifi_hostname.c_str());
    WiFi.begin(config_wifi_ssid.c_str(), config_wifi_password.c_str());

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < WIFI_CONNECT_ATTEMPTS) {
      wait(WIFI_CONNECT_DELAY_MS);
      #ifdef ESP32_BLE
        Serial.print(".");
      #endif
      attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
      #ifdef ESP32_BLE
        Serial.print("\nWiFi:  connected ");
        Serial.println(WiFi.localIP());
      #endif
      wifi_ap_mode = false;
    } else {
      #ifdef ESP32_BLE
        Serial.println("\nWiFi:  connection failed!");
      #endif
      WiFi.softAP(WIFI_AP_SSID);
      dnsServer.start();
      #ifdef ESP32_BLE
        Serial.println("WiFi: started AP mode");
      #endif
      wifi_ap_mode = true;
    }
  } else {
    WiFi.softAP(WIFI_AP_SSID);
    dnsServer.start();
    #ifdef ESP32_BLE
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
  #ifdef ESP32_BLE
    Serial.println("Webserver: running");
  #endif
  webserver_running = true;
}

void loop() {
  server.handleClient();
  if(websocket_connected) {
    client.poll();
    if(millis() > (websocket_last_ping + WEBSOCKET_PING_INTERVAL)) {
      websocket_last_ping = millis();
      client.ping();
    }
    if(millis() > (lastJoystickUpdate + JOYSTICK_UPDATE_INTERVAL)) {
      lastJoystickUpdate = millis();
      Joystick.sendState();
    }
    if(isBraking && (millis() > (brakeTime + DEFAULT_BRAKING_TIME))) {
      isBraking = false;
      Joystick.setBrake(0);
      Joystick.sendState();
    }
  }
  else {
    if(!wifi_ap_mode) connectWebsocket();
  }
}
