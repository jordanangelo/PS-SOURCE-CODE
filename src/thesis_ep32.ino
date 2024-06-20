#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <Ultrasonic.h>

#define relay2 26 // solenoid - GH
#define relay1 25 // solenoid - AQ
#define relay3 27 // water pump

#define pinOld 33 // D32 - ESP32 - Luma - Bago - 33
#define pinNew 14

Ultrasonic ultrasonic1(19, 21);
Ultrasonic ultrasonic2(22, 23);

const char* ssid = "WAPOMA"; // Jordan
const char* password = "DM8LTR5T42D"; // jordanangelo
const char* MQTT_username = "";
const char* MQTT_password = ""; 
const char* mqtt_server = "192.168.254.133"; // 172.20.10.2 // 192.168.254.130:1880/ui

WiFiClient espClient;
PubSubClient client(espClient);

bool relay3State = HIGH;

// Timers auxiliary variables
long now;
long lastMeasure = 0;
long now2;
long lastMeasure2 = 0;
long now3;
long lastMeasure3 = 0;
long now4;
long lastMeasure4 = 0;
int interval = 1000;

// flow variables
float calibrationFactor = 4.5;
volatile byte pulseCount1;
volatile byte pulseCount2;
byte pulse1Sec1 = 0;
byte pulse1Sec2 = 0;
float flowRate1;
float flowRate2;
float totalVolume1;
float totalVolume2;
float volumeTotal;
unsigned int flowMilliLitres1;
unsigned int flowMilliLitres2;
unsigned long totalMilliLitres1;
unsigned long totalMilliLitres2;

// level variables
float distance;
float level;

// connect ESP32 to the router
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

void callback(String topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;

  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();

  if (topic == "relay/control1/set") {
    Serial.print("Changing Relay 1 to ");
    if (messageTemp == "on") {
      digitalWrite(relay1, LOW);
      Serial.print("On");
    } 
    else if (messageTemp == "off") {
      digitalWrite(relay1, HIGH);
    }
  }
  if (topic == "relay/control2/set") {
    Serial.print("Changing Relay 2 to ");
    if (messageTemp == "on") {
      digitalWrite(relay2, LOW);
      Serial.print("On");
    }
    else if (messageTemp == "off") {
      digitalWrite(relay2, HIGH);
    }
  }

  // Update relay3State
  if (digitalRead(relay1) == LOW || digitalRead(relay2) == LOW) {
    relay3State = LOW;
  }
  else {
    relay3State = HIGH;
  }
  digitalWrite(relay3, relay3State); // Update relay3 based on its state
  Serial.println();
}

void reconnect() {
  // Loop until reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");

    if (client.connect("ESP_WATER", MQTT_username, MQTT_password)) {
      Serial.println("connected");

      client.subscribe("relay/control1/set");
      client.subscribe("relay/control2/set");
    }
    else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void IRAM_ATTR pulseCounter1() {
  pulseCount1++;
}

void IRAM_ATTR pulseCounter2() {
  pulseCount2++;
}

void setup() {
  pinMode(relay1, OUTPUT);
  pinMode(relay2, OUTPUT);
  pinMode(relay3, OUTPUT);
  pinMode(pinOld, INPUT_PULLUP);
  pinMode(pinNew, INPUT_PULLUP);
  digitalWrite(relay3, HIGH);

  Serial.begin(115200);
  while(!Serial) continue;
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  pulseCount1 = 0;
  pulseCount2 = 0;
  flowRate1 = 0.0;
  flowRate2 = 0.0;
  flowMilliLitres1 = 0;
  flowMilliLitres2 = 0;
  totalMilliLitres1 = 0;
  totalMilliLitres2 = 0;
  lastMeasure3 = 0;
  attachInterrupt(digitalPinToInterrupt(pinOld), pulseCounter1, FALLING);
  attachInterrupt(digitalPinToInterrupt(pinNew), pulseCounter2, FALLING);
}

void distanceFunc() {
  distance = 212 - (ultrasonic1.read() + ultrasonic2.read());
  level = (distance / 212) * 100; // level in percentage
  /*Serial.print("Water Level: ");
  Serial.print(level);
  Serial.println("%");*/
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  if(!client.loop())
    client.connect("ESP_WATER", MQTT_username, MQTT_password);

  now = millis();
  if (now - lastMeasure >= 60000) {
    distanceFunc();
    lastMeasure = now;
    if (isnan(level)) {
      Serial.println("Failed to read level!");
      return;
    }

    client.publish("distance/level", String(level).c_str());
    /*Serial.print("Level: ");
    Serial.print(level);
    Serial.println("%");*/
  }
  
  now2 = millis();
  if (now2 - lastMeasure2 >= 300000) {
    lastMeasure2 = now2;
    if (level < 20) {
      client.publish("level/alert", String(level).c_str());
    }
  }

  now3 = millis();
  if (now3 - lastMeasure3 >= interval) {    
    pulse1Sec1 = pulseCount1;
    pulse1Sec2 = pulseCount2;
    pulseCount1 = 0;
    pulseCount2 = 0;
    
    flowRate1 = ((1000.0 / (millis() - lastMeasure3)) * pulse1Sec1) / calibrationFactor;
    flowRate2 = ((1000.0 / (millis() - lastMeasure3)) * pulse1Sec2) / calibrationFactor;
    lastMeasure3 = now3;
    
    totalVolume1 += flowRate1 / 60.00;
    totalVolume2 += flowRate2 / 60.00;
    volumeTotal = totalVolume1 + totalVolume2;
    
    if (level > 99) {
      volumeTotal = 0;
    }
    if (isnan(flowRate1)) {
      Serial.println("Failed to read flowrate!");
      return;
    }
    if (flowRate1 < 0 || flowRate1 > 30) {
      flowRate1 = 0;
    }
    if (isnan(flowRate2)) {
      Serial.println("Failed to read flowrate!");
      return;
    }
    if (flowRate2 < 0 || flowRate2 > 30) {
      flowRate2 = 0;
    }

    client.publish("flow/rate1", String(flowRate1).c_str());
    client.publish("flow/rate2", String(flowRate2).c_str());
  }

  now4 = millis();
  if (now4 - lastMeasure4 > 30000) {
    client.publish("flow/volume", String(volumeTotal).c_str());
  }
}