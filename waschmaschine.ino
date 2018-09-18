#include "Arduino.h"
//The setup function is called once at startup of the sketch


#include <ShiftRegisterFactory.h>
#include <FourDigitSevenSegmentDisplay.h>
#include <MotorDriverTB6612FNG.h>
#include <WashingProgram.h>
#include <PushButton.h>

// defines

#define SIPO_ST_CP_LATCH 4
#define SIPO_SH_CP_CLOCK 2
#define SIPO_SER_DATA 3
#define SIPO_COUNT 2

#define DOOR_SHUT_PIN 5
#define START_BUTTON_PIN 10

#define MOTOR_PWM_PIN 9
#define MOTOR_STDBY_PIN 6
#define MOTOR_IN_1_PIN 7
#define MOTOR_IN_2_PIN 8


// define program chooser 8 position switch to program
#define PROGRAM_WOOL_PIN A2
#define PROGRAM_30_PIN A1
#define PROGRAM_40_PIN A0
#define PROGRAM_60_PIN A4
#define PROGRAM_90_PIN A5
#define PROGRAM_FLUSHING_PIN A6
#define PROGRAM_SPINNING_PIN A7
#define PROGRAM_OFF_PIN A3

// define Program IDs
int PROGRAM_WOOL_ID = 0;
int PROGRAM_30_ID = 1;
int PROGRAM_40_ID = 2;
int PROGRAM_60_ID = 3;
int PROGRAM_90_ID = 4;
int PROGRAM_FLUSHING_ID = 5;
int PROGRAM_SPINNING_ID = 6;
int PROGRAM_OFF_ID = -1;

// define the range for real washing programs. They need to be in an increasing range. no gaps, no flushing, spinning, punping in between.

int programWashingIdMin=0;
int programWashingIdMax=4;






#define PROGRAM_WASHING_DEFAULT_TIME 60
#define PROGRAM_FLUSHING_DEFAULT_TIME 30
#define PROGRAM_SPINNING_DEFAULT_TIME 45





// define log levels (debug=0, info=1, warn=2, error=3)

#define LOGLEVEL 0


// variables
int logLevel = LOGLEVEL;
ShiftRegisterFactory srf = ShiftRegisterFactory(SIPO_ST_CP_LATCH, SIPO_SH_CP_CLOCK, SIPO_SER_DATA, SIPO_COUNT);
FourDigitSevenSegmentDisplay myDisplay = FourDigitSevenSegmentDisplay(srf, 0, 0, 0, 1, 0, 2, 0, 3, 0, 4, 0, 5, 0, 6, 0, 7, 1, 0, 1, 2, 1, 3, 1, 4);

MotorDriverTB6612FNG motor = MotorDriverTB6612FNG(MOTOR_IN_1_PIN, MOTOR_IN_2_PIN, MOTOR_PWM_PIN, MOTOR_STDBY_PIN);

// global variables
#define NUMBER_OF_MAX_CHAINED_PROGRAMS 5
// int programArray[NUMBER_OF_MAX_CHAINED_PROGRAMS]; // holds all chained programm ids in order to run, eg. prewash, washing, flushing, spinning, end. if nothing is set or if program is over, set to -1.
// int currentProgramArrayId = 0;



//WashingProgram programWashing = WashingProgram(WASHING, PROGRAM_WASHING_DEFAULT_TIME, motor);
//WashingProgram programFlushing = WashingProgram(FLUSHING, PROGRAM_FLUSHING_DEFAULT_TIME, motor);
//WashingProgram programSpinning = WashingProgram(SPINNING, PROGRAM_SPINNING_DEFAULT_TIME, motor);
WashingProgram currentProgram = WashingProgram(WASHING, 0, motor);



PushButton doorOpenSwitch = PushButton(0, DOOR_SHUT_PIN);
PushButton startButton = PushButton(0, START_BUTTON_PIN);


bool programWashingNeedsToRun = false;
long programWashingTimeLeft = 0;
long programFlushingTimeLeft = 0;
bool programFlushingNeedsToRun = false;
long programSpinningTimeLeft = 0;
bool programSpinningNeedsToRun = false;

