// Written By: Brandon Artner

//include libraries
#include <Stepper.h>
#include <CurieBLE.h>

BLEPeripheral blePeripheral; // create peripheral instance

BLEService stepperService("19B10000-E8F2-537E-4F6C-D104768A1214"); // create service

// create switch characteristic and allow remote device to read and write
BLEIntCharacteristic stepperRPM("19B10001-E8F2-537E-4F6C-D104768A1215", BLERead | BLEWrite);

//define variables
#define DEBUG true //display BTSerial messages on Serial Monitor

const int stepsPerRevolution = 200;  // change this to fit the number of steps per revolution for your motor
// initialize the stepper library on pins 8 through 11: 
Stepper myStepper(stepsPerRevolution, 8, 9, 10, 11);

const int maxrpm = 1200;
int rpm = 0;

// Auxiliary functions
//********************
void blePeripheralConnectHandler(BLECentral& central) {
  // central connected event handler
  Serial.print("Connected event, central: ");
  Serial.println(central.address());
}

void blePeripheralDisconnectHandler(BLECentral& central) {
  // central disconnected event handler
  Serial.print("Disconnected event, central: ");
  Serial.println(central.address());
}

//**************
// Arduino Setup
//**************
void setup()
{
  
  // initialize the speed at 0 rpm
  myStepper.setSpeed(0);
  
  //start serial communication
  Serial.begin(9600);
  
  // set the local name peripheral advertises
  blePeripheral.setLocalName("STEPCB");
  // set the UUID for the service this peripheral advertises
  blePeripheral.setAdvertisedServiceUuid(stepperService.uuid());

  // add service and characteristic
  blePeripheral.addAttribute(stepperService);
  blePeripheral.addAttribute(stepperRPM);

  // assign event handlers for connected, disconnected to peripheral
  blePeripheral.setEventHandler(BLEConnected, blePeripheralConnectHandler);
  blePeripheral.setEventHandler(BLEDisconnected, blePeripheralDisconnectHandler);

  // assign event handlers for characteristic
  stepperRPM.setEventHandler(BLEWritten, stepperRPMWritten);
  
  // assign event handlers for characteristic
  stepperRPM.setEventHandler(BLEWritten, stepperRPMWritten);
  
  // set an initial value for the characteristic
  stepperRPM.setValue(0);
  
  // advertise the service
  blePeripheral.begin();
  Serial.println(("Bluetooth device active, waiting for connections..."));
}

//**********
// Main Loop
//**********
void loop() {
  // poll peripheral
  blePeripheral.poll();

  // If the current rpm is less than desired rpm increase by 100
  if(rpm<stepperRPM.value()){
    rpm+=100;
    myStepper.setSpeed(rpm);
    Serial.print("clockwise,rpm: ");
    Serial.println(rpm);
  }
  // Don't run if rpm is 0, will cause program to hang
  if(stepperRPM.value() != 0)
    myStepper.step(stepsPerRevolution);
}


//********************

void stepperRPMWritten(BLECentral& central, BLECharacteristic& characteristic) {
  // central wrote new value to characteristic, update LED
  Serial.print("Characteristic event, written: ");
  
  if(stepperRPM.value() > maxrpm){
    char* buf = (char*) malloc(sizeof(char)*100);
    sprintf(buf,"RPM entered is greater than maximum allowed. (Max RPM = %d)",maxrpm);
    Serial.println(buf);
    stepperRPM.setValue(0);
  }
  else
    rpm=0; 
}

