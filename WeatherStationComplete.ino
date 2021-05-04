/*
Three things to do!
1. Put your WiFi details into the code
2. Put the Firebase access code that I gave you into the code
3. Put your name into the path string
*/

#include <WiFi.h>
#include <FirebaseESP32.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "time.h"

#define BME_SCK 5           // SCK to SCK
#define BME_MISO 19         // SDO to MISO
#define BME_MOSI 18         // SDI to MOSI
#define BME_CS 32           // CS to Digital Pin 0
#define LED 14              // optional LED
#define SCREEN_WIDTH 128    // OLED display width, in pixels
#define SCREEN_HEIGHT 32    // OLED display height, in pixels
#define OLED_RESET    32    // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C // See datasheet for Address; 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Adafruit_BME280 bme(BME_CS, BME_MOSI, BME_MISO, BME_SCK); // software SPI

// WiFi and database details
#define WIFI_SSID "WIFI"
#define WIFI_PASSWORD "PASSWORD"
#define FIREBASE_HOST "weatherstatino-default-rtdb.europe-west1.firebasedatabase.app/"
#define FIREBASE_AUTH "SECRET_CODE"

// Define FirebaseESP32 data object
FirebaseData fbdo;
FirebaseJson json;

const char* ntpServer = "pool.ntp.org";   // NTP server for time data 
const int gmtOffset_sec = 3600;           // GMT+1
struct tm timeinfo;                       // Create time structure
String path = "/Maks/Readings ";          // Save data to directory; NAME YOUR DIRECTORY WITH YOUR NAME
String readTime;

// Define functions
void printReadings();
void printResult(FirebaseData &data);
void sendReadings(String parameter, int reading);

void setup() {

  // Initialise LED and serial
  pinMode(LED, OUTPUT);
  digitalWrite(LED, HIGH);
  Serial.begin(1000000);
  while(!Serial);       // Get the serial running
  delay(1000);          // Wait before accessing Sensor
  digitalWrite(LED, LOW);

  // Initialise BME Sensor
  unsigned status;
  status = bme.begin(0x76, &Wire);

  // Initialise OLED display
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);          // Don't proceed, loop forever
  }
  display.display();
  delay(500);         // Pause for 0.5 second

  display.clearDisplay();               // Clear the buffer
  display.setTextSize(1);               // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE);  // Draw white text
  display.setCursor(0,0);               // Start at top-left corner

  // Draw a single pixel in white
  display.drawPixel(10, 10, SSD1306_WHITE);
  display.display();
  delay(500);
  display.clearDisplay();   // Clear the buffer

  // Connect to WiFi
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
  display.println(WiFi.localIP());  // Display the system's IP
  display.display();

  // Initialise time
  configTime(gmtOffset_sec, 0, ntpServer);
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
  // Get time and display
  readTime = String(timeinfo.tm_mday)+" "+String(timeinfo.tm_mon+1)+" "+String(timeinfo.tm_hour)+":00";
  Serial.println(readTime);
  Serial.println(&timeinfo, "%B %d %Y %H:%M");
  display.print(&timeinfo, "%B %d %Y %H:%M");
  display.display();

  // Establish firebase connection
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Firebase.reconnectWiFi(true);
  Firebase.setReadTimeout(fbdo, 1000 * 60);   // Set database read timeout to 1 minute (max 15 minutes)
  Firebase.setwriteSizeLimit(fbdo, "tiny");   // Set size and it's write timeout
  Firebase.setFloatDigits(1);                 // Set the decimal places for float

  digitalWrite(LED, HIGH);
  delay(1000);
}

void loop() {   // Measure and send data every 5 seconds
  printReadings();
  delay(5000);
}

void printReadings() {
  
  String parameter;
  int reading;
  
  display.clearDisplay();                 // Clear the buffer
  display.setTextSize(1);                 // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE);    // Draw white text
  display.setCursor(0,0);                 // Start at top-left corner

  // Display temperature readings on the OLED display and send to the firebase
  display.print("Temperature: ");
  display.print(bme.readTemperature(), 1);
  display.println(" C");
  parameter = "Temperature (C)";
  reading = bme.readTemperature();
  sendReadings(parameter, reading);

  // Display air pressure readings on the OLED display and send to the firebase
  display.print("Pressure: ");
  display.print(bme.readPressure() / 100.0F, 1);
  display.println(" hPa");
  parameter = "Pressure (hPa)";
  reading = bme.readPressure() / 100.0F;
  sendReadings(parameter, reading);

  // Display humidity readings on the OLED display and send to the firebase
  display.print("Humidity: ");
  display.print(bme.readHumidity(), 2);
  display.println(" %");
  parameter = "Humidity (%)";
  reading = bme.readHumidity();
  sendReadings(parameter, reading);

  getLocalTime(&timeinfo);    // Update the clock
  display.print(&timeinfo, "%B %d %Y %H:%M");
  display.display();
  readTime = String(timeinfo.tm_mday)+" "+String(timeinfo.tm_mon+1)+" "+String(timeinfo.tm_hour)+":00";
}

void sendReadings(String parameter, int reading) {

  for (uint8_t i = 0; i < 5; i++) {
    // Format the reading in json dataframe
    json.set(parameter, reading);

    // Check for the proper data format
    if (Firebase.updateNode(fbdo, path + readTime, json)) {
      Serial.println("PASSED");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
      Serial.print("VALUE: ");
      printResult(fbdo);    // Print Result to Serial
      Serial.println();
    }
    else {    // Error! Might happen; If not happening constantly, then nothing to worry
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
      Serial.println();
    }
  }
}

void printResult(FirebaseData &data) {

  if (data.dataType() == "json") {  // Check for JSON dataframe

    String jsonStr;
    FirebaseJson &json = data.jsonObject(); // Create Firebase object

    json.toString(jsonStr, true);   // Convert JSON to String
    
    Serial.println(jsonStr);
    Serial.println();
    Serial.println("Iterate JSON data:");
    Serial.println();

    size_t len = json.iteratorBegin();  // Initialise JSON iterator
    String key, value = "";
    int type = 0;
    // Print JSON Data to Serial
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
  
  else {      // this block should not happen
    Serial.println(data.payload());
    Serial.println("\nERROR!\nDatatype not json");
    for(;;);  // loop forever
  }
}
