/*
Arduino Code for the OCRA Tabletop MRI System patient-bar controller.
Ivo Opitz, 2024 / Marcus Prier 2026
*/

#include <Arduino.h>
#include <math.h>
#include "BasicStepperDriver.h"

// Definitions for the Motor
#define MOTOR_STEPS 200  // 200 steps/revolution = 1.8 degrees/step
#define RPM 300
#define RPM_HOME 100
#define RPM_HOME_FINE 20
#define MICROSTEPS 8  // 8/x actual steps per send step
#define ACCEL 1000
#define DECEL 1000
// Pin Definitions:
#define DIR 2   // direction-pin
#define STEP 3  // step-pin
#define SLP 4   // Sleep-Pin (Low to Sleep)
#define RST 5   // Reset-Pin (Low to Reset Driver)
#define M2 6    // Microstep Bit 2
#define M1 7    // Microstep Bit 1
#define M0 8    // Microstep Bit 0
#define EN 9    // Enable-Pin (Low to Enable the Driver Output)
BasicStepperDriver stepper(MOTOR_STEPS, DIR, STEP);

// Keeping track of the position
#define SCREW_PITCH 5
float screw_max = 197;
float screw_min = 0;
float position = 0;  // current position
float desired_position = 0;
float mm_per_step = (float)SCREW_PITCH / ((float)MOTOR_STEPS * (float)MICROSTEPS);

// Fault Case
#define FAULT 10  // Fault Pin
volatile bool isFaultCase = false;

// Homing
#define STOP A7  // pin at which the end-stop-switch is located
// an external pulldown resistor is required for non three-pole-switches

// Messageing
bool newData = false;
const byte numChars = 32;
char receivedChars[numChars];
bool debugging = false;  // only use for debugging over serial console NOT with the Relax2.0 software
bool disableStepper = true;

void setup() {
  Serial.begin(115200);
  Serial.setTimeout(10);

  stepper.begin(RPM, MICROSTEPS);
  stepper.setSpeedProfile(stepper.LINEAR_SPEED, ACCEL, DECEL);

  pinMode(STOP, INPUT);
  pinMode(LED_BUILTIN, OUTPUT);

  pinMode(SLP, OUTPUT);
  digitalWrite(SLP, HIGH);

  pinMode(RST, OUTPUT);
  digitalWrite(RST, HIGH);

  pinMode(EN, OUTPUT);
  digitalWrite(EN, HIGH);

  pinMode(M0, OUTPUT);
  pinMode(M1, OUTPUT);
  pinMode(M2, OUTPUT);

  digitalWrite(M0, HIGH);
  digitalWrite(M1, HIGH);
  digitalWrite(M2, LOW);

  attachInterrupt(digitalPinToInterrupt(FAULT), faultCase, FALLING);

  if (debugging) {
    Serial.print("D0: Current Step Distance: ");
    Serial.print(mm_per_step, 10);
    Serial.println(" mm");
  }

  while (!Serial.available())
    ;
}

