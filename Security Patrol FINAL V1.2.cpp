/*
MTE 100 FINAL PROJECT
	EV3 Patrolling Defence System
	November 24, 2015

GROUP 42
	O.Andrienko
	D.P. Kushalker
	A.P. Rathke
	S. Srinivasan
*/

//FUNCTION PROTOTYPES
bool checkPIN();		
bool authenticate();
void rotateCW(int angle);
bool movementScan();
void pursuitEnemy();
int followTarget(int * encDist, int * degRot);
bool wayPointScan(int *path, int *rot, int *scan, int lim, bool first, bool forward);
void initScan(int *scan, int scanForward);
bool checkScan(int *scan, int scanBackward, bool forward);
void reverseArrays(int *path, int *rot, int const lim);
void resetMotors();

const int MAX = 50;

/* 
OLES ANDRIENKO'S FUNCTIONS:
 
 - Authenticate functions -> checkPIN(), authenticate()
 
 - Scan functions -> initScan(), checkScan()
 
 - Follow given path -> wayPointScan()
 
 - Additional -> resetMotors(), reverseArrays()
*/

/*
 Function checks for user input. Must use the sleep() as a timer 
 as getButtonPress did not work well in a while loop.
*/
bool checkPIN()
{
    
    int pinReal[4] = {1,2,3,4}; //password
    int pinEnter[4] = {0,0,0,0};
    bool permission = true;
    setLEDColor(ledGreen);
    
    displayBigStringAt(0, 120, "Press ENTER");
    //waits for button press
    while(getButtonPress(buttonEnter) == 0){}
    
    
    setLEDColor(ledOrange);
    playSoundFile("Download");
    displayBigStringAt(0, 100, "Input PIN");
    sleep(500);
    for (int i = 0; i < 4; i++) { //fills pinEnter
        sleep(1500); // gives time to hold a button
        if (getButtonPress(buttonLeft)) {
            pinEnter[i] = 1;
            playSoundFile("One");
        }
        else if (getButtonPress(buttonUp)) {
            pinEnter[i] = 2;
            playSoundFile("Two");
        }
        else if (getButtonPress(buttonRight)) {
            pinEnter[i] = 3;
            playSoundFile("Three");
        }
        else if (getButtonPress(buttonDown)){
            pinEnter[i] = 4;
            playSoundFile("Four");
        } else {
            playSoundFile("General alert");
        }
        
        displayString(6+i,"%i",pinEnter[i]);
    }
    
    for (int i = 0; i < 4; i++) { //checks pin entered
        if (pinReal[i] != pinEnter[i])
            permission = false;
    }
    return permission;
}

bool authenticate() { // main authenticate function
    sleep(500);
    playSoundFile("Hello");
    playSoundFile("Ready");
    for (int i=0; i <3; i++) { // checks fin. Allows to retry
        int auth = checkPIN();
        if (auth) {
            displayBigStringAt(0, 40, "CORRECT PIN");
            playSoundFile("Thank you");
            setLEDColor(ledGreenPulse);
            sleep(3000);
            return true;
        } else {
            displayBigStringAt(0, 40, "INCORRECT PIN");
            displayBigStringAt(20, 20, "Try Again");
            playSoundFile("Error alarm");
            setLEDColor(ledRedPulse);
            sleep(3000);
            eraseDisplay();
        }
    }
    
    eraseDisplay();
    displayBigStringAt(0, 120, "3 WRONG PINS");
    displayBigStringAt(0, 100, "ROBOT LOCKED");
    setLEDColor(ledRedPulse);
    sleep(500);
    playSoundFile("Goodbye");
    
    return false; // program ends when continued in main()
}


//fills the initial scan arrays
void initScan(int *scan, int scanForward) {

	for (int j=0; j < 11; j++) { //scan every 30deg and store
		scan[scanForward*11 + j] = SensorValue[S1];
		motor[motorB] = 5;
		while(getMotorEncoder(motorB) != 30){}
		motor[motorB] = 0;
		resetMotorEncoder(motorB);

		//sleep(10);
	}

	motor[motorB] = -5; //turn back sonar head
	while (getMotorEncoder(motorB) != -330){}
	motor[motorB] = 0;

}

