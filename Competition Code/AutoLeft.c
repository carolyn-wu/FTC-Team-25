#pragma config(Hubs,  S1, HTServo,  HTMotor,  HTMotor,  HTMotor)
#pragma config(Sensor, S2,     IRSeeker,       sensorHiTechnicIRSeeker1200)
#pragma config(Sensor, S3,     HTMC,           sensorI2CCustom)
#pragma config(Sensor, S4,     lightSensor,    sensorLightInactive)
#pragma config(Motor,  motorA,           ,             tmotorNXT, openLoop)
#pragma config(Motor,  motorB,           ,             tmotorNXT, openLoop)
#pragma config(Motor,  motorC,           ,             tmotorNXT, openLoop)
#pragma config(Motor,  mtr_S1_C2_1,     driveRight,    tmotorTetrix, openLoop, reversed)
#pragma config(Motor,  mtr_S1_C2_2,     driveLeft,     tmotorTetrix, openLoop)
#pragma config(Motor,  mtr_S1_C3_1,     grabberArm,    tmotorTetrix, PIDControl)
#pragma config(Motor,  mtr_S1_C3_2,     driveSide,     tmotorTetrix, PIDControl)
#pragma config(Motor,  mtr_S1_C4_1,     motorH,        tmotorTetrix, openLoop)
#pragma config(Motor,  mtr_S1_C4_2,     motorI,        tmotorTetrix, openLoop)
#pragma config(Servo,  srvo_S1_C1_1,    gravityShelf,         tServoStandard)
#pragma config(Servo,  srvo_S1_C1_2,    IRServo,              tServoStandard)
#pragma config(Servo,  srvo_S1_C1_3,    servo3,               tServoNone)
#pragma config(Servo,  srvo_S1_C1_4,    Ramp,                 tServoStandard)
#pragma config(Servo,  srvo_S1_C1_5,    servo5,               tServoNone)
#pragma config(Servo,  srvo_S1_C1_6,    servo6,               tServoNone)
//*!!Code automatically generated by 'ROBOTC' configuration wizard               !!*//

/////////////////////////////////////////////////////////////////////////////////////////////////////
//
//                           Autonomous Mode Code Template
//
// This file contains a template for simplified creation of an autonomous program for an TETRIX robot
// competition.
//
// You need to customize two functions with code unique to your specific robot.
//
/////////////////////////////////////////////////////////////////////////////////////////////////////

#include "JoystickDriver.c"  //Include file to "handle" the Bluetooth messages.

#include "../library/sensors/drivers/hitechnic-irseeker-v2.h"
#include "../library/sensors/drivers/hitechnic-compass.h"
#include "../library/sensors/drivers/lego-light.h"

#include "Lib/Lib12-13.c"

/////////////////////////////////////////////////////////////////////////////////////////////////////
//
//                                    initializeRobot
//
// Prior to the start of autonomous mode, you may want to perform some initialization on your robot.
// Things that might be performed during initialization include:
//   1. Move motors and servos to a preset position.
//   2. Some sensor types take a short while to reach stable values during which time it is best that
//      robot is not moving. For example, gyro sensor needs a few seconds to obtain the background
//      "bias" value.
//
// In many cases, you may not have to add any code to this function and it will remain "empty".
//
/////////////////////////////////////////////////////////////////////////////////////////////////////

