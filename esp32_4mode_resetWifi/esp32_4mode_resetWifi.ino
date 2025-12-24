#include <WiFi.h>
#include <Preferences.h>
#include <WiFiManager.h>

//HARDWARE

#define MODE_BTN 15

#define LED_RED   2
#define LED_BLUE  4
#define LED3     16
#define LED4     17

//WIFI

#define WIFI_COUNT 6
#define WIFI_TRY_TIME 15000

const char* ssids[WIFI_COUNT] = {
  "COFE_D",
  "COFE_8451F",
  "COFE_E",
  "COFE_A",
  "COFE_B",
  "COFE_C"
};

const char* passwords[WIFI_COUNT] = {
  "COFE12345D",
  "12345678",
  "COFE12345E",
  "COFE_12345A",
  "COFE12345B",
  "COFE12345C"
};

Preferences preferences;

int mode = 1;
bool lastBtn = HIGH;

int wifiIndex = 0;
unsigned long wifiStartTime = 0;
bool wifiTrying = false;

bool hotspotStarted = false;
bool inConfigMode = false;
bool ledState = false;
unsigned long lastBlink = 0;

//LED STATUS

void ledSearching() {
  if (millis() - lastBlink > 500) {
    lastBlink = millis();
    ledState = !ledState;
    digitalWrite(LED_RED, ledState);
    digitalWrite(LED_BLUE, LOW);
  }
}

void ledConnecting() {
  if (millis() - lastBlink > 200) {
    lastBlink = millis();
    ledState = !ledState;
    digitalWrite(LED_RED, ledState);
    digitalWrite(LED_BLUE, ledState);
  }
}

void ledConnected() {
  digitalWrite(LED_RED, LOW);
  digitalWrite(LED_BLUE, HIGH);
}

void ledConfigMode() {
  if (millis() - lastBlink > 300) {
    lastBlink = millis();
    ledState = !ledState;
    digitalWrite(LED_RED, ledState);
    digitalWrite(LED_BLUE, !ledState);
  }
}

//LED MDE

void setModeLEDs() {
  digitalWrite(LED_RED,  mode == 1);
  digitalWrite(LED_BLUE, mode == 2);
  digitalWrite(LED3,     mode == 3);
  digitalWrite(LED4,     mode == 4);
}

void startWiFi() {
  Serial.println("\n WiFi RESET & START");

  WiFi.disconnect(true, true);
  delay(500);

  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);

  wifiIndex = 0;
  wifiTrying = false;
}

void wifiTask() {

  if (WiFi.status() == WL_CONNECTED) {
    static unsigned long logT = 0;
    if (millis() - logT > 4000) {
      Serial.print(" CONNECTED → ");
      Serial.print(WiFi.SSID());
      Serial.print(" | IP: ");
      Serial.println(WiFi.localIP());
      logT = millis();
    }
    ledConnected();
    return;
  }

  if (!wifiTrying) {
    Serial.print("➡ Trying: ");
    Serial.println(ssids[wifiIndex]);

    WiFi.begin(ssids[wifiIndex], passwords[wifiIndex]);
    wifiStartTime = millis();
    wifiTrying = true;
    return;
  }

  if (millis() - wifiStartTime < WIFI_TRY_TIME) {
    ledConnecting();
    return;
  }

  Serial.println(" Timeout");
  WiFi.disconnect(true);
  delay(300);

  wifiIndex++;
  if (wifiIndex >= WIFI_COUNT) wifiIndex = 0;
  wifiTrying = false;
}

//WIFI MANAGER

void startWiFiManagerAP() {
  Serial.println("\n=== CONFIG MODE ===");
  inConfigMode = true;

  WiFi.disconnect(true);
  delay(500);

  WiFiManager wm;
  wm.setConfigPortalTimeout(120);

  if (wm.autoConnect("COFE-SETUP", "12345678")) {
    Serial.println(" Credentials Saved");

    preferences.begin("WifiCred", false);
    preferences.putString("ssid", WiFi.SSID());
    preferences.putString("password", WiFi.psk());
    preferences.end();
  }

  inConfigMode = false;
  hotspotStarted = false;
}

void setup() {
  Serial.begin(115200);

  pinMode(MODE_BTN, INPUT_PULLUP);

  pinMode(LED_RED, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);
  pinMode(LED3, OUTPUT);
  pinMode(LED4, OUTPUT);

  Serial.println("\n ESP32 BOOT");

  delay(200);
  if (digitalRead(MODE_BTN) == LOW) {
    Serial.println(" Config Button Hold");
    startWiFiManagerAP();
  }

  startWiFi();
  setModeLEDs();
}

void loop() {

  bool btn = digitalRead(MODE_BTN);

  if (lastBtn == HIGH && btn == LOW) {
    delay(50);

    unsigned long pressT = millis();
    while (digitalRead(MODE_BTN) == LOW) {
      if (millis() - pressT > 1000) {
        hotspotStarted = true;
        break;
      }
    }

    if (!hotspotStarted) {
      mode++;
      if (mode > 4) mode = 1;

      Serial.print("\n MODE = ");
      Serial.println(mode);

      if (mode == 1) startWiFi();
      else if (mode == 2) {
        Serial.println(" WiFi OFF");
        WiFi.disconnect(true);
        WiFi.mode(WIFI_OFF);
      }
      else if (mode == 3) {
        Serial.println("Light Sleep");
        WiFi.setSleep(true);
      }
      else if (mode == 4) {
        Serial.println(" Deep Sleep 30s");
        delay(200);
        esp_deep_sleep(30 * 1000000LL);
      }
    }

    setModeLEDs();
  }

  lastBtn = btn;

  if (hotspotStarted && !inConfigMode) {
    startWiFiManagerAP();
  }

  if (inConfigMode) {
    ledConfigMode();
    return;
  }

  if (mode == 1) {
    wifiTask();
  } else {
    ledSearching();
  }

  delay(20);
}