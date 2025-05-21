#include "config.h"
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>

//ArduinoWebsockets 0.5.4
#include <ArduinoWebsockets.h>

#if defined(ESP32_BLE)
  #include "src/Joystick_BLE/Joystick_BLE.h"
  Joystick_ Joystick;
#else
  #define ESP32S3
  #include <Joystick_ESP32S2.h> // Tested V0.9.5 https://github.com/schnoog/Joystick_ESP32S2
  Joystick_ Joystick(JOYSTICK_DEFAULT_REPORT_ID,JOYSTICK_TYPE_GAMEPAD, 
                   14, 2,true, false, false, false, false, 
                   false,false, false, true, true, false);
#endif

using namespace websockets;

bool initJoystick = false;
bool activeLaunchControl = false;

String websocket_server = "";
bool ap_mode = false;
bool websocket_connected = false;
unsigned long last_ping = 0;
unsigned long lastJoystickUpdate = 0;
bool isBraking = false;
unsigned long brakeTime = 0;

WebsocketsClient client;

const char* AP_SSID = "CH-GhostCar-SmartRace-Config";
WebServer server(80);
DNSServer dnsServer;
String hostName = "";
Preferences preferences;

int speed = 100; // ghostcar speed %

String ssid, password, serverAddress, serverPort;

void saveConfig() {
  preferences.putString("ssid", ssid);
  preferences.putString("password", password);
  preferences.putString("serverAddress", serverAddress);
  preferences.putString("serverPort", serverPort);
  preferences.putString("hostName", hostName);
  preferences.putInt("speed", speed); // ghostcar speed
}
  
void loadConfig() {
  ssid = preferences.getString("ssid", "");
  password = preferences.getString("password", "");
  serverAddress = preferences.getString("serverAddress", "");
  serverPort = preferences.getString("serverPort", "");
  hostName = preferences.getString("hostName", "CH-GhostCar-SmartRace");
  speed = preferences.getInt("speed", 100); // ghostcar speed
}
  
void handleNotFound() {
  server.sendHeader("Location", "/");
  server.send(302, "text/plain", "redirect to configuration page");
}
  