//compares the scans to the initial scans
bool checkScan(int *scan, int scanBackward, bool forward) {
	eraseDisplay();
	displayString(9, "IN CHECKSCAN");

	resetMotorEncoder(motorB);
	int encB = 0;

	int n;
	if (!forward)
		n = 6; //if going backward, must compare scans
                // differently
	else
		n = 0;

	for (int j=0; j < 11; j++) {
		//displayString(j,"SVal=%i --- scan[%i]=%i", SensorValue[S1],(scanBackward*11+j)+n,scan[(scanBackward*11+j)+n]);
        //checks scan while a tolerance of 30
		if (abs(SensorValue[S1] - scan[(scanBackward*11+j)+n]) > 30) {
			if(movementScan()) //checks if there is movement
			{

                //if movement detected,
                //return head back to start and return true to follow
				resetMotorEncoder(motorB);

				motor[motorB] = -5;
				while(getMotorEncoder(motorB) > -encB + 32)
				{}
				motor[motorB] = 0;

				return true;
			}

		}
		if (j+n == 11 && !forward)
			n=-6; //needed when backwards scans are compared to intital scans
		motor[motorB] = 5;
		while(getMotorEncoder(motorB) != 30){} //
		motor[motorB] = 0;
		encB += getMotorEncoder(motorB);
		resetMotorEncoder(motorB);

		sleep(100);
	}

	motor[motorB] = -15; //turn back sonar
	while (getMotorEncoder(motorB) != -330){}
	motor[motorB] = 0;

	return false;
}

/*
 Main pathfollowing function that also fills and checks scan array
*/
bool wayPointScan(int *path, int *rot, int *scan, int const lim, bool first, bool forward) {
	eraseDisplay();
	displayString(9, "IN WAYPOINT SCAN");


	int countDist = 0, scanForward = 0, scanBackward = 0, sum = 0;
	if(first || forward) {
		//int countDist=4000; //distance until scan
		if(first)
			countDist = 0; // does scan initially
		/*else
			countDist = 4000;*/
		scanForward = 0; //index for storing intial path[]
		if(!first)
			reverseArrays(path, rot, lim);
	} else {
		for(int i=0; i < lim; i++)
			sum+=path[i];
		countDist=sum%4000;
		scanBackward = sum/4000 - 1; //start index in reverse
		reverseArrays(path, rot, lim);
	}

	int targetDist=0; //distance from path[]




	for (int i=0; i < lim; i++) { //loop for all distances in path[]
		resetMotors(); // reset motors after rotate()
		targetDist = path[i];

		displayString(3, "DIST %i", path[i]);

		motor[motorA] = 100;
		motor[motorD] = 100;

		while(getMotorEncoder(motorA) <= targetDist) { //moves until targetDist reached

			if (getMotorEncoder(motorA) >= countDist) { // or until scan is needed
				motor[motorA] = 0;
				motor[motorD] = 0;

				if (first) { // if first time, fill scan
					initScan(scan, scanForward);
					scanForward++;
				} else {
					if(checkScan(scan, scanBackward, forward))
					{
						pursuitEnemy();
						return true;
					}
					scanBackward--;
				}

				targetDist -= countDist; //needed when reseting motors
				resetMotors();
				countDist = 4000; //sets countDist back to 4000
				//if (targetDist <= 0) { break; } //needed if targetDist = 0
				motor[motorA] = 100;
				motor[motorD] = 100;
				sleep(100); //this or the above if statement if targetDist=0
			}

		}

		motor[motorA] = 0;
		motor[motorD] = 0;
		if (countDist > getMotorEncoder(motorA)) {
			countDist -= getMotorEncoder(motorA); //remembers dist relative to countDist
		} else {
			countDist = 4000; //incase something goes wrong
		}
		resetMotors(); //reset for rotateCW()

		displayString(3, "ROT %i", rot[i]);
		if (!forward && i+1 < lim) { //not forward and rot[+1]
				rotateCW(rot[i+1]);
		} else if(forward) {
				rotateCW(rot[i]);
			}
	}

	rotateCW(180);//rotate backwards for next repeat
	return false;
}

