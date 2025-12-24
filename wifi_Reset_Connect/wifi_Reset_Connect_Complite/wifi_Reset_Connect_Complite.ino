#include <WiFi.h>
#include <Preferences.h>
#include <WiFiManager.h>

Preferences preferences;

#define RED_LED 2
#define BLUE_LED 4
#define RESET_BUTTON 15

const static char defaultSSIDS[3][25] = {"UTL", "COFE_8451F", "COFE0071"};
const static char defaultPasswords[3][25] = {"UTL12345", "12345678", "Cofe@1231"};
const int numWifis = 3;

unsigned long lastBlink = 0;
unsigned long lastButtonCheck = 0;
unsigned long lastReconnectAttempt = 0;
bool ledState = false;
bool hotspotStarted = false;
bool inConfigMode = false;

// WiFi reconnection management
int currentNetworkIndex = 0;
bool wasConnected = false;
bool connectingInProgress = false;
unsigned long connectionStartTime = 0;
const unsigned long CONNECTION_TIMEOUT = 10000; // 10 seconds

// ISR for reset button
void IRAM_ATTR resetButtonISR() {
  static unsigned long lastInterrupt = 0;
  unsigned long currentMillis = millis();
  
  if (currentMillis - lastInterrupt > 1000) {
    hotspotStarted = true;
    lastInterrupt = currentMillis;
  }
}

void ledSearching() {
  if (millis() - lastBlink > 500) {
    lastBlink = millis();
    ledState = !ledState;
    digitalWrite(RED_LED, ledState);
    digitalWrite(BLUE_LED, LOW);
  }
}

void ledConnected() {
  digitalWrite(RED_LED, LOW);
  digitalWrite(BLUE_LED, HIGH);
}

void ledConnecting() {
  if (millis() - lastBlink > 200) {
    lastBlink = millis();
    ledState = !ledState;
    digitalWrite(RED_LED, ledState);
    digitalWrite(BLUE_LED, ledState); // Both LEDs blink together
  }
}

void ledConfigMode() {
  if (millis() - lastBlink > 300) {
    lastBlink = millis();
    ledState = !ledState;
    digitalWrite(RED_LED, ledState);
    digitalWrite(BLUE_LED, !ledState);
  }
}

// Function to safely connect to WiFi
bool connectToWiFi(const char* ssid, const char* password) {
  Serial.printf("\n🔄 Connecting to: %s\n", ssid);
  
  // Check if already connected to this network
  if (WiFi.status() == WL_CONNECTED && WiFi.SSID() == String(ssid)) {
    Serial.println("Already connected to this network");
    return true;
  }
  
  // Clean disconnect before new connection
  WiFi.disconnect(true);
  delay(500); // Give time for disconnect to complete
  
  // Stop any ongoing connection
  WiFi.mode(WIFI_OFF);
  delay(100);
  WiFi.mode(WIFI_STA);
  delay(100);
  
  // Set WiFi to not autoconnect (we'll handle it manually)
  WiFi.setAutoReconnect(false);
  WiFi.persistent(false);
  
  Serial.printf("Starting connection to: %s\n", ssid);
  WiFi.begin(ssid, password);
  
  connectionStartTime = millis();
  connectingInProgress = true;
  
  return false; // Connection started, not yet complete
}