int lastProgramId=-1;
int currentProgramId=-1;
int temperature=0;

bool isProgramRunning=false;




void debug(char text[])
{
	if (logLevel == 0)
	{
		Serial.print(text);
	}
}

void debug (int integer)
{
	if (logLevel == 0)
	{
		Serial.print(integer);
	}
}

void debug (long mylong)
{
	if (logLevel == 0)
	{
		Serial.print(mylong);
	}
}

void info(char text[])
{
	if (logLevel <= 1)
	{
		Serial.print(text);
	}
}

void info (int integer)
{
	if (logLevel <= 1)
	{
		Serial.print(integer);
	}
}

/**
 * calculate remaining times
 */
void calculateRemainingTimes()
{
	if (currentProgram.getMode() == WASHING)
	{
		programWashingTimeLeft = currentProgram.getRemainingTimeInSeconds();
	}
	else if (currentProgram.getMode() == FLUSHING)
	{
		programFlushingTimeLeft = currentProgram.getRemainingTimeInSeconds();
	}
	else if (currentProgram.getMode() == SPINNING)
	{
		programSpinningTimeLeft = currentProgram.getRemainingTimeInSeconds();
	}
}

/**
 * gets the position of the ProgramChooserButton and programs the washing programs.
 */
void updateCurrentProgram()
{


	debug("\nanalog Ins: ");
	debug (digitalRead(PROGRAM_OFF_PIN));
	debug (digitalRead(PROGRAM_WOOL_PIN));
	debug (digitalRead(PROGRAM_30_PIN));
	debug (digitalRead(PROGRAM_40_PIN));
	debug (digitalRead(PROGRAM_60_PIN));
	debug (digitalRead(PROGRAM_90_PIN));
	debug (digitalRead(PROGRAM_SPINNING_PIN));
	debug (digitalRead(PROGRAM_FLUSHING_PIN));


	// we are using input pullup -> we need to check for false.

	if (!digitalRead(PROGRAM_OFF_PIN))
	{
		currentProgramId = PROGRAM_OFF_ID;
	}
	else if(!digitalRead(PROGRAM_WOOL_PIN))
	{
		currentProgramId = PROGRAM_WOOL_ID;
		temperature=0;
	}
	else if (!digitalRead(PROGRAM_30_PIN))
	{
		currentProgramId = PROGRAM_30_ID;
		temperature=30;
	}
	else if (!digitalRead(PROGRAM_40_PIN))
	{
		currentProgramId = PROGRAM_40_ID;
		temperature=40;
	}
	else if (!digitalRead(PROGRAM_60_PIN))
	{
		currentProgramId = PROGRAM_60_ID;
		temperature=60;
	}
	else if (!digitalRead(PROGRAM_90_PIN))
	{
		currentProgramId = PROGRAM_90_ID;
		temperature=90;
	}
	else if (!digitalRead(PROGRAM_FLUSHING_PIN))
	{
		currentProgramId = PROGRAM_FLUSHING_ID;
	}
	else if (!digitalRead(PROGRAM_SPINNING_PIN))
	{
		currentProgramId = PROGRAM_SPINNING_ID;
	}

}

/**
 * returns if the door is open
 */
bool isDoorOpen()
{
	// doorOpenSwitch.processButtonState();
	// debug("\ndoorOpenSwitch=" );
	// debug( doorOpenSwitch.getButtonStateRawDebounced());
	return doorOpenSwitch.getButtonStateRawDebounced();
}

/**
 * set washing program (washing + flushing + spinning)
 */
void chooseWashingProgram (int temperature)
{

	debug ("\nsetting WASHING program");
	long targetDuration = PROGRAM_WASHING_DEFAULT_TIME -15 + temperature / 2;
	debug ("\ntemperature=");
	debug (temperature);
	debug ("\nduration=");
	debug (targetDuration);
	currentProgram.setDuration(targetDuration);
	currentProgram.setMode(WASHING);
	programWashingNeedsToRun  = true;
	programFlushingNeedsToRun = true;
	programSpinningNeedsToRun = true;

	programWashingTimeLeft  = targetDuration;
	programFlushingTimeLeft = PROGRAM_FLUSHING_DEFAULT_TIME;
	programSpinningTimeLeft = PROGRAM_SPINNING_DEFAULT_TIME;
}


