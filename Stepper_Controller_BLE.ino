/***********************************************************************
* 	FILENAME :        Stepper_Controller_BLE.ino
*
* 	DESCRIPTION :
*       Arduino code for running a stepper motor at specific rpms via bluetooth. The motor's 
*		rpm is ramped to the desired speed using a user set acceleration.
*
* 	PUBLIC FUNCTIONS :
*       void blePeripheralConnectHandler( BLECentral& )
*		void blePeripheralDisconnectHandler( BLECentral& )
*		void setup( )
*		void stepperRPMWritten( BLECentral& , BLECharacteristic& )
*		
*
* 	NOTES :
*       - Uses the Curie Bluetooth Low-Energy library for bluetooth connection.
*		- We used nRF Connect from Nordic Semiconductor on Android appliction in order to send and recieve commands.
*
*
* 	AUTHOR :    Brandon Artner        START DATE :    June 2017
*
* 	CHANGES :
*
*		DATE    	WHO     DETAIL
*  		24Sep2017 	BA      Added all of these super handy comments
*
**/

#include <Stepper.h>
#include <CurieBLE.h>

#define DEBUG true 	// Display BTSerial messages on Serial Monitor
#define MAXIMUM_RPM 1200; 	// Maximum rpm of the motor, our motor shut off if set to a higher speed
#define STEPS_PER_REVOLUTION 200;  	// change this to fit the number of steps per revolution for your motor

BLEPeripheral blePeripheral; // create BLE peripheral instance

BLEService stepperService("19B10000-E8F2-537E-4F6C-D104768A1214"); // create BLE service

// create characteristic for stepper rpm and allow remote device to read and write
BLEIntCharacteristic stepperRPM("19B10001-E8F2-537E-4F6C-D104768A1215", BLERead | BLEWrite);

// create characteristic for the ramping speed and allow remote device to read and write
BLEIntCharacteristic rampingSpeed("19B10002-E8F2-537E-4F6C-D104768A1215", BLERead | BLEWrite);

// initialize the stepper library on pins 8 through 11:
Stepper myStepper(STEPS_PER_REVOLUTION, 8, 9, 10, 11);	

int rpm = 0;


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
  blePeripheral.addAttribute(rampingSpeed);

  // assign event handlers for connected, disconnected to peripheral
  blePeripheral.setEventHandler(BLEConnected, blePeripheralConnectHandler);
  blePeripheral.setEventHandler(BLEDisconnected, blePeripheralDisconnectHandler);

  // assign event handlers for characteristic
  stepperRPM.setEventHandler(BLEWritten, stepperRPMWritten);
  
  // assign event handlers for characteristic
  stepperRPM.setEventHandler(BLEWritten, stepperRPMWritten);
  
  // set an initial value for the characteristic
  stepperRPM.setValue(0);
  rampingSpeed.setValue(100);
  
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
    rpm+=rampingSpeed.value();
    myStepper.setSpeed(rpm);
    Serial.print("clockwise,rpm: ");
    Serial.println(rpm);
  }
  if(rpm>stepperRPM.value()){
	rpm = stepperRPM.value();
  }
  // Don't run if rpm is 0, will cause program to hang
  if(stepperRPM.value() != 0)
    myStepper.step(STEPS_PER_REVOLUTION);
}


//********************
// Auxiliary functions
//********************
void blePeripheralConnectHandler(BLECentral& central) {
  // central connected event handler
  if(DEBUG) {
	  Serial.print("Connected event, central: ");
	  Serial.println(central.address());
  }
}

void blePeripheralDisconnectHandler(BLECentral& central) {
  // central disconnected event handler
  if(DEBUG) {
	  Serial.print("Disconnected event, central: ");
	  Serial.println(central.address());
  }
}

void stepperRPMWritten(BLECentral& central, BLECharacteristic& characteristic) {
  // central wrote new value to characteristic, update LED
  Serial.print("Characteristic event, written: ");
  
  if(stepperRPM.value() > MAXIMUM_RPM){
	char* buffTemplate
    char* buff = (char*) malloc(sizeof(char)*100);
    sprintf(buff,"RPM entered is greater than maximum allowed. (Max RPM = %d)",MAXIMUM_RPM);
    Serial.println(buff);
    stepperRPM.setValue(0);
  }
  else
    rpm=0; 
}