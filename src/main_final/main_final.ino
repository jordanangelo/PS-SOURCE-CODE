#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Ultrasonic.h>
#include <FlowSensor.h>

#define relay1 7 // solenoid - GH
#define relay2 8 // solenoid - AQ
#define relay3 9 // waterPump

#define type YFS201
#define pin 5 // D1 - ESP8266

FlowSensor Sensor(type, pin);
unsigned long timebefore = 0; // Same type as millis()
unsigned long reset = 0;

// T - E
Ultrasonic ultrasonic1(6, 10); 
Ultrasonic ultrasonic2(4, 5);
// Change the credentials below, so your ESP8266 connects to your router
const char* ssid = "";
const char* password = "";

// MQTT broker credentials (set to NULL if not required)
const char* MQTT_username = ""; 
const char* MQTT_password = ""; 

// Change the variable to your Raspberry Pi IP address, so it connects to your MQTT broker
const char* mqtt_server = "192.168.0.25";

// Initializes the espClient. You should change the espClient name if you have multiple ESPs running in your home automation system
WiFiClient espClient;
PubSubClient client(espClient);

bool relay3State = LOW;

// Timers auxiliar variables
long now = millis();
long lastMeasure = 0;
long now2= millis();
long lastMeasure2 = 0;
long now3= millis();
long lastMeasure3 = 0;

// This functions connects your ESP8266 to your router
void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
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

// This function is executed when some device publishes a message to a topic that your ESP8266 is subscribed to
// Change the function below to add logic to your program, so when a device publishes a message to a topic that 
// your ESP8266 is subscribed you can actually do something
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
  if (digitalRead(relay1) == HIGH || digitalRead(relay2) == HIGH) {
    relay3State = HIGH;
  }
  else {
    relay3State = LOW;
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
    /*
     YOU MIGHT NEED TO CHANGE THIS LINE, IF YOU'RE HAVING PROBLEMS WITH MQTT MULTIPLE CONNECTIONS
     To change the ESP device ID, you will have to give a new name to the ESP8266.
     Here's how it looks:
       if (client.connect("ESP8266Client")) {
     You can do it like this:
       if (client.connect("ESP1_Office")) {
     Then, for the other ESP:
       if (client.connect("ESP2_Garage")) {
      That should solve your MQTT multiple connections problem
    */
    if (client.connect("ESP8266Client", MQTT_username, MQTT_password)) {
      Serial.println("connected");  
      // Subscribe or resubscribe to a topic
      // You can subscribe to more topics (to control more LEDs in this example)
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

// The setup function sets your ESP GPIOs to Outputs, starts the serial communication at a baud rate of 115200
// Sets your mqtt broker and sets the callback function
// The callback function is what receives messages and actually controls the relays
void setup() {
  pinMode(relay1, OUTPUT);
  pinMode(relay2, OUTPUT);
  pinMode(relay3, OUTPUT);
  digitalWrite(relay1, LOW);
  digitalWrite(relay2, LOW);
  digitalWrite(relay3, LOW);
  
  Serial.begin(115200);
  Sensor.begin(count);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

// For this project, you don't need to change anything in the loop function. Basically it ensures that you ESP is connected to your broker
void loop() {
  if (!client.connected()) {
    reconnect();
  }
  if(!client.loop())
    client.connect("ESP8266Client", MQTT_username, MQTT_password);
  now = millis();

  // Publishes new distance every 6 seconds
  if (now - lastMeasure > 6000) {
    lastMeasure = now;
    float distance = 180 - (ultrasonic1.read() + ultrasonic2.read());
    float level = (distance / 180) * 100; // level in percentage

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
  
  // send alert if water level is below 20%
  if (now2 - lastMeasure2 > 300000) {
    lastMeasure2 = now2;
    float distanceCrit = 180 - (ultrasonic1.read() + ultrasonic2.read());
    float levelCrit = (distanceCrit / 180) * 100;
    if (levelCrit < 20) {
      client.publish("level/alert", String(levelCrit).c_str());
    }
  }

  if (now3 - lastMeasure3 > 1000) {
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