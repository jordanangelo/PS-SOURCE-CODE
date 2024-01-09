#include <Arduino.h>
#include <WiFi.h>
#include <Wire.h>
#include <PubSubClient.h>
#include <ZMPT101B.h>
#include <EmonLib.h>

const int numSamp = 1480;

const char* ssid = "Jordan";
const char* password = "jordanangelo";
const char* MQTT_username = ""; 
const char* MQTT_password = ""; 
const char* mqtt_server = "172.20.10.14";

WiFiClient espClient;
PubSubClient client(espClient);

long now = millis();
long lastMeasure = 0;
long now2 = millis();
long lastMeasure2 = 0;
unsigned long previousMillis = 0;
int sampleCount = 0;
float totalApparent;

#define SENSITIVITY_1 453.25f // change to actual order of sensors
#define SENSITIVITY_2 401.0f
#define SENSITIVITY_3 423.5f
#define SENSITIVITY_4 501.5f
#define ESP32
#define VREF 3.3

ZMPT101B voltageSensor1(A0, 60.0);
ZMPT101B voltageSensor2(A1, 60.0);
ZMPT101B voltageSensor3(A2, 60.0);
ZMPT101B voltageSensor4(A3, 60.0);
EnergyMonitor emon1;
EnergyMonitor emon2;
EnergyMonitor emon3;
EnergyMonitor emon4;

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("WiFi connected - ESP IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  // Loop until reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");

    if (client.connect("ESP_POWER", MQTT_username, MQTT_password)) {
      Serial.println("connected");  
      // Subscribe or resubscribe to a topic
    }
    else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);

  voltageSensor1.setSensitivity(SENSITIVITY_1);
  voltageSensor2.setSensitivity(SENSITIVITY_2);
  voltageSensor3.setSensitivity(SENSITIVITY_3);
  voltageSensor4.setSensitivity(SENSITIVITY_4);
  emon1.current(A4, 20);  
  emon2.current(A5, 20);
  emon3.current(A6, 20);
  emon4.current(A7, 20);

  setup_wifi();
  client.setServer(mqtt_server, 1883);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  if(!client.loop())
    client.connect("ESP_POWER", MQTT_username, MQTT_password);
  now = millis();

  if (now - lastMeasure > 1500) {
    lastMeasure = now;
    float apparentPower_1 = emon1.calcIrms(numSamp) * voltageSensor1.getRmsVoltage();
    float apparentPower_2 = emon2.calcIrms(numSamp) * voltageSensor2.getRmsVoltage();
    float apparentPower_3 = emon3.calcIrms(numSamp) * voltageSensor3.getRmsVoltage();
    float apparentPower_4 = emon4.calcIrms(numSamp) * voltageSensor4.getRmsVoltage();
    totalApparent = apparentPower_1 + apparentPower_2 + apparentPower_3 + apparentPower_4;
    float maxCurrent = emon1.calcIrms(numSamp) + emon2.calcIrms(numSamp) + emon3.calcIrms(numSamp) + emon4.calcIrms(numSamp);
   
    if (isnan(apparentPower_1) || isnan(apparentPower_2 || isnan(apparentPower_3)) || isnan(apparentPower_4) || isnan(totalApparent)) {
      Serial.println("Failed to read from sensor!");
      return;
    }

    client.publish("power/apparent1", String(apparentPower_1).c_str());
    client.publish("power/apparent2", String(apparentPower_2).c_str());
    client.publish("power/apparent3", String(apparentPower_3).c_str());
    client.publish("power/apparent4", String(apparentPower_4).c_str());
    client.publish("power/total", String(totalApparent).c_str());
    client.publish("current/total", String(maxCurrent).c_str());

    Serial.print("Power_1: ");
    Serial.print(apparentPower_1);
    Serial.println("W");
    Serial.print("Power_2: ");
    Serial.print(apparentPower_2);
    Serial.println("W");
    Serial.print("Power_3: ");
    Serial.print(apparentPower_3);
    Serial.println("W");
    Serial.print("Power_4: ");
    Serial.print(apparentPower_4);
    Serial.println("W");
    Serial.print("Total Power: ");
    Serial.print(totalApparent);
    Serial.println("W");
    Serial.print("Max Current: ");
    Serial.print(maxCurrent);
    Serial.print("amps");
  }

  if (now2 - lastMeasure2 > 10000) {
    double energy;
    float totalPower;
    lastMeasure2 = now2;  
    unsigned long currentMillis = millis();

    if (currentMillis - previousMillis >= 1000) {
      previousMillis = currentMillis;
      totalPower += totalApparent;
      sampleCount++;
    }
    if (sampleCount = 10) {
        float averagePower = totalPower / 10;
        energy = averagePower * (10/3600); // energy consumed in 10 seconds
    }

    if (isnan(energy)) {
      Serial.println("Failed!");
      return;
    }
  client.publish("energy/total", String(energy).c_str());
  Serial.print("Energy: ");
  Serial.print(energy);
  Serial.println(" Wh");
  }
}
