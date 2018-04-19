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
*		    - We used nRF Connect from Nordic Semiconductor on Android appliction in order to send and recieve commands.
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
#define MAXIMUM_RPM 1200 	// Maximum rpm of the motor, our motor shut off if set to a higher speed
#define STEPS_PER_REVOLUTION 200  	// change this to fit the number of steps per revolution for your motor

BLEPeripheral blePeripheral; // create BLE peripheral instance

BLEService stepperService("19B10000-E8F2-537E-4F6C-D104768A1214"); // create BLE service

// create characteristic for job data and allow remote device to read and write

	// Data should be in the following bit pattern
	//		xxxx xxxx xxxx | yyyy yyyy yyyy | zzzz zzzz
	// where, 
	// 	x is the final desired rpm
	// 	y is the desired run duration
	// 	z is the desired ramping period duration
	
// The UUID is pretty arbitrary, the first part is the device id though
BLEIntCharacteristic jobData("19B10001-E8F2-537E-4F6C-D104768A1215", BLERead | BLEWrite);


// initialize the stepper library on pins 8 through 11:
Stepper myStepper(STEPS_PER_REVOLUTION, 8, 9, 10, 11);	



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
	blePeripheral.addAttribute(jobData);

	// assign event handlers for connected, disconnected to peripheral
	blePeripheral.setEventHandler(BLEConnected, blePeripheralConnectHandler);
	blePeripheral.setEventHandler(BLEDisconnected, blePeripheralDisconnectHandler);

	// assign event handlers for characteristic
	jobData.setEventHandler(BLEWritten, jobDataWritten);

	// set an initial value for the characteristic
	jobData.setValue(0);

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
	
}


//********************
// Auxiliary functions
//********************

/*
 *	Central connected event handler
 *	Adds notification that there was a connection to the serial port. Occurs if DEBUG is true. 
 *	Connect Arduino to usb port, open Arduino IDE, and open serial monitoring to receive the data.
 */
void blePeripheralConnectHandler(BLECentral& central) {
	if(DEBUG) {
		Serial.print("Connected event, central: ");
		Serial.println(central.address());
	}
}

/*
 *	Central disconnected event handler
 *	Adds notification that there was a disconnection to the serial port. Occurs if DEBUG is true. 
 *	Connect Arduino to usb port, open Arduino IDE, and open serial monitoring to receive the data.
 */
void blePeripheralDisconnectHandler(BLECentral& central) {
	if(DEBUG) {
		Serial.print("Disconnected event, central: ");
		Serial.println(central.address());
	}
}

/*
 *	Central wrote new value to characteristic
 *	
 */
void jobDataWritten(BLECentral& central, BLECharacteristic& characteristic) {
	Serial.print("Characteristic event, written: ");
	// Declare variable: 
	//		fRPM: Final desired rpm
	//		t: Time elapsed
	//		rampDur: Duration of the ramping period
	//		runDur: Duration of the run 
	//		curRPM: Current rpm for the ramping period
	//		w: Angular acceleration	
  //    d_t: Time step
  //    runRotations: Number of rotations to 
	int fRPM, rampDur, runDur;
	float curRPM, w, t, d_t, runRotations;

  if(DEBUG)
    Serial.println(jobData.value());
	
	curRPM = 0;
  d_t = 0.1;
	
	// Incoming data should be in the following bit pattern
	//		xxxx xxxx xxxx | yyyy yyyy yyyy | zzzz zzzz
	// where, 
	// 	x is the final desired rpm
	// 	y is the desired run duration
	// 	z is the desired ramping period duration
	fRPM = (jobData.value() & 0xfff00000) >> 20;
	runDur = (jobData.value() & 0x000fff00) >> 8;
	rampDur = (jobData.value() & 0x000000ff);

  if(DEBUG) {
    Serial.print("RPM: ");
    Serial.println(fRPM);
    Serial.print("Run Duration: ");
    Serial.println(runDur);
    Serial.print("Ramp Duration: ");
    Serial.println(rampDur);
  }
  
	// Check that desired rom is with in bounds
	if( fRPM > MAXIMUM_RPM ){
		//char* buffTemplate;
		char* buff = (char*) malloc(sizeof(char)*100);
		sprintf(buff,"RPM entered is greater than maximum allowed. (Max RPM = %d)",MAXIMUM_RPM);
		Serial.println(buff);
		jobData.setValue(0);
	}
	else {
		
		w = fRPM/rampDur; // Calculate angular acceleration
		t = 1;
    
		// Ramping Period
		while(t <= rampDur){
			// Update RPM using angular acceleration 
			curRPM = min(w*t, fRPM);
			myStepper.setSpeed(curRPM);
     
      if(DEBUG)
        Serial.println(curRPM);
      
      runRotations = (curRPM * d_t)/60;
			myStepper.step(round(STEPS_PER_REVOLUTION*runRotations));
			t+=d_t;
		}
		
    if(DEBUG)
        Serial.println(curRPM);
        
    runRotations = (fRPM*runDur)/60;

    // Run Period
		myStepper.step(round(STEPS_PER_REVOLUTION*runRotations));

	}
	myStepper.setSpeed(0);
}
