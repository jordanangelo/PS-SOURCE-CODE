#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <WebSocketsServer.h>

AsyncWebServer server(8012);
WebSocketsServer webSocket(8013);

#define trigPinGH 4 // Trigger Pin1 - OUTPUT - Greenhouse
#define echoPinGH 5 // Echo Pin1 - INPUT - Greenhouse
#define trigPinAQ 6 // Trigger Pin2 - OUTPUT - Aquaponics
#define echoPinAQ 10 // Echo Pin2 - INPUT - Aquaponics

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
int relayStates[NUM_RELAYS] = {0};

SoftwareSerial mySerial(2, 3);  // RX, TX

const char* ssid = "SSID";
const char* password = "PASSWORD";
const char *PARAM_INPUT_1 = "relay";
const char *PARAM_INPUT_2 = "state";
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Water Supply Control</title>
  <style>
    body {
      font-family: Arial, sans-serif;
      text-align: center;
      max-width: 600px;
      margin: 20px auto;
      padding-bottom: 25px;
    }

    h2 {
      font-size: 2rem;
      margin-bottom: 20px;
    }

    .switch-container {
      display: flex;
      align-items: center;
      justify-content: space-between;
    }

    .switch {
      position: relative;
      display: inline-block;
      width: 60px;
      height: 34px;
      margin-bottom: 10px;
    }

    .switch input {
      display: none;
    }

    .slider {
      position: absolute;
      top: 0;
      left: 0;
      right: 0;
      bottom: 0;
      background-color: #a30000;
      border-radius: 17px;
      cursor: pointer;
      transition: background-color 0.4s;
    }

    .slider:before {
      position: absolute;
      content: "";
      height: 26px;
      width: 26px;
      left: 4px;
      bottom: 4px;
      background-color: #ffffff;
      border-radius: 50%;
      transition: transform 0.4s;
    }

    input:checked + .slider {
      background-color: #21f32c;
    }

    input:checked + .slider:before {
      transform: translateX(26px);
    }

    .status {
      font-size: 1rem;
      margin-top: 10px;
    }

    label {
      display: flex;
      flex-direction: column;
      align-items: center;
    }
  </style>
</head>
<body>
  <h2>Water Supply Control</h2>
  <br><br><br><br>
  <div class="switch-container">
    <div>
      <label>
        <h3>Greenhouse</h3>
        <div class="switch">
          <input type="checkbox" onchange="toggleCheckbox(1)" id="1">
          <span class="slider"></span>
        </div>
        <div class="status" id="status1">Status: Off</div>
      </label>
    </div>

    <div>
      <label>
        <h3>Aquaponics</h3>
        <div class="switch">
          <input type="checkbox" onchange="toggleCheckbox(2)" id="2">
          <span class="slider"></span>
        </div>
        <div class="status" id="status2">Status: Off</div>
      </label>
    </div>
  </div>

   <script>
    // Establish WebSocket connection
    var socket = new WebSocket("ws://" + window.location.hostname + ":8013/");

    // WebSocket event listener
    socket.onmessage = function (event) {
      var data = event.data;
      var relayNum = parseInt(data.charAt(0));
      var newState = parseInt(data.charAt(1));

      // Update status display based on WebSocket message
      var statusElement = document.getElementById("status" + relayNum);
      statusElement.textContent = "Status: " + (newState ? "On" : "Off");

      // Update switch state based on WebSocket message
      var checkbox = document.getElementById(relayNum);
      checkbox.checked = newState;
    };

    // Function to toggle checkbox and send WebSocket message
    function toggleCheckbox(buttonId) {
      var checkbox = document.getElementById(buttonId);
      var statusElement = document.getElementById("status" + buttonId);
      var state = checkbox.checked ? 1 : 0;

      socket.send(buttonId + state); // Send WebSocket message

      statusElement.textContent = "Status: " + (checkbox.checked ? "On" : "Off"); // Update status display
    }
  </script>
</body>
</html>
)rawliteral";

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

// Replaces placeholder with button
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

void handleWebSocketMessage(uint8_t num, uint8_t *payload, size_t length) {
  if (length == 2) {
    int relayNum = payload[0] - '0';  // Convert ASCII to integer
    int newState = payload[1] - '0';
    if (relayNum >= 1 && relayNum <= NUM_RELAYS) {
      digitalWrite(relayGPIOs[relayNum - 1], (RELAY_NO) ? !newState : newState);
      relayStates[relayNum - 1] = newState;

      // Create a named variable for the message and then broadcast it
      String message = String(relayNum) + String(newState);
      webSocket.broadcastTXT(message);
    }
  }
}

void setup() {
  Serial.begin(9600);
  mySerial.begin(9600);
  webSocket.begin();
  webSocket.onEvent([](uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
    if (type == WStype_TEXT) {
      handleWebSocketMessage(num, payload, length);
    }
  });

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
                // Modify relay control logic based on buttonId
                int buttonId = inputMessage.toInt();
                if ((buttonId == 1) || (buttonId == 2)) {
                  // Button 1 controls Relay 1 and Relay 3
                  // Button 2 controls Relay 2 and Relay 3
                  digitalWrite(relayGPIOs[buttonId - 1], RELAY_NO ? !inputMessage2.toInt() : inputMessage2.toInt());
                  digitalWrite(relayGPIOs[2], RELAY_NO ? !inputMessage2.toInt() : inputMessage2.toInt());
                }
              }

              request->send(200, "text/plain", "OK");
            });

  // Start server
  server.begin();
}

void waterLevelAQ();
void waterLevelGH();
void loop() {
  webSocket.loop();
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