/**
 * set flushing program (flushing + spinning)
 */
void chooseFlushingProgram ()
{
	debug ("\nsetting FLUSHING program");
	currentProgram.setDuration(PROGRAM_FLUSHING_DEFAULT_TIME);
	currentProgram.setMode(FLUSHING);
	programWashingNeedsToRun  = false;
	programFlushingNeedsToRun = true;
	programSpinningNeedsToRun = true;

	programWashingTimeLeft  = 0;
	programFlushingTimeLeft = PROGRAM_FLUSHING_DEFAULT_TIME;
	programSpinningTimeLeft = PROGRAM_SPINNING_DEFAULT_TIME;

}

/**
 * set spinning only
 */
void chooseSpinningProgram ()
{
	debug ("\nsetting SPINNING program");
	currentProgram.setDuration(PROGRAM_SPINNING_DEFAULT_TIME);
	currentProgram.setMode(SPINNING);
	programWashingNeedsToRun  = false;
	programFlushingNeedsToRun = false;
	programSpinningNeedsToRun = true;

	programWashingTimeLeft  = 0;
	programFlushingTimeLeft = 0;
	programSpinningTimeLeft = PROGRAM_SPINNING_DEFAULT_TIME;

}

/**
 * reset all values. PowerOff will be implemented later
 */
void chooseEndProgram()
{
	debug ("\nsetting OFF program");
	programWashingNeedsToRun = false;
	programFlushingNeedsToRun = false;
	programSpinningNeedsToRun = false;
	programWashingTimeLeft  = 0;
	programFlushingTimeLeft = 0;
	programSpinningTimeLeft = 0;
	isProgramRunning = false;
}

/**
 * Poweroff
 */
void chooseOffProgram()
{
	// will be implemented later as soon as the power circuit is working.
}

/**
 * configures the program. Prepares anything that the program can be started by the start button.
 */
void configureProgram()
{

	// mark that there is no running program now, stop currently working ones -> you need to press start to start again
	isProgramRunning = false;


	if (currentProgramId >= programWashingIdMin && currentProgramId <= programWashingIdMax)
	{

		// we have a washing program
		chooseWashingProgram(temperature);
	}
	else if (currentProgramId == PROGRAM_FLUSHING_ID)
	{
		chooseFlushingProgram();
	}
	else if (currentProgramId == PROGRAM_SPINNING_PIN)
	{
		chooseSpinningProgram();
	}
	else if (currentProgramId == PROGRAM_OFF_ID)
	{
		chooseOffProgram();
	}
}

/**
 * switches to the next program part and starts it
 */
void switchToNextProgram()
{
	if (currentProgram.getMode() == WASHING)
	{
		programWashingNeedsToRun = false;
		if (programFlushingNeedsToRun)
		{
			chooseFlushingProgram();
		}
		else if (programSpinningNeedsToRun)
		{
			chooseSpinningProgram();
		}
		else
		{
			chooseEndProgram();
		}
	}
	else if (currentProgram.getMode() == FLUSHING)
	{
		 if (programSpinningNeedsToRun)
		{
			chooseSpinningProgram();
		}
		else
		{
			chooseEndProgram();
		}
	}
	else if (currentProgram.getMode() == SPINNING)
	{
		chooseEndProgram();
	}

}

