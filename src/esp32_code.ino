#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <Ultrasonic.h>
#include <FlowSensor.h>

#define relay3 25 // water pump
#define relay2 27 // solenoid - GH
#define relay1 26 // solenoid - AQ

#define type YFS201
#define pin 32 // D32 - ESP32 - Luma - Bago - 33
FlowSensor Sensor(type, pin);

Ultrasonic ultrasonic1(19, 21);
Ultrasonic ultrasonic2(22, 23);

const char* ssid = "Jordan";
const char* password = "jordanangelo";

const char* MQTT_username = ""; 
const char* MQTT_password = ""; 

const char* mqtt_server = "172.20.10.14";

WiFiClient espClient;
PubSubClient client(espClient);

bool relay3State = HIGH;

// Timers auxiliar variables
long now;
long lastMeasure = 0;
long now2;
long lastMeasure2 = 0;
long now3;
long lastMeasure3 = 0;
unsigned long currentMillis;
unsigned long previousMillis = 0;

float distance;
float level;

// connect ESP32 to the router
void setup_wifi() {
  delay(10);
  // start by connecting to a WiFi network
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

// This function is executed when some device publishes a message to a topic that your ESP32 is subscribed to
// Change the function below to add logic to your program, so when a device publishes a message to a topic that 
// your ESP32 is subscribed you can actually do something
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

  // If a message is received on the topics, check if the message is either on or off. Turns the relay GPIO according to the message
  if (topic == "relay/control1/set") {
    Serial.print("Changing Relay 1 to ");
    if (messageTemp == "on") {
      digitalWrite(relay1, HIGH);
      Serial.print("On");
    } 
    else if (messageTemp == "off") {
      digitalWrite(relay1, LOW);
    }
  }
  if (topic == "relay/control2/set") {
    Serial.print("Changing Relay 2 to ");
    if (messageTemp == "on") {
      digitalWrite(relay2, HIGH);
      Serial.print("On");
    }
    else if (messageTemp == "off") {
      digitalWrite(relay2, LOW);
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

// This functions reconnects your ESP8266 to your MQTT broker
// Change the function below if you want to subscribe to more topics with your ESP8266 
void reconnect() {
  // Loop until reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");

    // Attempt to connect
    if (client.connect("ESP_WATER", MQTT_username, MQTT_password)) {
      Serial.println("connected");

      // Subscribe or resubscribe to a topic
      client.subscribe("relay/control1/set");
      client.subscribe("relay/control2/set");
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

void IRAM_ATTR count() {
  Sensor.count();
}

void setup() {
  pinMode(relay1, OUTPUT);
  pinMode(relay2, OUTPUT);
  pinMode(relay3, OUTPUT);
  digitalWrite(relay1, HIGH);
  digitalWrite(relay2, HIGH);
  digitalWrite(relay3, HIGH);
  
  Serial.begin(115200);
  while(!Serial) continue;
  Sensor.begin(count);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void distanceFunc() {
  distance = 180 - (ultrasonic1.read() + ultrasonic2.read());
  level = (distance / 180) * 100; // level in percentage
  Serial.print("Water Level: ");
  Serial.print(level);
  Serial.println("%");
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  if(!client.loop())
    client.connect("ESP_WATER", MQTT_username, MQTT_password);

  // Publishes new level readings every 6 seconds
  now = millis();
  if (now - lastMeasure >= 6000) {
    distanceFunc();
    lastMeasure = now;
    // Check if any reads failed and exit early (to try again).
    if (isnan(level)) {
      Serial.println("Failed to read level!");
      return;
    }

    client.publish("distance/level", String(level).c_str());
    Serial.print("Level: ");
    Serial.print(level);
    Serial.println("%");
  }
  
  // publish alert if water level is below 20%
  now2= millis();
  if (now2 - lastMeasure2 >= 300000) {
    lastMeasure2 = now2;
    if (level < 20) {
      client.publish("level/alert", String(level).c_str());
    }
  }

  // publish flowrate reading every 1 second
  now3= millis();
  if (now3 - lastMeasure3 >= 1000) {
    Sensor.read();
    lastMeasure3 = now3;
    float flowrate = Sensor.getFlowRate_m();

    // Check if any reads failed and exit early (to try again).
    if (isnan(flowrate)) {
      Serial.println("Failed to read flowrate!");
      return;
    }

    client.publish("flow/rate", String(flowrate).c_str());
    Serial.print("Flow Rate: ");
    Serial.print(flowrate);
    Serial.println("L/min");
  }
}
