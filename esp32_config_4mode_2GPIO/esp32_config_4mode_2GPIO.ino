#include <WiFi.h>
#include <WiFiMulti.h>
#include <WiFiManager.h>

// HARDWARE
#define MODE_BTN   15
#define STATUS_LED 16   

// WIFI
#define WIFI_COUNT 6

const char* ssids[WIFI_COUNT] = {
  "COFE_D","COFE_8451F","COFE_E","COFE_A","COFE_B","COFE_C"
};

const char* passwords[WIFI_COUNT] = {

  "COFE12345D","12345678","COFE12345E",
  "COFE_12345A","COFE12345B","COFE12345C"
};

WiFiMulti wifiMulti;

int mode = 1;

//BUTTOn
bool lastBtn = HIGH;
bool btnPressed = false;
unsigned long btnPressTime = 0;

bool hotspotStarted = false;
bool inConfigMode   = false;

unsigned long ledPrev = 0;
bool ledState = false;

#define BLINK_SYSTEM_ALIVE   10000UL
#define BLINK_WIFI_CONNECTED  5000UL
#define BLINK_WIFI_LOST      20000UL
#define BLINK_CONFIG           250UL

void WiFiEvent(WiFiEvent_t event) { 
  switch (event) {
    case WIFI_EVENT_STA_CONNECTED:
      Serial.println(" WiFi connected to AP");
      break;

    case WIFI_EVENT_STA_DISCONNECTED:
      Serial.println(" WiFi discon+nected");
      break;

    case IP_EVENT_STA_GOT_IP:
      Serial.print(" IP Address: ");
      Serial.println(WiFi.localIP());
      break;

    default:
      break;
  }
}

// LED TASK 
void statusLedTask() {
  unsigned long now = millis();
  unsigned long interval;

  if (inConfigMode) {
    interval = BLINK_CONFIG;
  }
  else if (mode == 1 && WiFi.status() != WL_CONNECTED) {
    interval = BLINK_WIFI_LOST;
  }
  else if (mode == 1 && WiFi.status() == WL_CONNECTED) {
    interval = BLINK_WIFI_CONNECTED;
  }
  else {
    interval = BLINK_SYSTEM_ALIVE;
  }

  if (now - ledPrev >= interval) {
    ledPrev = now;
    ledState = !ledState;
    digitalWrite(STATUS_LED, ledState);
  }
}

void startWiFi() {
  Serial.println("\n Starting Multi WiFi...");
  WiFi.disconnect(true);
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);

  for (int i = 0; i < WIFI_COUNT; i++) {
    wifiMulti.addAP(ssids[i], passwords[i]);
  }
}

void wifiTask() {
  uint8_t status = wifiMulti.run();

  if (status != WL_CONNECTED) {
    static unsigned long retryLog = 0;
    if (millis() - retryLog > 3000) {
      Serial.println(" Trying next WiFi...");
      retryLog = millis();
    }
    return;
  }

  static unsigned long logT = 0;
  if (millis() - logT > 5000) {
    Serial.print(" Connected: ");
    Serial.print(WiFi.SSID());
    Serial.print(" | RSSI: ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
    logT = millis();
  }
}

//CONFIG PORTAL

void startWiFiManagerAP() {
  Serial.println("\n⚙ CONFIG PORTAL STARTED");
  inConfigMode = true;

  WiFi.disconnect(true);

  WiFiManager wm;
  wm.setConfigPortalTimeout(120);
  wm.autoConnect("COFE-SETUP", "12345678");

  Serial.println(" CONFIG PORTAL CLOSED");
  inConfigMode = false;
  hotspotStarted = false;

  startWiFi();
}

void setup() {
  Serial.begin(115200);
  WiFi.onEvent(WiFiEvent);

  pinMode(MODE_BTN, INPUT_PULLUP);
  pinMode(STATUS_LED, OUTPUT);

  Serial.println("\n ESP32 BOOTED");

  if (digitalRead(MODE_BTN) == LOW) {
    startWiFiManagerAP();
  }

  startWiFi();
}

void loop() {

  statusLedTask();

  bool btn = digitalRead(MODE_BTN);

  // Button press detect

  if (lastBtn == HIGH && btn == LOW) {
    btnPressTime = millis();
    btnPressed = true;
  }

  // Button release

  if (lastBtn == LOW && btn == HIGH && btnPressed) {
    btnPressed = false;
    unsigned long pressDuration = millis() - btnPressTime;

    if (pressDuration >= 1000) {
      Serial.println("⚙ Long press → Config Portal");
      hotspotStarted = true;
    } else {
      mode++;
      if (mode > 4) mode = 1;

      Serial.print("\n MODE = ");
      Serial.println(mode);

      if (mode == 1) {
        Serial.println(" WiFi ON");
        startWiFi();
      }
      else if (mode == 2) {
        Serial.println(" WiFi OFF");
        WiFi.disconnect(true);
        WiFi.mode(WIFI_OFF);
      }
      else if (mode == 3) {
        Serial.println(" Light Sleep Enabled");
        WiFi.setSleep(true);
      }
      else if (mode == 4) {
        Serial.println(" Deep Sleep 30s");
        delay(100);
        esp_deep_sleep(30 * 1000000LL);
      }
    }
  }

  lastBtn = btn;

  if (hotspotStarted && !inConfigMode) {
    startWiFiManagerAP();
  }

  if (mode == 1 && !inConfigMode) {
    wifiTask();
  }
}