// Check connection status
bool checkConnectionStatus() {
  if (!connectingInProgress) return false;
  
  wl_status_t status = WiFi.status();
  
  if (status == WL_CONNECTED) {
    Serial.printf(" Successfully connected to: %s\n", WiFi.SSID().c_str());
    Serial.printf("IP Address: %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("RSSI: %d dBm\n", WiFi.RSSI());
    connectingInProgress = false;
    return true;
  }
  
  // Check for timeout
  if (millis() - connectionStartTime > CONNECTION_TIMEOUT) {
    Serial.printf(" Timeout connecting to: %s\n", WiFi.SSID().c_str());
    
    // Print reason for failure
    switch(status) {
      case WL_IDLE_STATUS:
        Serial.println("Status: Idle");
        break;
      case WL_NO_SSID_AVAIL:
        Serial.println("Status: SSID not available");
        break;
      case WL_SCAN_COMPLETED:
        Serial.println("Status: Scan completed");
        break;
      case WL_CONNECT_FAILED:
        Serial.println("Status: Connection failed");
        break;
      case WL_CONNECTION_LOST:
        Serial.println("Status: Connection lost");
        break;
      case WL_DISCONNECTED:
        Serial.println("Status: Disconnected");
        break;
      default:
        Serial.printf("Status: Unknown (%d)\n", status);
        break;
    }
    
    WiFi.disconnect(true);
    delay(100);
    connectingInProgress = false;
  }
  
  return false;
}

// Get total number of networks available
int getNetworkCount() {
  int count = 0;
  
  // Check saved network
  preferences.begin("WifiCred", true);
  String savedSSID = preferences.getString("ssid", "");
  preferences.end();
  
  if (savedSSID != "") {
    count++;
  }
  
  // Add default networks
  count += numWifis;
  
  return count;
}

// Get network at index
bool getNetworkAtIndex(int index, String &ssid, String &password) {
  int count = 0;
  
  // First check saved network
  preferences.begin("WifiCred", true);
  String savedSSID = preferences.getString("ssid", "");
  String savedPass = preferences.getString("password", "");
  preferences.end();
  
  if (savedSSID != "") {
    if (count == index) {
      ssid = savedSSID;
      password = savedPass;
      return true;
    }
    count++;
  }
  
  // Check default networks
  for (int i = 0; i < numWifis; i++) {
    if (count == index) {
      ssid = String(defaultSSIDS[i]);
      password = String(defaultPasswords[i]);
      return true;
    }
    count++;
  }
  
  return false; // Index out of bounds
}

void startWiFiManagerAP() {
  Serial.println("\n=== Starting WiFiManager Hotspot ===");
  inConfigMode = true;
  
  // Disconnect from current network
  WiFi.disconnect(true);
  delay(1000);
  
  // Create WiFiManager instance
  WiFiManager *wm = new WiFiManager();
  
  // Set configuration portal timeout (2 minutes)
  wm->setConfigPortalTimeout(120);
  
  bool res = wm->autoConnect("COFE-SETUP", "12345678");
  
  if (res) {
    Serial.println("\n=== WiFi Connected Successfully ===");
    Serial.printf("SSID: %s\n", WiFi.SSID().c_str());
    Serial.printf("IP Address: %s\n", WiFi.localIP().toString().c_str());
    
    // Save new credentials
    preferences.begin("WifiCred", false);
    preferences.putString("ssid", WiFi.SSID());
    preferences.putString("password", WiFi.psk());
    preferences.end();
    
    Serial.println("Credentials saved to preferences");
    
    // Reset connection parameters
    wasConnected = true;
    currentNetworkIndex = 0;
    connectingInProgress = false;
    
  } else {
    Serial.println("Config portal timed out");
    // Fall back to saved/default networks
    WiFi.mode(WIFI_STA);
    delay(100);
  }
  
  delete wm;
  inConfigMode = false;
  hotspotStarted = false;
  
  Serial.println("=== Returning to normal operation ===\n");
}

void checkAndReconnectWiFi() {
  // Check if connection attempt is in progress
  if (connectingInProgress) {
    if (checkConnectionStatus()) {
      // Connection successful
      wasConnected = true;
      Serial.println(" Connection established!");
    }
    return;
  }
  
  // Check current connection status
  if (WiFi.status() == WL_CONNECTED) {
    if (!wasConnected) {
      Serial.printf("\n WiFi Connected!\n");
      Serial.printf("SSID: %s\n", WiFi.SSID().c_str());
      Serial.printf("IP: %s\n", WiFi.localIP().toString().c_str());
      Serial.printf("RSSI: %d dBm\n", WiFi.RSSI());
      wasConnected = true;
      currentNetworkIndex = 0; // Reset to first network
    }
    return;
  }
  
  // If we were connected but now disconnected
  if (wasConnected) {
    Serial.println("\n WiFi Disconnected! Attempting to reconnect...");
    wasConnected = false;
    WiFi.disconnect(true);
    delay(500);
    currentNetworkIndex = 0; // Start from first network
  }
  
  // Try to reconnect (every 5 seconds when not connected)
  if (millis() - lastReconnectAttempt > 5000) {
    lastReconnectAttempt = millis();
    
    int networkCount = getNetworkCount();
    if (networkCount == 0) {
      Serial.println("No networks configured!");
      return;
    }
    
    // Get current network
    String ssid, password;
    if (getNetworkAtIndex(currentNetworkIndex, ssid, password)) {
      Serial.printf("\n📶 Trying network %d/%d: %s\n", 
                   currentNetworkIndex + 1, networkCount, ssid.c_str());
      
      // Start connection
      connectToWiFi(ssid.c_str(), password.c_str());
      
      // Move to next network for next attempt
      currentNetworkIndex = (currentNetworkIndex + 1) % networkCount;
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n======================================");
  Serial.println("        COFE WiFi Device");
  Serial.println("        Manual WiFi Cycling");
  Serial.println("======================================\n");
  
  // Initialize LEDs
  pinMode(RED_LED, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);
  digitalWrite(RED_LED, LOW);
  digitalWrite(BLUE_LED, LOW);
  
  // Initialize reset button with interrupt
  pinMode(RESET_BUTTON, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(RESET_BUTTON), resetButtonISR, FALLING);
  
  // Check for reset button on startup
  delay(100);
  if (digitalRead(RESET_BUTTON) == LOW) {
    Serial.println("Reset button pressed on startup. Entering config mode.");
    delay(500);
    startWiFiManagerAP();
  }
  
  // Initialize WiFi
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(false); // We handle reconnection manually
  WiFi.persistent(false); // Don't save to flash automatically
  
  Serial.println("Setup complete. Starting WiFi connection...\n");
  
  // Force immediate connection attempt
  lastReconnectAttempt = millis() - 6000;
  
  // Print available networks
  Serial.println("Available Networks:");
  int count = getNetworkCount();
  for (int i = 0; i < count; i++) {
    String ssid, password;
    if (getNetworkAtIndex(i, ssid, password)) {
      Serial.printf("  %d. %s\n", i + 1, ssid.c_str());
    }
  }
  Serial.println();
}

void loop() {
  // Handle hotspot start request
  if (hotspotStarted && !inConfigMode) {
    startWiFiManagerAP();
  }
  
  // Handle config mode
  if (inConfigMode) {
    ledConfigMode();
    return;
  }
  
  // Handle connection in progress
  if (connectingInProgress) {
    ledConnecting();
    checkConnectionStatus();
  } else {
    // Check WiFi connection and reconnect if necessary
    checkAndReconnectWiFi();
    
    // Update LED status
    if (WiFi.status() == WL_CONNECTED) {
      ledConnected();
    } else {
      ledSearching();
    }
  }
  
  // Manual button check (backup to ISR)
  if (millis() - lastButtonCheck > 200) {
    lastButtonCheck = millis();
    
    if (digitalRead(RESET_BUTTON) == LOW) {
      static unsigned long lastButtonPress = 0;
      if (millis() - lastButtonPress > 1000) {
        lastButtonPress = millis();
        Serial.println("\nReset button pressed (manual check)");
        hotspotStarted = true;
      }
    }
  }
  
  // Print connection status periodically
  static unsigned long lastStatusPrint = 0;
  if (millis() - lastStatusPrint > 10000) { // Every 10 seconds
    lastStatusPrint = millis();
    
    wl_status_t status = WiFi.status();
    
    if (status == WL_CONNECTED) {
      Serial.printf("[Status] Connected to: %s, RSSI: %d dBm, IP: %s\n", 
                    WiFi.SSID().c_str(), WiFi.RSSI(), WiFi.localIP().toString().c_str());
    } else if (connectingInProgress) {
      Serial.printf("[Status] Connecting to network %d... (%lu ms elapsed)\n", 
                    currentNetworkIndex + 1, millis() - connectionStartTime);
    } else {
      Serial.printf("[Status] Not connected. Next attempt in %lu seconds\n", 
                    (5000 - (millis() - lastReconnectAttempt)) / 1000);
    }
  }
  
  delay(10); // Small delay
}