void initializeRobot()
{
  	// Place code here to sinitialize servos to starting positions.
  	// Sensors are automatically configured and setup by ROBOTC. They may need a brief time to stabilize.
  	servo[gravityShelf] = SHELFDOWN;
  	servo[IRServo] = IRUP;

	/*
	 * Assume lined up perpendicular to the pegs.
	 */
	HTMCsetTarget(HTMC);

	nMotorPIDSpeedCtrl[driveLeft] = mtrSpeedReg;
	nMotorPIDSpeedCtrl[driveRight] = mtrSpeedReg;
	nMotorPIDSpeedCtrl[driveSide] = mtrSpeedReg;

	// the default DSP mode is 1200 Hz.
	tHTIRS2DSPMode mode = DSP_1200;

	// set the DSP to the new mode
	HTIRS2setDSPMode(IRSeeker, mode);

  	return;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////
//
//                                         Main Task
//
// The following is the main code for the autonomous robot operation. Customize as appropriate for
// your specific robot.
//
// The types of things you might do during the autonomous phase (for the 2008-9 FTC competition)
// are:
//
//   1. Have the robot follow a line on the game field until it reaches one of the puck storage
//      areas.
//   2. Load pucks into the robot from the storage bin.
//   3. Stop the robot and wait for autonomous phase to end.
//
// This simple template does nothing except play a periodic tone every few seconds.
//
// At the end of the autonomous period, the FMS will autonmatically abort (stop) execution of the program.
//
/////////////////////////////////////////////////////////////////////////////////////////////////////

/*
 * lookForIRBeacon
 *
 * Drive sideways until we see the beacon in segment 5
 * of the IR receiver.
 */
void lookForIRBeacon(void)
{
	int dir;

    dir = HTIRS2readACDir(IRSeeker);

	if (dir != 5) {
		motor[driveSide] = 50;
	} else {
		return;
	}

	while (dir != 5) {
		dir = HTIRS2readACDir(IRSeeker);
	}

	motor[driveSide] = 0;
}

/*
 * lookForWhiteLine
 *
 * Look for the left edge of the white line by
 * moving sideways until we see the transistion.
 */
void lookForWhiteLine(direction_t dir)
{
	int val;

	switch (dir) {
		case LEFT:
			motor[driveSide] = -25;
			break;
		case RIGHT:
			motor[driveSide] = 25;
			break;
		case NO_DIR:
		default:
			return;
	}

	LSsetActive(lightSensor);
	val = LSvalNorm(lightSensor);
	while (val <= 22) {
		val = LSvalNorm(lightSensor);
	}
	LSsetInactive(lightSensor);

	motor[driveSide] = 0;
}

/*
 * alignToPeg
 *
 * We may have moved off perpendicular to the
 * peg during our travels.  If so, rotate back.
 */
direction_t alignToPeg(void)
{
	int bearing;

	// Are we aligned?  If so we do nothing.
	bearing = HTMCreadRelativeHeading(HTMC);
	if (bearing == 0) {
		return NO_DIR;
	} else {
		turn(bearing);
		if (bearing < 0) {
			return LEFT;
		} else {
			return RIGHT;
		}
	}
}

/*
 * placeRing
 *
 * Completely automated place functionality for putting
 * a ring on a peg.
 *
 * Assumes we are on the platform and in front of the
 * peg we want to place the ring on.
 */
void placeRing(void)
{
	raiseShelfToPlacePosition();

	// We are aligned, and on the white line so
	// move forward until we hit the proper strength
	// value from the beacon.
	moveToBeacon(BEACON_TARGET_STRENGTH);

	lowerShelfToDischargePosition();

	deployPusher();
	moveBackward(3);
}

task main()
{
	direction_t dir;

  	initializeRobot();

  	// waitForStart(); // Wait for the beginning of autonomous phase.

  	// Move forward a predetermined amount.
	nMotorEncoder[driveRight] = 0;
	nMotorEncoderTarget[driveRight] = 360;
	motor[driveRight] = 50;
	motor[driveLeft] = 50;

	WAIT_UNTIL_MOTOR_OFF;

  	// Read IR sensor.
	lookForIRBeacon();

	// We found the IR beacon, we should be to the
 	// left of the white line.  Move to the line.
	lookForWhiteLine(RIGHT);

	// If we were knocked off target, realign ourself.
	dir = alignToPeg();

	/*
	 * The rotatation may have knocked us off the white line
	 * If we rotated right, look back to the left, if we rotated
	 * left, do the opposite.
	 */
	switch (dir) {
		case RIGHT:
			lookForWhiteLine(LEFT);
			break;
		case LEFT:
			lookForWhiteLine(RIGHT);
			break;
		case NO_DIR:
		default:
	}

	placeRing();

	while (true)
	{}
}