void loop() {
  recvWithEndMarker();
  if (newData) {
    String s = receivedChars;
    newData = false;
    if (s.substring(0, 3).equals("G28")) {
      home();
    } else if (s.substring(0, 3).equals("M18")) {
      disableStepper = true;
      pinMode(EN, OUTPUT);
      digitalWrite(EN, HIGH);
    } else if (s.substring(0, 3).equals("M17")) {
      disableStepper = false;
      pinMode(EN, OUTPUT);
      digitalWrite(EN, LOW);
    } else if (s.substring(0, 3).equals("M84")) {
      pinMode(EN, OUTPUT);
      digitalWrite(EN, HIGH);
    } else if (s.substring(0, 2).equals("G0")) {
      float x = s.substring(3).toFloat();

      // check for valid length
      if (x >= screw_min && x <= screw_max) {
        long steps_required = (round(x * 100000) / round(mm_per_step * 100000));  // this increases precision
        desired_position = steps_required * mm_per_step;
        if (debugging) {
          Serial.print("D0: Moving to ");
          Serial.print(desired_position, 3);
          Serial.println(" mm");
        }
      } else {
        if (debugging) {
          Serial.println("D1: Given distance is negative or longer than the screw.");
        }
      }
    } else if (s.substring(0, 4).equals("M114")) {
      Serial.println(position);
    } else if (s.substring(0, 4).equals("M115")) {
      Serial.print("MRI-Patient-Motor-Control");
      Serial.print(" MAX_LENGTH: ");
      Serial.print(screw_max);
      Serial.print(" MIN_LENGTH: ");
      Serial.print(screw_min);
      Serial.print(" VERSION: 3.2");
      Serial.println("");
    } else if (s.substring(0, 4).equals("M118")) {
      Serial.println(s.substring(5));
    } else if (s.substring(0, 4).equals("M203")) {
      int index = s.substring(5).indexOf(" ");
      if (index == -1) {
        Serial.print(screw_min);
        Serial.print(" ");
        Serial.println(screw_max);
      } else {
        screw_min = s.substring(5, index + 5).toFloat();
        screw_max = s.substring(index + 6).toFloat();

        if (debugging) {
          Serial.print("D0: set min and max axis length to: ");
          Serial.print(screw_min);
          Serial.print(" and ");
          Serial.println(screw_max);
        }
      }
    } else {
      digitalWrite(LED_BUILTIN, HIGH);  // turn the LED on (HIGH is the voltage level)
      Serial.print("No valid command found. Please check the following:");
      Serial.print("\tG28 - Home the axis.");
      Serial.print("\tG0 x - Move to position x.");
      Serial.print("\tM17 - Enables the stepper motor between movement commands to keep it locked in place.");
      Serial.print("\tM18 - Disables the stepper motor. It will activate for movement commands but otherwise will stay disabled.");
      Serial.print("\tM84 - Disables the stepper motor until the next movement command. Afterwards it will stay active again.");
      Serial.print("\tM114 - Returns current position.");
      Serial.print("\tM115 - Returns Device Name.");
      Serial.print("\tM118 x - Returns String x. A1 and E1 Parameters will not be handeled");
      Serial.print("\tM203 min max - Set the current Minimum and Maximum Axis Length");
      Serial.print("\tM203 - Returns the current Minimum and Maximum Axis Length");
      Serial.println("");
      delay(1000);                     // wait for a second so the light can be seen by the user
      digitalWrite(LED_BUILTIN, LOW);  // turn the LED off by making the voltage LOW
    }
  }

  if ((position != desired_position) && !isFaultCase) {
    pinMode(EN, OUTPUT);
    digitalWrite(EN, LOW);

    float distance = desired_position - position;
    long steps = round(distance * 100000) / round(mm_per_step * 100000);  // doing so increases precision
    stepper.move(steps);
    position = desired_position;

    if (disableStepper) {
      pinMode(EN, OUTPUT);
      digitalWrite(EN, HIGH);
    }
  }
}

void recvWithEndMarker() {
  static byte index = 0;
  char endChar = '\n';
  char receivedChar;

  while (Serial.available() > 0 && newData == false) {
    receivedChar = Serial.read();

    if (receivedChar != endChar) {
      receivedChars[index] = receivedChar;
      index++;
      if (index >= numChars) {
        index = numChars - 1;
      }
    } else {
      receivedChars[index] = '\0';  // terminate the string
      index = 0;
      newData = true;
    }
  }
}

void home() {
  pinMode(EN, OUTPUT);
  digitalWrite(EN, LOW);

  if (debugging) {
    Serial.println("D0: Starting Homing");
  }

  stepper.setRPM(RPM_HOME);
  stepper.startMove(-1000000);
  while (digitalRead(STOP) == LOW) {
    stepper.nextAction();
  }

  stepper.stop();
  delay(200);

  stepper.setRPM(RPM_HOME_FINE);
  stepper.startMove(5000);

  while (digitalRead(STOP) == HIGH) {
    stepper.nextAction();
  }

  stepper.stop();
  delay(200);

  stepper.setRPM(RPM);

  /*
  while(digitalRead(STOP) == LOW){
    stepper.move(-1);
    delay(2);
  }
  delay(500);
  while(digitalRead(STOP) == HIGH){
    stepper.move(1);
    delay(20);
  }
  
  if(debugging){
    Serial.println("D0: Homing completed");
  }
  */

  position = screw_min;
  desired_position = screw_min;

  if (disableStepper) {
    pinMode(EN, OUTPUT);
    digitalWrite(EN, HIGH);
  }
}

void faultCase() {
  Serial.println("E0: Overcurrent or Overtemp on Motor Driver reached. Check the hardware and restart the software afterwards.");
  isFaultCase = true;

  pinMode(SLP, OUTPUT);
  digitalWrite(SLP, LOW);
}