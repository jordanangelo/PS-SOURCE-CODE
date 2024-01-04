#include <SoftwareSerial.h>
#include <Wire.h>
#include <Ultrasonic.h>
#include <ArduinoJson.h>

// Arduino Rx=9, Tx=10
SoftwareSerial nodemcu(9, 10);

// (Trig, Echo)
Ultrasonic ultrasonic1(5, 6); 
Ultrasonic ultrasonic2(7, 8);

float distance;
float level;
long now = millis();
long lastMeasure = 0;

void setup() {
  Serial.begin(115200);
  nodemcu.begin(9600);
  delay(500);
  Serial.println("Program started");
}

void loop() {
  if (now - lastMeasure > 500) {
    lastMeasure = now;
    StaticJsonDocument<1000> doc;
    distanceFunc();

    // assign data to a JSON object
    doc["level"] = level;

    serializeJson(doc, nodemcu);
  }
}

void distanceFunc() {
  distance = 180 - (ultrasonic1.read() + ultrasonic2.read());
  level = (distance / 180) * 100; // level in percentage
  Serial.print("Water Level: ");
  Serial.print(level);
  Serial.println("%");
}