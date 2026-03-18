#include <EEPROM.h>
#include <TimerOne.h>

//Analog Inputs
const int soilMoistureSensor = A0;
const int lightSensor = A1;
const int temperatureSensor = A2;

//Digital Inputs
const int buttonPin = 21;

//Digital Outputs
const int drySoilLED = 5;
const int hotTempLED = 6;
const int lowLightLED = 7;

//EEPROM Addresses
const int addrSoil = 0;
const int addrTemp = 4;
const int addrLight = 8;

//Thresholds which are read from EEPROM
int thresholdSoil;
int thresholdTemp;
int thresholdLight;

//Volatile shared data
volatile int lastSoil = 0; //Last collected data of soil moisture
volatile int lastTemp = 0; //Last collected data of temperature
volatile int lastLight = 0; //Last collected data of light
volatile int displayMode = 0; //Three display modes: 0 = humidity, 1 = light, 2 = temperature
volatile bool newData = false;

//Debouncing Variables
volatile unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 200; // 200 ms debounce window

void changeDisplayISR() {
  unsigned long currentTime = millis(); //Stores how long the Arduino program has been running
  if (currentTime - lastDebounceTime > debounceDelay){ //If more than 200 ms has passes since the last time the button has been pressed
    displayMode = (displayMode + 1) % 3; //Change the display (0 = humidity, 1 = light, 2 = temperature)
    lastDebounceTime = currentTime; //Store the time that this button was last debounced
  }
}

void Timer1ISR() {
  int rawSoil = analogRead(soilMoistureSensor);
  int rawLight = analogRead(lightSensor);
  int rawTemp = analogRead(temperatureSensor);

  //Soil Moisture (%V/V)
  float voltageSoil = (rawSoil * 5.0) / 1023.0; //Converts the ADC value to a voltage value
  float percentSoil = (voltageSoil / 5.0) * 100.0; //Converts the voltage value to a percentage
  lastSoil = (int)percentSoil; //Casts the percentage to int

  //Light Sensor (Lux - simulated)
  float voltageLight = (rawLight * 5.0) / 1023.0; //Converts the ADC value to a voltage value
  float lux = (voltageLight / 5.0) * 1000.0; //Converts the voltage value to a lux value (Range: 0-1000)
  lastLight = (int)lux; //Casts the lux value to int

  //Temperature Sensor (°F)
  float voltageTemp = (rawTemp * 5.0) / 1023.0; //Converts the ADC value to a voltage value
  float celsius = voltageTemp * 100.0; //Converts the voltage values to Celsius (0 - 500)
  float fahrenheit = celsius * 9.0 / 5.0 + 32.0; //Converts the Celsius value to Fahrenheit
  lastTemp = (int)fahrenheit; //Casts the Fahrenheit value

  newData = true; //The data has been collected
}

void setup(){
  Serial.begin(9600);

  //Setup pins
  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(drySoilLED, OUTPUT);
  pinMode(hotTempLED, OUTPUT);
  pinMode(lowLightLED, OUTPUT);

  //Write thresholds to EEPROM
  EEPROM.put(addrSoil, 30); //30% moisture
  EEPROM.put(addrTemp, 80); //80° F
  EEPROM.put(addrLight, 200); //200 lux

  //Read thresholds
  EEPROM.get(addrSoil, thresholdSoil);
  EEPROM.get(addrTemp, thresholdTemp);
  EEPROM.get(addrLight, thresholdLight);

  //Timer ISR every 1 second
  Timer1.initialize(1000000);
  Timer1.attachInterrupt(Timer1ISR);

  //Button interrupt to change display
  attachInterrupt(digitalPinToInterrupt(buttonPin), changeDisplayISR, RISING);
}

void loop(){
  if (newData){
    noInterrupts(); //Temporarily disables interrupts to protect shared data
    int soilMoisture = lastSoil; 
    int lightLevel = lastLight;
    int temp = lastTemp;
    newData = false; //The new data has been collected
    interrupts(); //Resumes interrupts

    if(soilMoisture < thresholdSoil){
      digitalWrite(drySoilLED, HIGH); //LED is on when the moisture of the soil is below 30%
    }
    else{
      digitalWrite(drySoilLED, LOW);
    }

    if(lightLevel < thresholdLight){
      digitalWrite(lowLightLED, HIGH); //LED is on when the light level is below 200 lux
    }
    else{
      digitalWrite(lowLightLED, LOW);
    }

    if(temp > thresholdTemp){
      digitalWrite(hotTempLED, HIGH); //LED is on when the temperature of the soil is above 80° F
    }
    else{
      digitalWrite(hotTempLED, LOW);
    }

    switch (displayMode){ //Printing Block for each display mode. Prints the measured data (humidity, light, or temperature), and the threshold value
      case 0:
        Serial.print("Humidity:");
        Serial.print(soilMoisture);
        Serial.print(",");
        Serial.print("Threshold:");
        Serial.println(thresholdSoil);
        break;
      case 1:
        Serial.print("Light:");
        Serial.print(lightLevel);
        Serial.print(",");
        Serial.print("Threshold:");
        Serial.println(thresholdLight);
        break;
      case 2:
        Serial.print("Temperature:");
        Serial.print(temp);
        Serial.print(",");
        Serial.print("Threshold:");
        Serial.println(thresholdTemp);
        break;
    }
  }
}
