#include <Wire.h>
#include <cmath>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <vector>

#include <WiFi.h>
#include <HTTPClient.h>


const char* ssid = "Galaxy A15 5G 5F49";
const char* password = "y3425ygu";

// ThingSpeak settings
const char* thingSpeakServer = "http://api.thingspeak.com/update";
const char* apiKey = "*****";
unsigned long channelID = 3089109;

// ThingSpeak timing
unsigned long lastThingSpeakUpdate = 0;
const unsigned long thingSpeakInterval = 5000;

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_I2C_ADDRESS 0x3C 

Adafruit_SH1106G display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

std::vector<int> sensorValues;
int counter = 0;
int flicker_cycle = 0;
bool t = false;
String state[] = {"Street Light OFF", "Street Light ON", "Street Light FLICKER"};
int status_code = 0;

void setup() {
  // Serial.begin(9600);

  Serial.begin(115200);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi..");
  }

  Serial.println("Connected to the WiFi network");

  // Wire.begin(21, 22);
  
  if(!display.begin(OLED_I2C_ADDRESS)) {
    Serial.println(F("SH1106 allocation failed"));
    for(;;);
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(0,0);
  display.println(F("Hello, Arduino!"));
  display.display();
}

double standard_deviation(const std::vector<int>& sv) {
  if (sv.empty()) return 0;

  double mean = 0;
  for (size_t j = 0; j < sv.size(); j++) {
    mean += sv[j];
  }
  mean /= sv.size();

  double variance = 0;
  for (size_t j = 0; j < sv.size(); j++) {
    variance += pow(sv[j] - mean, 2);
  }
  variance /= sv.size();

  return sqrt(variance);
}

void sendToThingSpeak(int lightLevel, int stdDev, int status) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    
    // Create the URL with parameters
    String url = String(thingSpeakServer) + "?api_key=" + String(apiKey) + 
                 "&field1=" + String(lightLevel) +
                 "&field2=" + String(stdDev) +
                 "&field3=" + String(status);
    
    Serial.println("Sending to ThingSpeak: " + url);
    
    http.begin(url);
    int httpCode = http.GET();
    
    if (httpCode > 0) {
      String payload = http.getString();
      Serial.println("ThingSpeak Response: " + payload);
      
      // Show success on display
      display.setCursor(0, 55);
      display.print("TS: ");
      display.print(httpCode == 200 ? "OK" : "ERR");
    } else {
      Serial.println("ThingSpeak Error: " + String(httpCode));
      display.setCursor(0, 55);
      display.print("TS: FAIL");
    }
    
    http.end();
  } else {
    Serial.println("WiFi not connected!");
    display.setCursor(0, 55);
    display.print("TS: NO WIFI");
  }
}

void loop() {
  if (counter >= 20) {
    t = true;
    counter = 0;
  } else {
    counter++;
  }

  int sensorValue = analogRead(1);
  
  display.clearDisplay();
  display.setCursor(0, 0);

  sensorValues.push_back(sensorValue);

  if (sensorValues.size() > 20) {
    sensorValues.erase(sensorValues.begin());
  }
  int sd = standard_deviation(sensorValues);

  if (sensorValue < 1000 && sd < 200){
    status_code = 0;
  } else if (sensorValue >= 1000 && sd <= 200){
    status_code = 1;
  } else if (sd > 400){
    flicker_cycle += 1;
    if (flicker_cycle > 20){
      status_code = 2;
      flicker_cycle = 0;
    }
  }
  display.println(state[status_code]);

  unsigned long currentTime = millis();
  if (currentTime - lastThingSpeakUpdate >= thingSpeakInterval) {
    sendToThingSpeak(sensorValue, sd, status_code);
    lastThingSpeakUpdate = currentTime;
  } else {
    // Show countdown to next update
    display.setCursor(0, 55);
    display.print("Next: ");
    display.print((thingSpeakInterval - (currentTime - lastThingSpeakUpdate)) / 1000);
    display.print("s");
  }

  Serial.println(sensorValue);

  display.display();
  delay(50);
}
