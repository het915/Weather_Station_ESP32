//AIzaSyAV9Is-wg2rImdIh5lpRoBiDObqUbTV7fs
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <Wire.h>
#include "DFRobot_BME680_I2C.h"
#include "time.h"
#define CALIBRATE_PRESSURE
DFRobot_BME680_I2C bme(0x76);
float seaLevel;
// Provide the token generation process info.
#include "addons/TokenHelper.h"
// Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

// Insert your network credentials
#define WIFI_SSID "189"
#define WIFI_PASSWORD "hello3214"

// Insert Firebase project API Key
#define API_KEY "AIzaSyAV9Is-wg2rImdIh5lpRoBiDObqUbTV7fs"

// Insert Authorized Email and Corresponding Password
#define USER_EMAIL "het915@gmail.com"
#define USER_PASSWORD "hetpatel"

// Insert RTDB URLefine the RTDB URL
#define DATABASE_URL "https://weatherdata-5faec-default-rtdb.asia-southeast1.firebasedatabase.app/"

// Define Firebase objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Variable to save USER UID
String uid;

// Database main path (to be updated in setup with the user UID)
String databasePath;
// Database child nodes
String tempPath = "/temperature";
String humPath = "/humidity";
String presPath = "/pressure";
String timePath = "/timestamp";

// Parent Node (to be updated in every loop)
String parentPath;

int timestamp;
FirebaseJson json;

const char* ntpServer = "pool.ntp.org";


float temperature;
float humidity;
float pressure;

// Timer variables (send new readings every three minutes)
unsigned long sendDataPrevMillis = 0;
unsigned long timerDelay = 60000;

// Initialize BME280
void initBME(){
  uint8_t rslt = 1;
  while (rslt != 0) {
    rslt = bme.begin();
    if (rslt != 0) {
      Serial.println("bme begin failure");
      delay(2000);
    }
  }
  Serial.println("bme begin successful");
#ifdef CALIBRATE_PRESSURE
  bme.startConvert();
  delay(1000);
  bme.update();
  seaLevel = bme.readSeaLevel(525.0);
  Serial.print("seaLevel :");
  Serial.println(seaLevel);
#endif
}

// Initialize WiFi
void initWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
  Serial.println();
}

// Function that gets current epoch time
unsigned long getTime() {
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    //Serial.println("Failed to obtain time");
    return(0);
  }
  time(&now);
  return now;
}

void setup(){
  Serial.begin(9600);

  initBME();
  initWiFi();
  configTime(0, 0, ntpServer);

  // Assign the api key (required)
  config.api_key = API_KEY;

  // Assign the user sign in credentials
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  // Assign the RTDB URL (required)
  config.database_url = DATABASE_URL;

  Firebase.reconnectWiFi(true);
  fbdo.setResponseSize(4096);

  // Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h

  // Assign the maximum retry of token generation
  config.max_token_generation_retry = 5;

  // Initialize the library with the Firebase authen and config
  Firebase.begin(&config, &auth);

  // Getting the user UID might take a few seconds
  Serial.println("Getting User UID");
  while ((auth.token.uid) == "") {
    Serial.print('.');
    delay(1000);
  }
  // Print user UID
  uid = auth.token.uid.c_str();
  Serial.print("User UID: ");
  Serial.println(uid);

  // Update database path
  databasePath = "/UsersData/" + uid + "/readings";
}

void loop(){
  bme.startConvert();
  delay(1000);
  bme.update();
  // Send new readings to database
  if (Firebase.ready() && (millis() - sendDataPrevMillis > timerDelay || sendDataPrevMillis == 0)){
    sendDataPrevMillis = millis();

    //Get current timestamp
    timestamp = getTime();
    Serial.print ("time: ");
    Serial.println (timestamp);

    parentPath= databasePath + "/" + String(timestamp);

    json.set(tempPath.c_str(), String(bme.readTemperature()));
    json.set(humPath.c_str(), String(bme.readHumidity()));
    json.set(presPath.c_str(), String(bme.readPressure()/100.0F));
    json.set(timePath, String(timestamp));
    Serial.printf("Set json... %s\n", Firebase.RTDB.setJSON(&fbdo, parentPath.c_str(), &json) ? "ok" : fbdo.errorReason().c_str());
  }
}
