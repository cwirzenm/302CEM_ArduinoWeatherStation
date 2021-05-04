/*
Three things to do!
1. Put your WiFi details into the code
2. Put the Firebase access code that I gave you into the code
3. Put your name into the path string
If you want to use DHT sensors, then uncomment all related lines and use the floats that are on the lines 36 and 37
*/

//#include "dht.h"        // if using DHT's, uncomment all the dht lines and change the data formula
//#define dht1_apin A0    // Analog Pin sensor is connected to
//#define dht2_apin A1    // Analog Pin sensor is connected to

#include <WiFi.h>
#include <FirebaseESP32.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "time.h"

#define BME_SCK 5       // SCK to SCK
#define BME_MISO 19     // SDO to MISO
#define BME_MOSI 18     // SDI to MOSI
#define BME_CS 32       // CS to Digital Pin 0
#define LED 14          // optional LED
#define SCREEN_WIDTH 128    // OLED display width, in pixels
#define SCREEN_HEIGHT 32    // OLED display height, in pixels
#define OLED_RESET    32    // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Adafruit_BME280 bme(BME_CS, BME_MOSI, BME_MISO, BME_SCK); // software SPI

//dht DHT1, DHT2;
//float temperatureReading;
//float humidityReading;

#define WIFI_SSID "YOUR_WIFI"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
#define FIREBASE_HOST "weatherstatino-default-rtdb.europe-west1.firebasedatabase.app/"
#define FIREBASE_AUTH "ACCESS_CODE"

//Define FirebaseESP32 data object
FirebaseData fbdo;
FirebaseJson json;

const char* ntpServer = "pool.ntp.org";
const int gmtOffset_sec = 3600;
struct tm timeinfo;

String path = "/INSERT_YOUR_NAME/Readings ";
String readTime;

void printReadings();
void printResult(FirebaseData &data);
void sendReadings(String parameter, int reading);

void setup() {

  pinMode(LED, OUTPUT);
  digitalWrite(LED, HIGH);
  Serial.begin(1000000);
  while(!Serial);     // get the serial running
  delay(1000);        // wait before accessing Sensor
  digitalWrite(LED, LOW);

  unsigned status;
  status = bme.begin(0x76, &Wire);

  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);          // Don't proceed, loop forever
  }
  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  display.display();
  delay(500);         // Pause for 0.5 second

  display.clearDisplay();   // Clear the buffer
  display.setTextSize(1);                   // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE);      // Draw white text
  display.setCursor(0,0);                   // Start at top-left corner

  // Draw a single pixel in white
  display.drawPixel(10, 10, SSD1306_WHITE);
  display.display();
  delay(500);

  display.clearDisplay();   // Clear the buffer
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  display.print("Connecting to Wi-Fi");
  display.display();
  while (WiFi.status() != WL_CONNECTED) {
    display.print(".");
    display.display();
    delay(300);
  }
  display.clearDisplay();   // Clear the buffer
  display.println();
  display.println("Connected with IP: ");
  display.println(WiFi.localIP());
  display.display();

  //init and get the time
  configTime(gmtOffset_sec, 0, ntpServer);
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
  readTime = String(timeinfo.tm_mday)+" "+String(timeinfo.tm_mon+1)+" "+String(timeinfo.tm_hour)+":00";
  Serial.println(readTime);
  Serial.println(&timeinfo, "%B %d %Y %H:%M");
  display.print(&timeinfo, "%B %d %Y %H:%M");
  display.display();

  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Firebase.reconnectWiFi(true);

  //Set database read timeout to 1 minute (max 15 minutes)
  Firebase.setReadTimeout(fbdo, 1000 * 60);
  //tiny, small, medium, large and unlimited.
  //Size and its write timeout e.g. tiny (1s), small (10s), medium (30s) and large (60s).
  Firebase.setwriteSizeLimit(fbdo, "tiny");

  //set the decimal places for float
  Firebase.setFloatDigits(1);

  digitalWrite(LED, HIGH);
  delay(1000);
}

void loop() { //Start of Program
  printReadings();
  delay(5000);
}

void printReadings() {
  
  String parameter;
  int reading;
//  DHT1.read11(dht1_apin);
//  DHT2.read11(dht2_apin);
//  temperatureReading = (DHT1.temperature + DHT2.temperature + bme.readTemperature())/3;
//  humidityReading = (DHT1.humidity + DHT2.humidity + bme.readHumidity())/3;
  
  display.clearDisplay();                     // Clear the buffer
  display.setTextSize(1);                     // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE);        // Draw white text
  display.setCursor(0,0);                     // Start at top-left corner
  
  display.print("Temperature: ");
  display.print(bme.readTemperature(), 1);       // if using dht: print(temperatureReading);
  display.println(" C");
  parameter = "Temperature (C)";
  reading = bme.readTemperature();
  sendReadings(parameter, reading);

  display.print("Pressure: ");
  display.print(bme.readPressure() / 100.0F, 1);
  display.println(" hPa");
  parameter = "Pressure (hPa)";
  reading = bme.readPressure() / 100.0F;
  sendReadings(parameter, reading);

  display.print("Humidity: ");
  display.print(bme.readHumidity(), 2);          // if using dht: print(humidityReading);
  display.println(" %");
  parameter = "Humidity (%)";
  reading = bme.readHumidity();
  sendReadings(parameter, reading);

  getLocalTime(&timeinfo);
  display.print(&timeinfo, "%B %d %Y %H:%M");
  display.display();
  readTime = String(timeinfo.tm_mday)+" "+String(timeinfo.tm_mon+1)+" "+String(timeinfo.tm_hour)+":00";
}

void sendReadings(String parameter, int reading) {

  Serial.println("------------------------------------");
  Serial.println("Update test...");

  for (uint8_t i = 0; i < 5; i++) {

    json.set(parameter, reading);
    
    if (Firebase.updateNode(fbdo, path + readTime, json)) {
      Serial.println("PASSED");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
      //No ETag available
      Serial.print("VALUE: ");
      printResult(fbdo);
      Serial.println("------------------------------------");
      Serial.println();
    }
    else {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
      Serial.println("------------------------------------");
      Serial.println();
    }
  }
}

void printResult(FirebaseData &data) {

  if (data.dataType() == "json") {
    Serial.println();
    FirebaseJson &json = data.jsonObject();
    //Print all object data
    String jsonStr;
    json.toString(jsonStr, true);
    Serial.println(jsonStr);
    Serial.println();
    Serial.println("Iterate JSON data:");
    Serial.println();
    size_t len = json.iteratorBegin();
    String key, value = "";
    int type = 0;
    for (size_t i = 0; i < len; i++) {
      json.iteratorGet(i, type, key, value);
      Serial.print(i);
      Serial.print(", ");
      Serial.print("Type: ");
      Serial.print(type == FirebaseJson::JSON_OBJECT ? "object" : "array");
      if (type == FirebaseJson::JSON_OBJECT)
      {
        Serial.print(", Key: ");
        Serial.print(key);
      }
      Serial.print(", Value: ");
      Serial.println(value);
    }
    json.iteratorEnd();
  }
  
  else {          // this block should not happen if data formatted properly
    Serial.println(data.payload());
    Serial.println("\nERROR!\nDatatype not json");
    for(;;);      // loop forever
  }
}
