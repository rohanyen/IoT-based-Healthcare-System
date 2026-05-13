#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "MAX30100_PulseOximeter.h"
//#include ""

const char* ssid = "Akash's Galaxy A54 5G";
const char* password = "Akash Nidagundi";

String apiKey = "RLSMSS3141KUWAA0";
String server = "http://api.thingspeak.com/update";

#define OLED_SDA 21
#define OLED_SCL 22
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

#define ECG_PIN 34
#define TEMP_SENSOR 35
#define BUZZER_PIN 25
#define LED_PIN 26

OneWire oneWire(TEMP_SENSOR);
DallasTemperature sensors(&oneWire);

PulseOximeter pox;

unsigned long lastMedicineTime = 0;
const unsigned long reminderInterval = 4 * 60 * 60 * 1000;
const int ECG_THRESHOLD = 2000; // Adjust this threshold based on your needs

String getHeartRateStatus(float hr) {
if (hr == 0 || isnan(hr)) return "No Finger Detected";
if (hr < 50) return "Low HR";
if (hr > 120) return "High HR";
return "Normal";
}

String getTimeLeft(unsigned long lastTime, unsigned long interval) {
unsigned long elapsed = millis() - lastTime;
unsigned long timeLeft = interval - elapsed;

if (timeLeft <= 0) return "00:00";

int hours = timeLeft / (60 * 60 * 1000);
int minutes = (timeLeft % (60 * 60 * 1000)) / (60 * 1000);

return (hours < 10 ? "0" : "") + String(hours) + ":" +
(minutes < 10 ? "0" : "") + String(minutes);
}

void setup() {
Serial.begin(115200);
pinMode(BUZZER_PIN, OUTPUT);
pinMode(LED_PIN, OUTPUT);
pinMode(ECG_PIN, INPUT);

Wire.begin();

if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
Serial.println("SSD1306 allocation failed");
while (1);
}
display.clearDisplay();
display.setTextSize(1);
display.setTextColor(WHITE);
display.setCursor(0, 0);
display.println("Initializing...");
display.display();

sensors.begin();

if (!pox.begin()) {
Serial.println("MAX30100 failed to initialize!");
display.clearDisplay();
display.println("HR Sensor Failed!");
display.display();
while (1);
}
pox.setIRLedCurrent(MAX30100_LED_CURR_7_6MA);

WiFi.begin(ssid, password);
while (WiFi.status() != WL_CONNECTED) {
delay(1000);
}
display.clearDisplay();
display.setCursor(0, 10);
display.println("WiFi Connected!");
display.display();
delay(1000);

lastMedicineTime = millis();
}

void loop() {
pox.update();
float heartRate = pox.getHeartRate();
int ecgValue = analogRead(ECG_PIN);

sensors.requestTemperatures();
float temperature = sensors.getTempCByIndex(0);

String hrStatus = getHeartRateStatus(heartRate);
String timeLeft = getTimeLeft(lastMedicineTime, reminderInterval);

if (heartRate > 120 || ecgValue > ECG_THRESHOLD) {
digitalWrite(LED_PIN, HIGH);
digitalWrite(BUZZER_PIN, HIGH);
} else {
digitalWrite(LED_PIN, LOW);
digitalWrite(BUZZER_PIN, LOW);
}

display.clearDisplay();
display.setCursor(0, 10);
display.setTextSize(1);
display.setTextColor(WHITE);
display.println("Heart Rate: " + String(heartRate) + " BPM");
display.println("Status: " + hrStatus);
display.println("ECG: " + String(ecgValue));
display.println("Temp: " + String(temperature) + " C");
display.println("Med in: " + timeLeft);

if (millis() - lastMedicineTime >= reminderInterval) {
display.setCursor(0, 50);
display.println("Take Medicine Now!");
lastMedicineTime = millis();
}

display.display();

if (WiFi.status() == WL_CONNECTED) {
HTTPClient http;
String url = server + "?api_key=" + apiKey +
"&field1=" + String(ecgValue) +
"&field2=" + String(heartRate);

http.begin(url);
int httpResponseCode = http.GET();
http.end();

Serial.print("ThingSpeak Response: ");
Serial.println(httpResponseCode);
}

delay(1000);
}