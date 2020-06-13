
/*
  Based on code from David Naves (http://daveworks.net, http://davenaves.com)
  2020-06-13 :
    - Added 433mhz signaling of door status
  2020-06-01 :
    - Added integration of max timer for opening/closing
    - Opening and closing is only once (controlled by boolean) and only interrupted if sucessfull or max timer
    - If fails when closing door, first open (reverse turning) before error 
*/

// LIBRARIES

#include <SimpleTimer.h>              //TIMER library from github
#include <RCSwitch.h>                 // RC switch library

// CONFIG

const boolean SerialDisplay = false;                       // Serial debug ?

// PINS ASSIGNED

//General
const int photocellPin = 0;                 // photocell connected to analog 0
const int enableCoopDoorMotorB = 7;          // enable motor b - pin 7
const int directionCloseCoopDoorMotorB = 8;  // direction close motor b - pin 8
const int directionOpenCoopDoorMotorB = 9;   // direction open motor b - pin 9
const int bottomSwitchPin = 26;              // bottom switch is connected to pin 26
const int topSwitchPin = 27;                 // top switch is connected to pin 27
const int transmitterPin = 10;              // transmitter connected to pin 10

// DEFINE OBJECTS
SimpleTimer coopPhotoCellTimer;       //TIMER : init
RCSwitch doorSwitch = RCSwitch();       //RCswitch : init

// INIT VARIABLES

// photocell
int photocellReading;                  // PHOTOCELL : analog reading of the photocel
int photocellReadingLevel;             // PHOTOCELL : photocel reading levels (dark, twilight, light)
// top switch
int topSwitchPinVal;                   // top switch : var for reading the pin status
int topSwitchPinVal2;                  // top switch : var for reading the pin delay/debounce status
int topSwitchState;                    // top switch : var for to hold the switch state
// bottom switch
int bottomSwitchPinVal;                // bottom switch : var for reading the pin status
int bottomSwitchPinVal2;               // bottom switch : var for reading the pin delay/debounce status
int bottomSwitchState;                 // bottom switch : var for to hold the switch state
// door variable
bool dooropen = false;                         // door : opened or closed


// DEFINE VARIABLES

// debounce delay
long lastDebounceTime = 0;            // Reed switch debounce
long debounceDelay = 100;             // Reed switch debounce interval
// Max execution time
long MaxMotorRunTime = 20000;         // If motor runs more than 20s, stop program
long MotorRunTime = 0;

// ************************************** the setup **************************************

void setup(void) {

  // WELCOME SERIAL
  Serial.begin(9600); // initialize serial port hardware
  Serial.println("Starting software");

  // TRANSMITTER 433mhz
  doorSwitch.enableTransmit(transmitterPin);
  doorSwitch.switchOn("1111111", "00010");   // signal switchOn
  delay(1000);

  //DEFINE SWITCH
  // coop door motor
  pinMode (enableCoopDoorMotorB, OUTPUT);           // enable motor pin = output
  pinMode (directionCloseCoopDoorMotorB, OUTPUT);   // motor close direction pin = output
  pinMode (directionOpenCoopDoorMotorB, OUTPUT);    // motor open direction pin = output
  // bottom switch
  pinMode(bottomSwitchPin, INPUT);                  // set bottom switch pin as input
  digitalWrite(bottomSwitchPin, HIGH);              // activate bottom switch resistor
  // top switch
  pinMode(topSwitchPin, INPUT);                     // set top switch pin as input
  digitalWrite(topSwitchPin, HIGH);                 // activate top switch resistor
  Serial.println("All switches defined.");

  // Reset door position by lowering it mid-way, open it, than measure lumi
  Serial.println("Reset Door Position");
  digitalWrite (directionCloseCoopDoorMotorB, HIGH);    // turn on motor close direction
  digitalWrite (directionOpenCoopDoorMotorB, LOW);      // turn off motor open direction
  analogWrite (enableCoopDoorMotorB, 255);              // enable motor, max speed
  delay(3000);                                           // door moves for 3 seconds  
  digitalWrite (directionCloseCoopDoorMotorB, LOW);     // turn off motor close direction
  digitalWrite (directionOpenCoopDoorMotorB, LOW);      // turn off motor open direction
  analogWrite (enableCoopDoorMotorB, 0);              // enable motor, 0 speed
  openCoopDoorMotorB();                                         // place door in normal condition
  delay(3000);
  doCoopDoor();
  
  // TIMER
  coopPhotoCellTimer.setInterval(600000, readPhotoCell);   // read the photocell every 10 minutes, was 600000
  Serial.println("Photocell reading duration defined : 10 minutes.");

}


// ************************************** functions **************************************


// operate the coop door

// photocel to read levels of exterior light

void readPhotoCell() { // function to be called repeatedly - per coopPhotoCellTimer set in setup

  photocellReading = analogRead(photocellPin);
  if (SerialDisplay) {
    Serial.print(" Photocel Analog Reading = ");
    Serial.println(photocellReading);
  }

  //  set photocel threshholds
  if (photocellReading >= 0 && photocellReading <= 150) {
    photocellReadingLevel = '1';

    if (SerialDisplay) {
      Serial.print(" Photocel Reading Level:");
      Serial.println(" - Dark");
    }
  }
  else if (photocellReading  >= 151 && photocellReading <= 300) {
    photocellReadingLevel = '2';
    if (SerialDisplay) {
      Serial.print(" Photocel Reading Level:");
      Serial.println(" - Twilight");
    }
  }
  else if (photocellReading  >= 301 ) {
    photocellReadingLevel = '3';
    if (SerialDisplay) {
      Serial.print(" Photocel Reading Level:");
      Serial.println(" - Light");
    }
  }
  Serial.println();
}