void resetMotors() {
	resetMotorEncoder(motorA);
	resetMotorEncoder(motorD);
	resetMotorEncoder(motorB);
}

void reverseArrays(int *path, int *rot, int const lim) {

	int temp[MAX];
	for(int i = 0; i < MAX; i++)
		temp[i] = -1;

	for (int i = 0; i < lim; i++){
		temp[i] = path[i];
	}
	for(int i=1; i < lim; i++) {
		path[i] = temp[(lim)-i];
	}
	for (int i = 0; i < lim; i++){
		temp[i] = rot[i];
	}
	for(int i=1; i < lim; i++) {
		rot[i] = -1*temp[(lim-1)-i];
	}

}

/* 
DHRUV KUSHALKAR'S FUNCTIONS:
 
 - Scanning for movement -> movementScan()
*/

// Function scans for movement in the environment
bool movementScan ()
{
	int sum = 0;
	for(int i = 0; i < 30; i++)
	{
		sum += SensorValue(S1);
		wait1Msec(10);
	}

	float measure1 = sum / 30;

	sum = 0;

	for(int i = 0; i < 30; i++)
	{
		sum += SensorValue(S1);
		wait1Msec(10);
	}

	float measure2 = sum / 30;


	if(abs(measure1 - measure2) > 30)
		return true;

	return false;
}

/* 
SANJAY SRINIVASAN'S FUNCTIONS:
 
 - Aggressively follow target -> pursuitEnemy()
*/

/*
Chases target once detected, done by comparing sensor values,
rotating certain directions, and going forward as a result.
*/
void pursuitEnemy()
{
	playSoundFile("Detected");

	resetMotorEncoder(motorB);

	motor[motorB] = -5;
	while(getMotorEncoder(motorB) > -30)
	{}

	motor[motorB] = 0;

	eraseDisplay();
	displayString(9, "IN PURSUITENEMY");		//TEST CODE


	const int PURSUIT_RANGE = 75;
	const int MAX_POWER = 100;
	time1[T1] = 0;
	bool exit = false;

	motor[motorA] = MAX_POWER;
	motor[motorD] = -1 * MAX_POWER;
	clearTimer(T2);
	while(SensorValue(S1) > PURSUIT_RANGE && time1(T2) < 9000)
	{}

	do
	{
		// if only right sensor detects person; sensor value of s4 must be less than sensor value at s2 and s1 for the turn to execute(right turn)
		if(SensorValue(S4) < PURSUIT_RANGE && SensorValue(S4) < SensorValue(S2) && SensorValue(S4) < getUSDistance(S1))
		{
			motor[motorD] = -1*MAX_POWER;
			motor[motorA] = MAX_POWER;
			if(SensorValue(S4) < 10)
				exit = true; //end the pursuit if the human gets too close
			while(SensorValue(S4) < PURSUIT_RANGE && SensorValue(S4) < SensorValue(S2) && SensorValue(S4) < getUSDistance(S1))
			{}
		}
		//if only left sensor detects person; sensor value of s2 must be less than sensor vlaue at s4 and s1 for the turn to execute(left turn)
		else if(SensorValue(S2) < PURSUIT_RANGE && SensorValue(S2) < SensorValue(S4) && SensorValue(S2) < getUSDistance(S1))
		{
			motor[motorD] = MAX_POWER;
			motor[motorA] = -1*MAX_POWER;
			if(SensorValue(S2) < 10)
				exit = true; //end the pursuit if the human gets too close
			while(SensorValue(S2) < PURSUIT_RANGE && SensorValue(S2) < SensorValue(S4) && SensorValue(S2) < getUSDistance(S1))
			{}
		}
		//if only forward sensor detects person; sensor value of s1 must be less than the other sensors for this to be true(drive straight)
		else if(getUSDistance(S1) < PURSUIT_RANGE + 70 && SensorValue(S1) < SensorValue(S2) && getUSDistance(S1) < SensorValue(S4))
		{
			motor[motorA] = MAX_POWER - 15;
			motor[motorD] = MAX_POWER;
			if(getUSDistance(S1) < 25)
				exit = true; //end the pursuit if the human gets too close
			while(getUSDistance(S1) < PURSUIT_RANGE && SensorValue(S1) < SensorValue(S2) && getUSDistance(S1) < SensorValue(S4))
			{}
		}
		else
			motor[motorA] = motor[motorD] =	motor[motorB] = 0;
	}
	while(time1(T1) < 200000 && !exit);
}

