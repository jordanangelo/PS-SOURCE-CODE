#include <Arduino.h>
#include <WiFi.h>
#include <Wire.h>
#include <PubSubClient.h>
#include <ZMPT101B.h>

#define ACPin1 A4
#define ACPin2 A5 
#define ACPin3 A6 
#define ACPin4 A7

#define ACTectionRange 20; //set Non-invasive AC Current Sensor tection range
#define VREF 3.3

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

float readACCurrentValue1() {
  float ACCurrtntValue1 = 0;
  float peakVoltage1 = 0;  
  float voltageVirtualValue1 = 0;  //Vrms
  for (int i = 0; i < 5; i++) {
    peakVoltage1 += analogRead(ACPin1);
    delay(1);
  }
  peakVoltage1 = peakVoltage1 / 5;   
  voltageVirtualValue1 = peakVoltage1 * 0.707;

  /*The circuit is amplified by 2 times, so it is divided by 2.*/
  voltageVirtualValue1 = (voltageVirtualValue1 / 1024 * VREF ) / 2;  

  ACCurrtntValue1 = voltageVirtualValue1 * ACTectionRange;
  return ACCurrtntValue1;
}

float readACCurrentValue2() {
  float ACCurrtntValue2 = 0;
  float peakVoltage2 = 0;  
  float voltageVirtualValue2 = 0;  //Vrms
  for (int i = 0; i < 5; i++) {
    peakVoltage2 += analogRead(ACPin2);
  }
  peakVoltage2 = peakVoltage2 / 5;   
  voltageVirtualValue2 = peakVoltage2 * 0.707;

  /*The circuit is amplified by 2 times, so it is divided by 2.*/
  voltageVirtualValue2 = (voltageVirtualValue2 / 1024 * VREF ) / 2;  

  ACCurrtntValue2 = voltageVirtualValue2 * ACTectionRange;
  return ACCurrtntValue2;
}

float readACCurrentValue3() {
  float ACCurrtntValue3 = 0;
  float peakVoltage3 = 0;  
  float voltageVirtualValue3 = 0;  //Vrms
  for (int i = 0; i < 5; i++) {
    peakVoltage3 += analogRead(ACPin3);
    delay(1);
  }
  peakVoltage3 = peakVoltage3 / 5;   
  voltageVirtualValue3 = peakVoltage3 * 0.707;

  /*The circuit is amplified by 2 times, so it is divided by 2.*/
  voltageVirtualValue3 = (voltageVirtualValue3 / 1024 * VREF ) / 2;  

  ACCurrtntValue3 = voltageVirtualValue3 * ACTectionRange;
  return ACCurrtntValue3;
}

float readACCurrentValue4() {
  float ACCurrtntValue4 = 0;
  float peakVoltage4 = 0;  
  float voltageVirtualValue4 = 0;  //Vrms
  for (int i = 0; i < 5; i++) {
    peakVoltage4 += analogRead(ACPin4);
    delay(1);
  }
  peakVoltage4 = peakVoltage4 / 5;   
  voltageVirtualValue4 = peakVoltage4 * 0.707;

  /*The circuit is amplified by 2 times, so it is divided by 2.*/
  voltageVirtualValue4 = (voltageVirtualValue4 / 1024 * VREF ) / 2;  

  ACCurrtntValue4 = voltageVirtualValue4 * ACTectionRange;
  return ACCurrtntValue4;
}

void setup() {
  Serial.begin(115200);

  voltageSensor1.setSensitivity(SENSITIVITY_1);
  voltageSensor2.setSensitivity(SENSITIVITY_2);
  voltageSensor3.setSensitivity(SENSITIVITY_3);
  voltageSensor4.setSensitivity(SENSITIVITY_4);

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
    float ACCurrentValue1 = readACCurrentValue1();
    float ACCurrentValue2 = readACCurrentValue2();
    float ACCurrentValue3 = readACCurrentValue3();
    float ACCurrentValue4 = readACCurrentValue4();
    float apparentPower_1 = ACCurrentValue1 * voltageSensor1.getRmsVoltage();
    float apparentPower_2 = ACCurrentValue2 * voltageSensor2.getRmsVoltage();
    float apparentPower_3 = ACCurrentValue3 * voltageSensor3.getRmsVoltage();
    float apparentPower_4 = ACCurrentValue4 * voltageSensor4.getRmsVoltage();
    totalApparent = apparentPower_1 + apparentPower_2 + apparentPower_3 + apparentPower_4;
    float maxCurrent = ACCurrentValue1 + ACCurrentValue2 + ACCurrentValue3 + ACCurrentValue4;
   
    if (isnan(apparentPower_1) || isnan(apparentPower_2) || isnan(apparentPower_3) || isnan(apparentPower_4) || isnan(totalApparent)) {
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
    Serial.println("amps");
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
