int sensorPin1 = D2;
int sensorPin2 = D4;
volatile long pulse1;
volatile long pulse2;
unsigned long lastTime;
float volume1;
float volume2;

void setup() {
  pinMode(sensorPin1, INPUT);
  pinMode(sensorPin2, INPUT);
  Serial.begin(9600);
  attachInterrupt(digitalPinToInterrupt(sensorPin1), increase, RISING);
  attachInterrupt(digitalPinToInterrupt(sensorPin2), increase, RISING);
}
void loop() {
  volume1 = 2.663 * pulse1 / 1000 * 30;
  volume2 = 2.663 * pulse2 / 1000 * 30;

  if (millis() - lastTime > 2000) {
    pulse1 = 0;
    pulse2 = 0;
    lastTime = millis();
  }
  Serial.print(volume1);
  Serial.println(" L/m");
  Serial.print(volume2);
  Serial.println(" L/m");
}

ICACHE_RAM_ATTR void increase() {
  pulse1++;
  pulse2++;
}