void handleRoot() {
  String html = "<!DOCTYPE html><html><head><title>CH-GhostCar-SmartRace</title>";
  html += "<meta charset='UTF-8'>"; // Specify UTF-8 encoding
  html += "<style>body{display:flex;justify-content:center;align-items:center;min-height:100vh;margin:0;}form{display:flex;flex-direction:column;max-width:100vw;}label{margin-bottom:5px;}input[type=text],input[type=password],input[type=number]{width:100%;padding:8px;margin-bottom:10px;border:1px solid #ccc;border-radius:4px;}input[type=submit]{background-color:#4CAF50;color:white;padding:10px 15px;border:none;border-radius:4px;}</style>";
  html += "</head><body>";
  html += "<form action='/config' method='POST'>";
  html += "<h1 align=center>CH-GhostCar-SmartRace</h1>";
  html += "<label for='ssid'>SSID:</label>";
  html += "<input type='text' style='width:auto;' id='ssid' name='ssid' value='" + ssid + "'><br>";
  html += "<label for='password'>Passwort:</label>";
  html += "<input type='password' style='width:auto;' id='password' name='password' value='" + password + "'><br>";
  html += "<label for='serverAddress'>Server Adresse:</label>";
  html += "<input type='text' style='width:auto;' id='serverAddress' name='serverAddress' value='" + serverAddress + "'><br>";
  html += "<label for='serverPort'>Server Port:</label>";
  html += "<input type='number' style='width:auto;' id='serverPort' name='serverPort' value='" + serverPort + "'><br>";
  html += "<label for='hostName'>Hostname:</label>";
  html += "<input type='text' style='width:auto;' id='hostName' name='hostName' value='" + hostName + "'><br>";
  html += "<label for='speed'>GhostCar speed (%):</label>";
  html += "<input type='number' style='width:auto;' id='speed' name='speed' value='" + String(speed) + "'><br>";
  html += "<input type='submit' style='margin-bottom:20px;' value='Speichern'>";
  html += "</form>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}
  
void handleConfig() {
  if (server.args() > 0) {
    bool reConnectWifi = ssid != server.arg("ssid") || password != server.arg("password" || hostName != server.arg("hostName"));
    ssid = server.arg("ssid");
    password = server.arg("password");
    serverAddress = server.arg("serverAddress");
    serverPort = server.arg("serverPort");
    hostName = server.arg("hostName");
    speed = server.arg("speed").toInt();
    if (speed > 100) {
      speed = 100;
    }
    else if (speed < 0) {
      speed = 0;
    }
    Joystick.setAccelerator(speed);
    Joystick.sendState();
    saveConfig();
    server.send(200, "text/html", "<!DOCTYPE html><html><head><title>CH-GhostCar-SmartRace</title></head><body><h1>Configuration saved!</h1><p>You will be redirected in 2 seconds.</p><script>setTimeout(function() { window.location.href = '/'; }, 2000);</script></body></html>");

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
      while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
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
    server.send(200, "text/html", "<!DOCTYPE html><html><head><title>CH-GhostCar-SmartRace</title></head><body><h1>Invalid request!</h1><p>You will be redirected in 2 seconds.</p><script>setTimeout(function() { window.location.href = '/'; }, 2000);</script></body></html>");
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
    wait(1000);
    attempts++;
  }
  Serial.println("");
  if(websocket_connected) {
    client.send("{\"type\":\"api_version\"}");
    client.ping();
    client.send("{\"type\":\"controller_set\",\"data\":{\"controller_id\":\"1\"}}");
    if(!initJoystick) {
      initializeJoystickMode();
      initJoystick = true;
    }
  }
}


void onMessageCallback(WebsocketsMessage message) {
  #ifdef DEBUG
    Serial.print("Got Message: ");
    Serial.println(message.data());
  #endif
  if (message.data() == "{\"type\":\"update_event_status\",\"data\":\"prepare_for_start\"}") {
    #ifdef DEBUG
      Serial.println("INFO - prepare_for_start");
    #endif
  }
  else if (message.data() == "{\"type\":\"update_event_status\",\"data\":\"starting\"}") {
    #ifdef DEBUG
      Serial.println("INFO - starting");
    #endif
    activateLaunchControl();
  }
  else if (message.data() == "{\"type\":\"update_event_status\",\"data\":\"running\"}") {
    #ifdef DEBUG
      Serial.println("INFO - running");
    #endif
    drive();
  }
  else if (message.data() == "{\"type\":\"update_event_status\",\"data\":\"suspended\"}") {
    #ifdef DEBUG
      Serial.println("INFO - suspended");
    #endif
    stop();
  }
  else if (message.data() == "{\"type\":\"update_event_status\",\"data\":\"ended\"}") {
    #ifdef DEBUG
      Serial.println("INFO - ended");
    #endif
    stop();
  }
  else if (message.data() == "{\"type\":\"reset\"}") {
    #ifdef DEBUG
      Serial.println("INFO - reset");
    #endif
    stop();
  }
}

void onEventsCallback(WebsocketsEvent event, String data) {
  if(event == WebsocketsEvent::ConnectionOpened) {
    Serial.println("INFO - Connnection Opened");
    #ifdef RGB_LED
      rgbLedWrite(RGB_LED_PIN, 200, 200, 200);
    #endif
    #ifdef LED_PIN
      digitalWrite(LED_PIN, LOW);
    #endif
    websocket_connected = true;
  } else if(event == WebsocketsEvent::ConnectionClosed) {
    Serial.println("INFO - Connnection Closed");
    #ifdef RGB_LED
      rgbLedWrite(RGB_LED_PIN, 0, 0, 255);
    #endif
    #ifdef LED_PIN
      digitalWrite(LED_PIN, HIGH);
    #endif
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

void activateLaunchControl() {
  #ifdef DEBUG
    Serial.println("INFO - activate launch control");
  #endif
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
  #ifdef DEBUG
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
  #ifdef RGB_LED
    rgbLedWrite(RGB_LED_PIN, 255, 0, 0);
  #endif
}

void stop() {
  #ifdef DEBUG
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
  Joystick.setXAxis(0L);
  Joystick.setAccelerator(0L);
  Joystick.setBrake(0L);
  Joystick.setButton(KEY_TURBO, 0L);
  Joystick.sendState();
  isBraking = false;
}

void setup() {
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

  server.on("/", handleRoot);
  server.on("/config", HTTP_POST, handleConfig);
  // all unknown pages are redirected to configuration page
  server.onNotFound(handleNotFound);
  
  server.begin();
  Serial.println("Webserver gestartet...");

  Serial.print("CH-GhostCar-SmartRace Version: ");
  Serial.print(VERSION);
  Serial.println(" started.");
  Serial.println("############################");
}
  
void loop() {
  server.handleClient();
  if(websocket_connected) {
    client.poll();
    if(millis() > (last_ping + 5000)) {
      last_ping = millis();
      client.ping();
    }
    if(millis() > (lastJoystickUpdate + 100)) {
      lastJoystickUpdate = millis();
      Joystick.sendState();
    }
    if(isBraking && (millis() > (brakeTime+5000))) {
      isBraking = false;
      Joystick.setBrake(0);
      Joystick.sendState();
    }
  }
  else {
    if(!ap_mode) connectWebsocket();
  }
}