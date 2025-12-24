#include <WiFi.h>
#include <WiFiMulti.h>
#include <Preferences.h>

WiFiMulti wifiMulti;
const int numWifis = 2;  

const static char defaultSSIDS[3][25] = {"UTL", "cofe-4G", "COFE007"};  
const static char defaultPasswords[3][25] = {"UTL12345", "Cofe@123", "Cofe@123"}; 

 Preferences preferences;

void saveCredentials(const char *ssid, const char *password)
{
 
  preferences.begin("WifiCred", false);
  preferences.putBytes("ssid", ssid, 64);
  preferences.putBytes("password", password, 64);
  preferences.end();
}

void loadWifiDetails()
{

  for (int i = 0; i < numWifis; i++)
  {
    wifiMulti.addAP(defaultSSIDS[i], defaultPasswords[i]);
  }
  WiFi.persistent(false);
}

void wifiLoop()

{

  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("Connecting to WiFi...");
    int wifi_status = wifiMulti.run();
  }
  Serial.println("Connected to WiFi!");

}

void setup()
{
  Serial.begin(115200);
  
  preferences.begin("WifiCred", false);
  char ssid[64] = "";
  char password[64] = "";
  preferences.getBytes("ssid", ssid, sizeof(ssid));
  preferences.getBytes("password", password, sizeof(password));
  preferences.end();

  if (ssid[0] != '\0' && password[0] != '\0') 
  {

    WiFi.begin(ssid, password);
  }
  else
  {

    loadWifiDetails();
    wifiLoop();
    
  }
}

void loop()
{

}