void setup()
{
	Serial.begin(115200);
	debug("\n\nstarting\n");



	// initiate Inputs
	pinMode(DOOR_SHUT_PIN, INPUT_PULLUP);
	pinMode(START_BUTTON_PIN,INPUT_PULLUP);

	pinMode(PROGRAM_WOOL_PIN, INPUT_PULLUP);
	pinMode(PROGRAM_30_PIN, INPUT_PULLUP);
	pinMode(PROGRAM_40_PIN, INPUT_PULLUP);
	pinMode(PROGRAM_60_PIN, INPUT_PULLUP);
	pinMode(PROGRAM_90_PIN, INPUT_PULLUP);
	pinMode(PROGRAM_FLUSHING_PIN, INPUT_PULLUP);
	pinMode(PROGRAM_SPINNING_PIN, INPUT_PULLUP);
	pinMode(PROGRAM_OFF_PIN, INPUT_PULLUP);


	// initialize program array with -1 (not set)
//	for (int i=0; NUMBER_OF_MAX_CHAINED_PROGRAMS; i++)
//	{
//		programArray[i] = -1;
//	}

	// deactivate Motor for safety
	motor.standbyEnable();

//	myDisplay.writeCharToDigit(0, '0');
//	srf.writeNewValue();
//
//	debug("\nregister0=");
//	debug(srf.getNewValue(0));
//	debug("\nregister1=");
//	debug(srf.getNewValue(1));
//	debug("\n\n\n\n");
//	debug("\nregister0=");
//	debug(srf.getCurrentValue(0));
//	debug("\nregister1=");
//	debug(srf.getCurrentValue(1));


//	debug("array length is=");
//	debug(srf.getCurrentValueSize());
//	debug("\n\nHallo\n");
//
//	srf.setNewValue(0, 10);
//	srf.setNewValueBit(0, 0, 1);
//	srf.setNewValue(1, 0b00000001);
//	debug(srf.getNewValue(0));
//	debug("\n");
//	debug(srf.getNewValue(1));
//	debug("\n");
//	debug(srf.getNewValue(2));
//	debug("\n");
//
//	debug("\nstill running");
}

// The loop function is called in an endless loop
void loop()
{


	debug("\n\ntime: ");
	debug((long) millis());


	// process the timer
	currentProgram.Timer();
	currentProgram.debugTimer();
	calculateRemainingTimes();

	// update status of push Buttons
	startButton.processButtonState();
	doorOpenSwitch.processButtonState();

	// debug button status
	debug ("\nstartButton: ");
	debug (startButton.getButtonStateRawDebounced());
	debug ("\nDoorOpenSwitch: ");
	debug (doorOpenSwitch.getButtonStateRawDebounced());

	// update the currently choosen program
	updateCurrentProgram();

	// debug programs
	debug ("\nlastPorgamId=");
	debug (lastProgramId);
	debug (" currentProgramId=");
	debug (currentProgramId);

	// debug if program is running
	debug (" isProgramRunning=");
	debug (isProgramRunning);

	// check if program has changed compared to previous loop iteration
	if (currentProgramId != lastProgramId)
	{
		// program ID has been changed -> stop motor
		motor.standbyEnable();

		// configure new program
		debug ("configure Program");
		configureProgram();
	}

	//debug remaining times
	debug ("\ntimes remaining: washing: ");
	debug ((long) programWashingTimeLeft);
	debug (" flushing: ");
	debug ((long) programFlushingTimeLeft);
	debug (" spinning: ");
	debug ((long) programSpinningTimeLeft);




	// always check first, if the door is open. If it is open, set the motor to standby
	if (isDoorOpen())
	{
		// enable motor standby for emergency. If door is opened, the motor must not start or must stop if it is already running.
		debug("\nenable standby");
		motor.standbyEnable();

	}
	else
	{
		// door is shut. proceed with normal procedure

		// check if a program is already running -> check the current array. -1 is not running / not defined

		if (isProgramRunning)
		{
			debug("\nrunning a program: time remaining");
			debug(currentProgram.getRemainingTimeInSeconds());

			// a program is running
			// -> check counter if we need to go over to the next program part

			if (currentProgram.getRemainingTimeInSeconds() == 0)
			{
				// stop current program, because it has been finished
				currentProgram.end();
				switchToNextProgram();
			}

		}
		else
		{

			// currently no program is running. Not set yet
			// check if start button is pressed

			// check which program is choosen is something different then off
			if (currentProgramId != PROGRAM_OFF_ID)
			{


				// we have a valid program (using INPUT_PULLUP, true means unpressed -> negate condition)
				if (!startButton.getButtonStateRawDebounced())
				{
					// mark program as running
					currentProgram.start();
					isProgramRunning = true;
				}
			}



		}
	}
	delay(750);



	lastProgramId = currentProgramId;








}
