#include <Arduino.h>
/*
   LCD RS pin to digital pin 12
   LCD Enable pin to digital pin 11
   LCD D4 pin to digital pin 5
   LCD D5 pin to digital pin 4
   LCD D6 pin to digital pin 3
   LCD D7 pin to digital pin 2
   LCD R/W pin to ground
   LCD VSS pin to ground
   LCD VCC pin to 5V
   10K resistor:
   ends to +5V and ground
   wiper to LCD VO pin (pin 3)
   Relay to pin 13
   AlwaysOnToggle to pin
*/

#include <LiquidCrystal.h>

const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
const int up = 6, down = 7, ok = 8, left = 9, right = 10, relay = 13, alwaysOn = 1;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

#define THERMISTORPIN A0
#define THERMISTORNOMINAL 100000
#define TEMPERATURENOMINAL 25
#define NUMSAMPLES 25
#define BCOEFFICIENT 4066
#define SERIESRESISTOR 4700

uint16_t samples[NUMSAMPLES];

int currentScreen = 0;
int cursorPosition = 0;
int hundreds = 2;
int tens = 0;
int ones = 0;
int setTemp = 200;
boolean firstDisplay = true;
boolean firstAlwaysOn = true;

long sampleTemp();

void setup() {
  pinMode(up, INPUT);
  pinMode(down, INPUT);
  pinMode(left, INPUT);
  pinMode(right, INPUT);
  pinMode(ok, INPUT);
  pinMode(alwaysOn, INPUT);

  pinMode(relay, OUTPUT);

  digitalWrite(relay, LOW);

  analogReference(EXTERNAL);

  lcd.begin(16, 2);
  lcd.print("Solder Oven");
  lcd.setCursor(0, 1);
  lcd.print("By: Zach Goode");
  delay(5000);
  lcd.clear();

  lcd.print("press select");
  lcd.setCursor(0, 1);
  lcd.print("to begin heating");
}

void loop() {
  if (digitalRead(alwaysOn) == HIGH) {
    if (firstAlwaysOn == false) {
      digitalWrite(relay, LOW);
      currentScreen = 0;
      lcd.setCursor(0, 0);
      lcd.clear();
      firstAlwaysOn = true;
      firstDisplay = true;
    }
    if (currentScreen == 0) { //home screen
      if (firstDisplay == true) {
        lcd.clear();
        lcd.print("press select");
        lcd.setCursor(0, 1);
        lcd.print("to begin heating");
        firstDisplay = false;
      }

      if (digitalRead(ok) == HIGH) {
        currentScreen++;
        lcd.clear();
        firstDisplay = true;
        delay(500);
      }
    } else if (currentScreen == 1) { //temperature select scree
      if (firstDisplay == true) {
        lcd.clear();
        lcd.blink();
        lcd.print("Set Temperature");
        lcd.setCursor(0, 1);
        String temp = ((String)hundreds);
        temp += ((String)tens);
        temp += ((String)ones);
        lcd.print(temp + " Celsius");
        lcd.setCursor(0, 1);
        firstDisplay = false;
      }

      if (digitalRead(up) == HIGH) {
        if (cursorPosition == 0) {
          if (hundreds < 4) {
            hundreds++;
          }
        } else if (cursorPosition == 1) {
          if (tens < 9) {
            tens++;
          }
        } else if (cursorPosition == 2) {
          if (ones < 9) {
            ones++;
          }
        }

        lcd.setCursor(0, 1);
        String temp = ((String)hundreds);
        temp += ((String)tens);
        temp += ((String)ones);
        lcd.print(temp + " Celsius");
        lcd.setCursor(cursorPosition, 1);

        delay(250);
      }

      if (digitalRead(down) == HIGH) {
        if (cursorPosition == 0) {
          if (hundreds > 0) {
            hundreds--;
          }
        } else if (cursorPosition == 1) {
          if (tens > 0) {
            tens--;
          }
        } else if (cursorPosition == 2) {
          if (ones > 0) {
            ones--;
          }
        }

        lcd.setCursor(0, 1);
        String temp = ((String)hundreds);
        temp += ((String)tens);
        temp += ((String)ones);
        lcd.print(temp + " Celsius");
        lcd.setCursor(cursorPosition, 1);

        delay(250);
      }

      if (digitalRead(left) == HIGH) {
        if (cursorPosition > 0) {
          cursorPosition--;
          lcd.setCursor(cursorPosition, 1);
        }
        delay(250);
      }

      if (digitalRead(right) == HIGH) {
        if (cursorPosition < 2) {
          cursorPosition++;
          lcd.setCursor(cursorPosition, 1);
        }
        delay(250);
      }

      if (digitalRead(ok) == HIGH) {
        setTemp = (hundreds * 100) + (tens * 10) + (ones);
        currentScreen++;
        lcd.noBlink();
        lcd.clear();
        firstDisplay = true;
        delay(500);
      }
    } else if (currentScreen == 2) { //heating in progress
      long tempTemp = sampleTemp();

      if (firstDisplay == true) {
        digitalWrite(relay, HIGH);
        lcd.clear();
        lcd.print("Target Temp: " + String(setTemp));
        lcd.setCursor(0, 1);
        firstDisplay = false;
        lcd.print("Temp now: " + String(tempTemp));
      }

      if (setTemp <= tempTemp) {
        digitalWrite(relay, LOW);
        currentScreen++;
        firstDisplay = true;
        lcd.clear();
        delay(100);
      }

      delay(100);

      lcd.setCursor(10, 1);
      lcd.print(String(tempTemp) + "       ");
    } else if (currentScreen == 3) { //done
      long tempTemp = sampleTemp();
      if (firstDisplay == true) {
        lcd.clear();
        lcd.print("Cooling Down");
        lcd.setCursor(0, 1);
        firstDisplay = false;
        lcd.print("Temp: " + String(tempTemp));
      }

      if (50 > tempTemp) {
        currentScreen = 0;
        lcd.clear();
        firstDisplay = true;
        delay(500);
      }

      delay(100);

      lcd.setCursor(6, 1);
      lcd.print(String(tempTemp) + "              ");
    }
  } else {
    if (firstAlwaysOn == true) {
      lcd.clear();
      digitalWrite(relay, HIGH);
      currentScreen = 0;
      lcd.setCursor(0, 0);
      lcd.print("Always On Mode");
      firstAlwaysOn = false;
    }
  }
}

long sampleTemp () {
  uint8_t i;
  float average;
  for (i = 0; i < NUMSAMPLES; i++) {
    samples[i] = analogRead(THERMISTORPIN);
    delay(10);
  }

  average = 0;
  for (i = 0; i < NUMSAMPLES; i++) {
    average += samples[i];
  }
  average /= NUMSAMPLES;

  average = 1023 / average - 1;
  average = SERIESRESISTOR / average;

  float steinhart;
  steinhart = average / THERMISTORNOMINAL;
  steinhart = log(steinhart);
  steinhart /= BCOEFFICIENT;
  steinhart += 1.0 / (TEMPERATURENOMINAL + 273.15);
  steinhart = 1.0 / steinhart;
  steinhart -= 273.15;

  return long(steinhart);
}
