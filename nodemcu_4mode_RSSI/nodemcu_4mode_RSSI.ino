#include <ESP8266WiFi.h>

/* ================= CONFIG ================= */

#define MODE_BTN D1

#define LED1 D5
#define LED2 D6
#define LED3 D7
// D8 NOT USED (BOOT PIN)

#define WIFI_COUNT 6
#define WIFI_TRY_TIME 15000   // 15 sec safe

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

/* ================= LED ================= */

void setLEDs() {
  digitalWrite(LED1, mode == 1);
  digitalWrite(LED2, mode == 2);
  digitalWrite(LED3, mode == 3);
}

/* ================= WIFI ================= */

void startWiFi() {
  Serial.println("\n WiFi START");

  WiFi.mode(WIFI_STA);
  WiFi.setSleepMode(WIFI_NONE_SLEEP);
  WiFi.disconnect();   // <<< IMPORTANT FIX

  wifiIndex = 0;
  wifiTrying = false;
}

void wifiTask() {

  //  CONNECTED
  if (WiFi.status() == WL_CONNECTED) {
    static unsigned long logT = 0;
    if (millis() - logT > 3000) {
      Serial.print(" CONNECTED → ");
      Serial.print(WiFi.SSID());
      Serial.print(" | IP: ");
      Serial.print(WiFi.localIP());
      Serial.print(" | RSSI: ");
      Serial.print(WiFi.RSSI());
      Serial.println(" dBm");
      logT = millis();
    }
    return;
  }

  //  START TRY
  if (!wifiTrying) {
    Serial.print("\n➡ Trying WiFi: ");
    Serial.println(ssids[wifiIndex]);

    WiFi.begin(ssids[wifiIndex], passwords[wifiIndex]);
    wifiStartTime = millis();
    wifiTrying = true;
    return;
  }

  //  WAIT
  if (millis() - wifiStartTime < WIFI_TRY_TIME) {
    Serial.print(".");
    delay(300);
    return;
  }

  //  FAILED → NEXT
  Serial.println("\n Failed, switching AP");

  WiFi.disconnect();
  delay(500);

  wifiIndex++;
  if (wifiIndex >= WIFI_COUNT) wifiIndex = 0;

  wifiTrying = false;
}

/* ================= SETUP ================= */

void setup() {
  Serial.begin(115200);
  delay(2000);

  pinMode(MODE_BTN, INPUT_PULLUP);

  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT);

  Serial.println("\n NODEMCU BOOT");
  Serial.println("MODE 1 = WIFI ON");

  startWiFi();
  setLEDs();
}

/* ================= LOOP ================= */

void loop() {

  bool btn = digitalRead(MODE_BTN);

  if (lastBtn == HIGH && btn == LOW) {
    delay(50);
    mode++;
    if (mode > 3) mode = 1;

    Serial.print("\n MODE = ");
    Serial.println(mode);

    if (mode == 1) startWiFi();
    else if (mode == 2) {
      Serial.println(" WiFi OFF");
      WiFi.disconnect();
      WiFi.mode(WIFI_OFF);
    }
    else if (mode == 3) {
      Serial.println(" Light Sleep");
      WiFi.setSleepMode(WIFI_LIGHT_SLEEP);
    }

    setLEDs();
  }

  lastBtn = btn;

  if (mode == 1) wifiTask();
}