//debounce bottom reed switch

void debounceBottomReedSwitch() {
  //debounce bottom reed switch
  bottomSwitchPinVal = digitalRead(bottomSwitchPin);       // read input value and store it in val
  // delay(10);

  if ((millis() - lastDebounceTime) > debounceDelay) {    // delay 10ms for consistent readings

    bottomSwitchPinVal2 = digitalRead(bottomSwitchPin);    // read input value again to check or bounce

    if (bottomSwitchPinVal == bottomSwitchPinVal2) {       // make sure we have 2 consistant readings
      if (bottomSwitchPinVal != bottomSwitchState) {       // the switch state has changed!
        bottomSwitchState = bottomSwitchPinVal;
      }
      if (SerialDisplay) {
        Serial.print (" Bottom Switch Value: ");           // display "Bottom Switch Value:"
        Serial.println(digitalRead(bottomSwitchPin));      // display current value of bottom switch;
      }
    }
  }
}

// debounce top reed switch
void debounceTopReedSwitch() {

  topSwitchPinVal = digitalRead(topSwitchPin);             // read input value and store it in val
  //  delay(10);

  if ((millis() - lastDebounceTime) > debounceDelay) {     // delay 10ms for consistent readings

    topSwitchPinVal2 = digitalRead(topSwitchPin);          // read input value again to check or bounce

    if (topSwitchPinVal == topSwitchPinVal2) {             // make sure we have 2 consistant readings
      if (topSwitchPinVal != topSwitchState) {             // the button state has changed!
        topSwitchState = topSwitchPinVal;
      }
      if (SerialDisplay) {
        Serial.print (" Top Switch Value: ");              // display "Bottom Switch Value:"
        Serial.println(digitalRead(topSwitchPin));         // display current value of bottom switch;
      }
    }
  }
}

// stop the coop door motor
void stopCoopDoorMotorB() {
  digitalWrite (directionCloseCoopDoorMotorB, LOW);      // turn off motor close direction
  digitalWrite (directionOpenCoopDoorMotorB, LOW);       // turn on motor open direction
  analogWrite (enableCoopDoorMotorB, 0);                 // enable motor, 0 speed
  delay(2000);
}

// close the coop door motor (motor dir close = clockwise)
void closeCoopDoorMotorB() {
  MotorRunTime = millis();
  while (digitalRead(bottomSwitchPin) == 1) {
    digitalWrite (directionCloseCoopDoorMotorB, HIGH);     // turn on motor close direction
    digitalWrite (directionOpenCoopDoorMotorB, LOW);       // turn off motor open direction
    analogWrite (enableCoopDoorMotorB, 255);               // enable motor, full speed
    if ((millis() - MotorRunTime) > MaxMotorRunTime) {
    ;
    error();
    }
  };                       // if bottom reed switch circuit is closed
  stopCoopDoorMotorB();
  dooropen = false;
  doorSwitch.switchOff("11111", "00010");                // signal door status
  delay(1000);
  if (SerialDisplay) {
    Serial.println(" Coop Door Closed");
    Serial.println();
  }
}

// open the coop door (motor dir open = counter-clockwise)
void openCoopDoorMotorB() {
  MotorRunTime = millis();
  while (digitalRead(topSwitchPin) == 1) {
    digitalWrite(directionCloseCoopDoorMotorB, LOW);       // turn off motor close direction
    digitalWrite(directionOpenCoopDoorMotorB, HIGH);       // turn on motor open direction
    analogWrite(enableCoopDoorMotorB, 255);                // enable motor, full speed
    if ((millis() - MotorRunTime) > MaxMotorRunTime) {
    error();
    }
  };
  stopCoopDoorMotorB();
  dooropen = true;
  doorSwitch.switchOn("11111", "00010");                // signal door status
  delay(1000);
  if (SerialDisplay) {
    Serial.println(" Coop Door Opened");
    Serial.println();
  }
}

// do the coop door
void doCoopDoor() {
  if (photocellReadingLevel  == '1') {              // if it's dark
    if (photocellReadingLevel != '2') {             // if it's not twilight
      if (photocellReadingLevel != '3') {           // if it's not light
        if (dooropen == true) {
          debounceTopReedSwitch();                    // read and debounce the switches
          debounceBottomReedSwitch();
          closeCoopDoorMotorB();                      // close the door
        }
      }
    }
  }
  if (photocellReadingLevel  == '3') {              // if it's light
    if (photocellReadingLevel != '2') {             // if it's not twilight
      if (photocellReadingLevel != '1') {           // if it's not dark
        if (dooropen == false) {
          debounceTopReedSwitch();                    // read and debounce the switches
          debounceBottomReedSwitch();
          openCoopDoorMotorB();                       // Open the door
        }
      }
    }
  }
}

void error(){
stopCoopDoorMotorB();
Serial.println("Error, stopping");
doorSwitch.setRepeatTransmit(15);       // increases probability of receiving
doorSwitch.switchOff("1111111", "00010");   // signal error
while(true) {};                         // stops program
}

// show values in serial
void doCoopSerial() {
  Serial.print(" Lumi: ");
  Serial.println(analogRead(photocellPin));
  Serial.print(" Bottom: ");
  Serial.println(digitalRead(bottomSwitchPin));
  Serial.print(" Top: ");
  Serial.println(digitalRead(topSwitchPin));
  Serial.println();
}

// ************************************** the loop **************************************

void loop() {
  coopPhotoCellTimer.run();      // timer for readPhotoCell
  doCoopDoor();
  //doCoopSerial();
}
