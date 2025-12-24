#include <WiFi.h>

/* ================= CONFIG ================= */

#define MODE_BTN 15

#define LED1 2
#define LED2 4
#define LED3 16
#define LED4 17

#define WIFI_COUNT 6
#define WIFI_TRY_TIME 15000   // ESP32 NEEDS MORE TIME

const char* ssids[WIFI_COUNT] = {
 "COFE_A",
  "COFE_B",
  "COFE_C",
  "COFE_D",
  "COFE_E",
  "COFE_8451F"

};

const char* passwords[WIFI_COUNT] = {
 "COFE_12345A",
  "COFE12345B",
  "COFE12345C",
  "COFE12345D",
  "COFE12345E",
  "12345678"
};

/* ================= VARS ================= */

int mode = 1;
bool lastBtn = HIGH;

int wifiIndex = 0;
unsigned long wifiStartTime = 0;
bool wifiTrying = false;
//led function

void setLEDs() {
  digitalWrite(LED1, mode == 1);
  digitalWrite(LED2, mode == 2);
  digitalWrite(LED3, mode == 3);
  digitalWrite(LED4, mode == 4);
}

//start wifi

void startWiFi() {
  Serial.println("\n📡 WiFi RESET & START");

  WiFi.disconnect(true, true);
  delay(500);

  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);

  wifiIndex = 0;
  wifiTrying = false;
}

//wifi tasks

void wifiTask() {

  // ---- CONNECTED ----
  if (WiFi.status() == WL_CONNECTED) {
    static unsigned long logT = 0;
    if (millis() - logT > 4000) {
      Serial.print("✅ CONNECTED → ");
      Serial.print(WiFi.SSID());
      Serial.print(" | IP: ");
      Serial.println(WiFi.localIP());
      logT = millis();
    }
    return;
  }

  // ---- START TRY ----
  if (!wifiTrying) {
    Serial.print("➡ Trying: ");
    Serial.println(ssids[wifiIndex]);

    WiFi.begin(ssids[wifiIndex], passwords[wifiIndex]);
    wifiStartTime = millis();
    wifiTrying = true;
    return;
  }

  // ---- WAIT FOR RESULT ----
  wl_status_t st = WiFi.status();

  if (st == WL_CONNECT_FAILED || st == WL_NO_SSID_AVAIL) {
    Serial.println("❌ Immediate failure");
  }
  else if (millis() - wifiStartTime < WIFI_TRY_TIME) {
    return; // STILL TRYING
  }
  else {
    Serial.println("⏰ Timeout");
  }

  // ---- NEXT ROUTER ----
  WiFi.disconnect(true);
  delay(300);

  wifiIndex++;
  if (wifiIndex >= WIFI_COUNT) wifiIndex = 0;

  wifiTrying = false;
}

//setup

void setup() {
  Serial.begin(115200);

  pinMode(MODE_BTN, INPUT_PULLUP);

  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT);
  pinMode(LED4, OUTPUT);

  Serial.println("\n🔌 ESP32 BOOT");
  Serial.println("MODE 1 = WIFI ON");

  startWiFi();
  setLEDs();
}

//loop

void loop() {

  bool btn = digitalRead(MODE_BTN);

  if (lastBtn == HIGH && btn == LOW) {
    delay(50);
    mode++;
    if (mode > 4) mode = 1;

    Serial.print("\n🔁 MODE = ");
    Serial.println(mode);

    if (mode == 1) {
      startWiFi();
    }
    else if (mode == 2) {
      Serial.println("❌ WiFi OFF");
      WiFi.disconnect(true);
      WiFi.mode(WIFI_OFF);
    }
    else if (mode == 3) {
      Serial.println("😴 Light Sleep");
      WiFi.setSleep(true);
    }
    else if (mode == 4) {
      Serial.println("💤 Deep Sleep 30s");
      delay(200);
      esp_deep_sleep(30 * 1000000LL);
    }

    setLEDs();
  }

  lastBtn = btn;

  if (mode == 1) {
    wifiTask();
  }

  delay(20);
}