/* 
ALEXANDER RATHKE'S FUNCTIONS:
 
 - Following Target & Storing path -> followTarget()
 
 - Additional -> rotateCW()
*/


/*								FOLLOW TARGET
DESCRIPTION
================
Function follows a human target through the use of the two
front ultrasonic sensors. As it follows the person it stores
"waypoints" at certain intervals, which are represented by
two parallel arrays - encDist and degRot. The function
ends when the robot detects someone within a minimum
range using the ultrasonic sensor on the back of the robot.

Waypoints:
Created whenever the robot rotates or when it goes straight
for an amount larger than 32700 encoder counts, to prevent
encoder value rollover (encoder max val before rollover
is 32,767).

encDist:
Stores the value of motorA's motor encoder at each waypoint,
reseting the encoder after each waypoint. The path
between waypoints will always be linear.

degRot:
Stores the degrees of rotation at each waypoint, all
turns are done on a point - no arcs, and waypoints
are made at all turns.

Turning:
Turning is done by using the front two ultrasonic sensors,
each angled at roughly 45 degrees to opposite sides. When
a sensor on one side detects something within a given
range, the robot will make a fixed degree turn in that
direction and continue proceeding forward.

Limitations:
This code many serious limitations, one key limit being
navigating in tight spaces with many objects. There is
no logic which distinguishes a person being detected
rather than an object/wall. Additionally using fixed
degree rotations may limit the storing of some paths,
and/or lead to akward movement patterns required to
store certain paths. The choice to go with a fixed
degree rotation was made after extensive testing
with variable degree rotations, which led to
errors, inaccuracy, and it was ultimately
determined that a variable degree rotation
would require more time than available
to properly implement.

CONFIGURATION
================
S1: Top Ultrasonic sensor
S2: Left Ultrasonic sensor
S4: Front Right Ultrasonic Sensor
motorA = left motor
motorD = right motor
*/
int followTarget(int *encDist, int *degRot)
{
	//VARIABLES/CONSTANTS
		//Range for detecting an object
	const int RANGE = 60;
		//Degrees rotated when turning
	const int turn = 40;
		//Turns to true when robot gets too close to user, stops following when true
	bool stopFollowing = false;
		//Number of waypoints created, returned by function
	int pointNum = 0;


	//Wait for user detection
	while(SensorValue(S1) > 50)
	{}

	wait1Msec(1000);


	//Loop that repeats while a target is being followed
	do
	{
		//Resets encoder to store only the distance travalled from previous waypoints
		resetMotorEncoder(motorA);

		motor[motorA] = 100;
		motor[motorD] = 100;

		/*
		Continues moving forward while S2 and S4 don't detect anything within range, and
		S1 (rear mounted sensor) doesn't detect anything too close to robot, and
		the motor encoder value for motorA doesn't go over 32700, as at 32767 it
		will rollover and become negative, causing issues with recording distance.
		*/
		while(SensorValue(S2) > RANGE && SensorValue(S4) > RANGE
					&& SensorValue(S1) > 30 && getMotorEncoder(motorA) < 32700)
		{}

		/*
		Don't make a change if both front sensors detect something, unless back sensor is
		within minimum range (in case of robot about to run into a wall, still want to react).
		*/
		if(!(SensorValue(S2) <= RANGE && SensorValue(S4) <= RANGE) || SensorValue(S1) <= 30)
		{
			//Stores the distance travalled since last waypoint
			encDist[pointNum] = getMotorEncoder(motorA);
			//Increment number of waypoints created
			pointNum++;

			//Target too close to robot, stops following
			if (SensorValue(S1) <= 30)
			{
				motor[motorA] = 0;
				motor[motorD] = 0;
				stopFollowing = true;
			}
			//Target detected on the left side, turns left
			else if(SensorValue(S2) <= RANGE)
			{
				rotateCW(-turn);
				degRot[pointNum] = -turn;		//Stores degrees of rotation
			}
			//Target detected on the right side, turns right
			else if(SensorValue(S4) <= RANGE)
			{
				rotateCW(turn);
				degRot[pointNum] = turn;
			}
			/*
			Note, no else branch was added as sometimes the sensors may get incorrect values,
			falsely exiting the while loop and then going into the else branch. Thus the
			decision was made to keep all statements in else if branches for more
			reliable code.
			*/



		}
	//Keeps following until user is too close to robot, and stopFollowing is set to true in if branch
	}	while(stopFollowing == false);

	//Returns number of waypoints created
	return pointNum;
}


