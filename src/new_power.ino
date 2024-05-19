#define ACPin1 A4
#define ACPin2 A5 
#define ACPin3 A6 
#define ACPin4 A7

#define ACTectionRange 20;    //set Non-invasive AC Current Sensor tection range
#define VREF 3.3

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
}

void loop() {
  float ACCurrentValue1 = readACCurrentValue1();
  float ACCurrentValue2 = readACCurrentValue2();
  float ACCurrentValue3 = readACCurrentValue3();
  float ACCurrentValue4 = readACCurrentValue4();
}
