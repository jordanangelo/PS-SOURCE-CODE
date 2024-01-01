#include <SoftwareSerial.h>
#include <FlowSensor.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>

#define trigPinGH 4 // Trigger Pin1 - OUTPUT - Greenhouse
#define echoPinGH 5 // Echo Pin1 - INPUT - Greenhouse
#define trigPinAQ 6 // Trigger Pin2 - OUTPUT - Aquaponics
#define echoPinAQ 10 // Echo Pin2 - INPUT - Aquaponics

// flowrate sensor
#define type YFS201
#define pin 3
FlowSensor Sensor(type, pin); // configure flowrate sensor
unsigned long timeBefore = 0;

long durationGH;
long durationAQ;
float distanceAQ;
float distanceGH;
float totalAQ;
float totalGH;
float averageGH;
float averageAQ;
float readingsGH[10];
float readingsAQ[10];

#define RELAY_NO true // Define Relay as Normally Open (NO)
#define NUM_RELAYS 3 // Set number of relays

int relayGPIOs[NUM_RELAYS] = {8, 9, 10}; // 7 - solenoidGH, 8 - solenoidAQ, 9 - pump

SoftwareSerial mySerial(2, 3);  // RX, TX

const char* ssid = "your_SSID";
const char* password = "your_PASSWORD";
const char *PARAM_INPUT_1 = "relay";
const char *PARAM_INPUT_2 = "state";

AsyncWebServer server(80);

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    html {font-family: Arial; display: inline-block; text-align: center;}
    h2 {font-size: 3.0rem;}
    p {font-size: 3.0rem;}
    body {max-width: 600px; margin:0px auto; padding-bottom: 25px;}
    .switch {position: relative; display: inline-block; width: 120px; height: 68px}
    .switch input {display: none}
    .slider {position: absolute; top: 0; left: 0; right: 0; bottom: 0; background-color: #ccc; border-radius: 34px}
    .slider:before {position: absolute; content: ""; height: 52px; width: 52px; left: 8px; bottom: 8px; background-color: #fff; -webkit-transition: .4s; transition: .4s; border-radius: 68px}
    input:checked+.slider {background-color: #2196F3}
    input:checked+.slider:before {-webkit-transform: translateX(52px); -ms-transform: translateX(52px); transform: translateX(52px)}
  </style>
</head>
<body>
  <h2>Water Supply</h2>
  %BUTTONPLACEHOLDER%
<script>function toggleCheckbox(element) {
  var xhr = new XMLHttpRequest();
  if(element.checked){ xhr.open("GET", "/update?relay="+element.id+"&state=1", true); }
  else { xhr.open("GET", "/update?relay="+element.id+"&state=0", true); }
  xhr.send();
}</script>
</body>
</html>
)rawliteral";

// Replaces placeholder with button section in your web page
String processor(const String &var) {
  if (var == "BUTTONPLACEHOLDER") {
    String buttons = "";
    for (int i = 1; i <= NUM_RELAYS; i++) {
      String relayStateValue = relayState(i);
      buttons += "<h4>Relay #" + String(i) + " - GPIO " + relayGPIOs[i - 1] + "</h4><label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\"" + String(i) + "\" " + relayStateValue + "><span class=\"slider\"></span></label>";
    }
    return buttons;
  }
  return String();
}

String relayState(int numRelay) {
  if (RELAY_NO) {
    if (digitalRead(relayGPIOs[numRelay - 1])) {
      return "";
    }
    else {
      return "checked";
    }
  }
  else {
    if (digitalRead(relayGPIOs[numRelay - 1])) {
      return "checked";
    }
    else {
      return "";
    }
  }
  return "";
}

void count() {
	Sensor.count();
}

void setup() {
  Serial.begin(9600);
  mySerial.begin(9600);
  Sensor.begin(count);

  // Set all relays to off when the program starts
  for (int i = 1; i <= NUM_RELAYS; i++) {
    pinMode(relayGPIOs[i - 1], OUTPUT);
    if (RELAY_NO) {
      digitalWrite(relayGPIOs[i - 1], HIGH);
    }
    else {
      digitalWrite(relayGPIOs[i - 1], LOW);
    }
  }

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  Serial.println(WiFi.localIP());
  Serial.println("Connected to WiFi");

  pinMode(trigPinGH, OUTPUT);
  pinMode(echoPinGH, INPUT);
  pinMode(trigPinAQ, OUTPUT);
  pinMode(echoPinAQ, INPUT);

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send_P(200, "text/html", index_html, processor); });

  // Send a GET request to <ESP_IP>/update?relay=<inputMessage>&state=<inputMessage2>
  server.on("/update", HTTP_GET, [](AsyncWebServerRequest *request) {
              String inputMessage;
              String inputParam;
              String inputMessage2;
              String inputParam2;

              if (request->hasParam(PARAM_INPUT_1) & request->hasParam(PARAM_INPUT_2)) {
                inputMessage = request->getParam(PARAM_INPUT_1)->value();
                inputParam = PARAM_INPUT_1;
                inputMessage2 = request->getParam(PARAM_INPUT_2)->value();
                inputParam2 = PARAM_INPUT_2;

                if (RELAY_NO) {
                  digitalWrite(relayGPIOs[inputMessage.toInt() - 1], !inputMessage2.toInt());
                }
                else {
                  digitalWrite(relayGPIOs[inputMessage.toInt() - 1], inputMessage2.toInt());
                }
              }

              request->send(200, "text/plain", "OK");
            });

  // Start server
  server.begin();
}

void loop() {
  waterLevelGH();
  waterLevelAQ();
  Serial.print("Tank 1: ");
  Serial.println(averageGH);
  Serial.print("Tank 2: ");
  Serial.print(averageAQ);
  delay(5000);
}

void waterLevelGH() {
  totalGH = 0;
  for (int i = 0; i < 10; i++) {
    digitalWrite(trigPinGH, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPinGH, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPinGH, LOW);

    durationGH = pulseIn(echoPinGH, HIGH);
    distanceGH = durationGH * 0.034 / 2; // calculate distance in cm

    if (distanceGH > 0 && distanceGH < 100) {
      readingsGH[i] = distanceGH;
      totalGH += distanceGH;
    }
  }
  averageGH = 86 - (totalGH / 10);
}

void waterLevelAQ() {
  totalAQ = 0;
  for (int j = 0; j < 10; j++) {
    digitalWrite(trigPinAQ, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPinAQ, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPinAQ, LOW);
    
    durationAQ = pulseIn(echoPinAQ, HIGH);
    distanceAQ = durationAQ * 0.034 / 2; // calculate distance in cm

    if (distanceAQ > 0 && distanceAQ < 100) {
      readingsAQ[j] = distanceAQ;
      totalAQ += distanceAQ;
    }
  }
  averageAQ = 86 - (totalAQ / 10);
}