/*
Rotates clockwise for a value of degrees input as
a parameter (int angle).
*/
void rotateCW(int angle)
{
	//Resets motor encoders
	resetMotorEncoder(motorA);
	resetMotorEncoder(motorD);
	/*
	Revs is a value proportional to the angle of
	rotation, with a constant value that was
	determined through testing.
	*/
	int revs = (6.1)*angle;

	//LEFT TURN (negative degrees)
	if (revs < 0)
	{
		motor[motorA] = -100;
		motor[motorD] = 100;
		while(getMotorEncoder(motorA) > revs)
		{}
	}
	//RIGHT TURN (positive degrees or 0)
	else
	{
		motor[motorA] = 100;
		motor[motorD] = -100;
		while(getMotorEncoder(motorA) < revs)
		{}
	}

	motor[motorA] = 0;
	motor[motorD] = 0;
}


//Wrote by Alexander and Oles
task main()
{

	//INITIALIZATION
		//MOTORS
	motor[motorA] = 0;	//motorA is left
	motor[motorD] = 0;	//motorD is right
	motor[motorB] = 0;	//motorB is middle

	resetMotorEncoder(motorA);
	resetMotorEncoder(motorD);
	resetMotorEncoder(motorB);
		//SENSORS
	SensorType[S1] = sensorEV3_Ultrasonic; //top ultrasonic sensor is S1
	SensorType[S2] = sensorSONAR; //left ultrasonic sensor is S2
	SensorType[S4] = sensorSONAR; //right ultrasonic sensor is S4
		//REVERSING MOTOR A&D
	setMotorReversed(motorA, 1);
	setMotorReversed(motorD, 1);
		//SOUND
	setSoundVolume(100);

	//Parallel arrays used to store path data
	int distances[MAX];		//Encoder distances between waypoints
	int degRot[MAX];		//Degrees of clockwise rotation at each waypoint

	//fill arrays with 0s
	for(int i = 0; i < MAX; i++)
	{
		distances[i] = 0;
		degRot[i] = 0;
	}

	//If person doesn't get pin in 3 tries, code won't run
	if(authenticate())
	{
		waitForButtonPress();
		eraseDisplay();
		wait1Msec(3000);

		int numWaypoints = followTarget(distances, degRot);

		int scan[12][12];
		bool first = true, forward = true;
		wait1Msec(10000);

		//follows the path backwards and fills scan array
		rotateCW(180);
		reverseArrays(distances, degRot, numWaypoints);
		wayPointScan(distances, degRot, scan, numWaypoints, first, forward);

		//repeats path until a change is dedected at a scan
		first = false;
		while(!wayPointScan(distances, degRot, scan, numWaypoints, first, forward))
		{
			forward = !forward;
		}

		//Shutdown
		motor[motorA] = 0;
		motor[motorD] = 0;
		displayBigStringAt(0, 120, "DONE");
		playSoundFile("Goodbye");
		wait1Msec(5000);
	}
}
