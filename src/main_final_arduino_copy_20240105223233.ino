#include <SoftwareSerial.h>
#include <Wire.h>
#include <ArduinoJson.h>

#define trigPin1 5
#define echoPin1 6
#define trigPin2 7
#define echoPin2 8

// Arduino Rx=9, Tx=10
SoftwareSerial nodemcu(9, 10);

float distance;
float duration1;
float duration2;
float level;
long now = millis();
long lastMeasure = 0;

void setup() {
  pinMode(echoPin1, INPUT);
  pinMode(echoPin2, INPUT);
  pinMode(trigPin1, OUTPUT);
  pinMode(trigPin2, OUTPUT);
  
  Serial.begin(115200);
  nodemcu.begin(9600);
  delay(500);
  Serial.println("Program started");
}

void loop() {
  if (now - lastMeasure > 1000) {
    lastMeasure = now;
    distanceFunc();
    StaticJsonBuffer<1000> jsonBuffer;
    JsonObject& data = jsonBuffer.createObject();

    // assign data to a JSON object
    data["level"] = level;

    data.printTo(nodemcu);
    jsonBuffer.clear();
  }
}

void distanceFunc() {
  digitalWrite(trigPin1, LOW);
  digitalWrite(trigPin2, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin1, HIGH);
  digitalWrite(trigPin2, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin1, LOW);
  digitalWrite(trigPin2, LOW);

  duration1 = pulseIn(echoPin1, HIGH);
  duration2 = pulseIn(echoPin2, HIGH);
  
  distance = 180 - ((duration1 * 0.034 / 2) + (duration2 * 0.034 / 2));
  level = (distance / 180) * 100; // level in percentage
  Serial.print("Water Level: ");
  Serial.print(level);
  Serial.println("%");
}
