/*
  These are the general control functions

  setDial - go to a given dial position (0 to 99)
  resetDial - spin the dial twice to move reset discs A, B, and C. Stop at 0.
  goHome - tell dial to return to where the flag interruts the photogate. Depending on how the robot is attached to dial, this may be true zero or an offset
  setDiscsToStart - Go home, clear dial, go to starting numbers

  Supporting functions:
  gotoStep - go to a certain step value out of 8400. The DC motor has momentum so this function, as best as possible, goes to a step value
  stepRequired - calcs the steps of the encoder to get from current position to a given position
  convertDialToEncoder - given a dial number, how many encoder steps is that?
  convertEncoderToDial - given encoder value, what is the dial position?
  enable/disableMotor - controls the enable pin on motor driver
  turnCW/CCW - controls the direction pin on motor driver

*/

//Given a step value, go to that step
//Assume user has set direction prior to calling
//Returns the delta from what you asked and where it ended up
//Adding a full rotation will add a 360 degree full rotation
int gotoStep(int stepGoal, boolean addAFullRotation)
{
  int coarseSpeed = 200; //Speed at which we get to coarse window. 150, 200 works. 210, 230 fails
  int coarseWindow = 1000; //Once we are within this amount, switch to fine adjustment
  int fineSpeed = 50; //Less than 50 may not have enough torque
  int fineWindow = 32; //One we are within this amount, stop searching

  //Because we're switching directions we need to add extra steps to take
  //up the slack in the encoder
  if (direction == CW && previousDirection == CCW)
  {
    steps += switchDirectionAdjustment;
    if (steps > 8400) steps -= 8400;
    previousDirection = CW;
  }
  else if (direction == CCW && previousDirection == CW)
  {
    steps -= switchDirectionAdjustment;
    if (steps < 0) steps += 8400;
    previousDirection = CCW;
  }

  setMotorSpeed(coarseSpeed); //Go!
  //while (stepsRequired(steps, stepGoal) > coarseWindow) delayMicroseconds(10); //Spin until coarse window is closed
  while (stepsRequired(steps, stepGoal) > coarseWindow) ; //Spin until coarse window is closed

  //After we have gotten close to the first coarse window, proceed past the goal, then proceed to the goal
  if (addAFullRotation == true)
  {
    delay(500); //This should spin us past the goal
    while (stepsRequired(steps, stepGoal) > coarseWindow) delayMicroseconds(10); //Spin until coarse window is closed
  }

  setMotorSpeed(fineSpeed); //Slowly approach

  //  while (stepsRequired(steps, stepGoal) > fineWindow) delayMicroseconds(10); //Spin until fine window is closed
  while (stepsRequired(steps, stepGoal) > fineWindow) ; //Spin until fine window is closed

  setMotorSpeed(0); //Stop

  delay(200); //Wait for motor to stop

  int delta = steps - stepGoal;

  /*if (direction == CW) Serial.print("CW ");
    else Serial.print("CCW ");
    Serial.print("stepGoal: ");
    Serial.print(stepGoal);
    Serial.print(" / Final steps: ");
    Serial.print(steps);
    Serial.print(" / delta: ");
    Serial.print(delta);
    Serial.println();*/

  return (delta);
}

//Calculate number of steps to get to our goal
//Based on current position, goal, and direction
//Corrects for 8400 roll over
int stepsRequired(int currentSteps, int goal)
{
  if (direction == CW)
  {
    if (currentSteps >= goal) return (currentSteps - goal);
    else if (currentSteps < goal) return (8400 - goal + currentSteps);
  }
  else if (direction == CCW)
  {
    if (currentSteps >= goal) return (8400 - currentSteps + goal);
    else if (currentSteps < goal) return (goal - currentSteps);
  }

  Serial.println("stepRequired failed"); //We shouldn't get this far
  Serial.print("Goal: ");
  Serial.println(goal);
  Serial.print("currentSteps: ");
  Serial.print(currentSteps);

  return (0);
}

//Given a dial number, goto that value
//Assume user has set the direction before calling
//Returns the dial value we actually ended on
int setDial(int dialValue, boolean extraSpin)
{
  //Serial.print("Want dialValue: ");
  //Serial.println(dialValue);

  int encoderValue = convertDialToEncoder(dialValue); //Get encoder value
  //Serial.print("Want encoderValue: ");
  //Serial.println(encoderValue);

  gotoStep(encoderValue, extraSpin); //Goto that encoder value
  //Serial.print("After movement, steps: ");
  //Serial.println(steps);

  int actualDialValue = convertEncoderToDial(steps); //Convert back to dial values
  //Serial.print("After movement, dialvalue: ");
  //Serial.println(actualDialValue);

  return (actualDialValue);
}

//Spin until we detect the photo gate trigger
void goHome()
{
  byte fastSearch = 255; //Speed at which we locate photogate
  byte slowSearch = 50;

  turnCW();

  //Begin spinning
  setMotorSpeed(fastSearch);

  //If the photogate is already detected spin until we are out
  if (flagDetected() == true)
  {
    //Serial.println("We're too close to the photogate");
    int currentDial = convertEncoderToDial(steps);
    currentDial += 50;
    if (currentDial > 100) currentDial -= 100;
    setDial(currentDial, false); //Advance to 50 dial ticks away from here

    setMotorSpeed(fastSearch);
  }

  while (flagDetected() == false) delayMicroseconds(1); //Spin freely

  //Ok, we just zipped past the gate. Stop and spin slowly backward
  setMotorSpeed(0);
  delay(250); //Wait for motor to stop spinning
  turnCCW();

  setMotorSpeed(slowSearch);
  while (flagDetected() == false) delayMicroseconds(1); //Find flag

  setMotorSpeed(0);
  delay(250); //Wait for motor to stop

  //Adjust steps with the real-world offset
  steps = (84 * homeOffset); //84 * the number the dial sits on when 'home'

  previousDirection = CCW; //Last adjustment to dial was in CCW direction
}

//Set the discs to the current combinations (user selectable)
void resetDiscsWithCurrentCombo(boolean pause)
{
  //Go to starting conditions
  goHome(); //Detect magnet and center the dial
  delay(300);

  resetDial(); //Clear out everything
  delay(300);

  //Set discs to this combo
  turnCCW();
  int discAIsAt = setDial(discA, false);
  Serial.print("DiscA is at: ");
  Serial.println(discAIsAt);
  if(pause == true) messagePause("Verify disc position");

  turnCW();
  //Turn past disc B one extra spin
  int discBIsAt = setDial(discB, true);
  Serial.print("DiscB is at: ");
  Serial.println(discBIsAt);
  if(pause == true) messagePause("Verify disc position");

  turnCCW();
  int discCIsAt = setDial(discC, false);
  Serial.print("DiscC is at: ");
  Serial.println(discCIsAt);
  if(pause == true) messagePause("Verify disc position");

  discCAttempts = -1; //Reset
}


//Given a dial value, covert to an encoder value (0 to 8400)
//If there are 100 numbers on the dial, each number is 84 ticks wide
int convertDialToEncoder(int dialValue)
{
  int encoderValue = dialValue * 84;

  if (encoderValue > 8400)
  {
    Serial.print("Whoa! Your trying to go to a dial value that is illegal. encoderValue: ");
    Serial.println(encoderValue);
    while (1);
  }

  return (84 * dialValue);
}

//Given an encoder value, tell me where on the dial that equates
//Returns 0 to 99
//If there are 100 numbers on the dial, each number is 84 ticks wide
int convertEncoderToDial(int encoderValue)
{
  int dialValue = encoderValue / 84; //2388/84 = 28.43
  int partial = encoderValue % 84; //2388%84 = 36

  if (partial >= (84 / 2)) dialValue++; //36 < 42, so 28 is our final answer

  if (dialValue > 99) dialValue = 0;

  return (dialValue);
}

//Reset the dial
//Turn CCW, past zero, then continue until we return to zero
void resetDial()
{
  turnCCW();

  //If we're too close to zero, add 50
  if (convertEncoderToDial(steps) > 97 || convertEncoderToDial(steps) < 4)
  {
    //Serial.println("We're too close to zero");
    setDial(50, false); //Advance to 50 dial ticks away from here
  }
  previousDirection = CCW;

  setDial(0, true); //Turn to zero with an extra spin

  //Second spin
  
  //If we're too close to zero, add 50
  if (convertEncoderToDial(steps) > 97 || convertEncoderToDial(steps) < 4)
  {
    //Serial.println("We're too close to zero");
    setDial(50, false); //Advance to 50 dial ticks away from here
  }

  previousDirection = CCW;

  setDial(0, false); //Turn to zero
}

//Tells the servo to pull down on the handle
//After a certain amount of time we test to see the actual
//position of the servo. If it's properly moved all the way down
//then the handle has moved and door is open!
//If position is not more than a determined amount, then return failure
boolean tryHandle()
{
  //Attempt to pull down on handle
  handleServo.write(servoTryPosition);

  delay(500); //Wait for servo to move, but don't let it stall for too long and burn out

  //Check if we're there
  handlePosition = averageAnalogRead(servoPosition);
  if (handlePosition > handleOpenPosition)
  {
    //Holy smokes we're there!
    return (true);
  }

  //Ok, we failed
  //Return to resting position
  handleServo.write(servoRestingPosition);

  delay(500); //Wait for servo to return to reseting position

  return (false);
}

//Sets the motor speed
void setMotorSpeed(int speedValue)
{
  analogWrite(motorPWM, speedValue);
}

//Gives the current being used by the motor in milliamps
int readMotorCurrent()
{
  const int VOLTAGE_REF = 5;  //This board runs at 5V
  const float RS = 0.1;          //Shunt resistor value (in ohms)

  float sensorValue = averageAnalogRead(currentSense);

  // Remap the ADC value into a voltage number (5V reference)
  sensorValue = (sensorValue * VOLTAGE_REF) / 1023;

  // Follow the equation given by the INA169 datasheet to
  // determine the current flowing through RS. Assume RL = 10k
  // Is = (Vout x 1k) / (RS x RL)
  float current = sensorValue / (10 * RS);

  return (current * 1000);
}

//Tell motor to turn dial clock wise
void turnCW()
{
  direction = CW;
  digitalWrite(motorDIR, HIGH);
}

//Tell motor to turn dial counter-clock wise
void turnCCW()
{
  direction = CCW;
  digitalWrite(motorDIR, LOW);
}

//Turn on the motor controller
void enableMotor()
{
  digitalWrite(motorReset, HIGH);
}

void disableMotor()
{
  digitalWrite(motorReset, LOW);
}

//Interrupt routine when encoderA pin changes
void countA()
{
  if (direction == CW) steps--;
  else steps++;
  if (steps < 0) steps = 8399; //Limit variable to zero
  if (steps > 8399) steps = 0; //Limit variable to 8399
}

//Interrupt routine when encoderB pin changes
void countB()
{
  if (direction == CW) steps--;
  else steps++;
  if (steps < 0) steps = 8399; //Limit variable to zero
  if (steps > 8399) steps = 0; //Limit variable to 8399
}

//Checks to see if we detect the photogate being blocked by the flag
boolean flagDetected()
{
  if (digitalRead(photo) == LOW) return (true);
  return (false);
}

//Play a music tone to indicate door is open
void announceSuccess()
{
  //Beep! Piano keys to frequencies: http://www.sengpielaudio.com/KeyboardAndFrequencies.gif
  tone(buzzer, 2093, 150); //C
  delay(150);
  tone(buzzer, 2349, 150); //D
  delay(150);
  tone(buzzer, 2637, 150); //E
  delay(150);
}

//Disc A is the safety disc that prevents you from feeling the edges of the wheels
//It has 12 upper and 12 low indents which means 100/24 = 4.16 per lower indent
//So it moves a bit across the wheel. We could do floats, instead we'll do a lookup
//Values were found by hand: What number is in the middle of the indent?
int lookupIndentValues(int indentNumber)
{
  switch (indentNumber)
  {
    //Values found by measureIndents() function
    case 0: return (99); //98 to 1 on the wheel
    case 1: return (8); //6-9
    case 2: return (16); //14-17
    case 3: return (24); //23-26
    case 4: return (33); //31-34
    case 5: return (41); //39-42
    case 6: return (50); //48-51
    case 7: return (58); //56-59
    case 8: return (66); //64-67
    case 9: return (74); //73-76
    case 10: return (83); //81-84
    case 11: return (91); //90-93
    case 12: return (-1); //Not valid

      /* Original values found by hand
        case 0: return (0); //98 to 1 on the wheel
        case 1: return (8); //6-9
        case 2: return (16); //14-17
        case 3: return (24); //23-26
        case 4: return (32); //31-34
        case 5: return (41); //39-42
        case 6: return (49); //48-51
        case 7: return (58); //56-59
        case 8: return (66); //64-67
        case 9: return (74); //73-76
        case 10: return (83); //81-84
        case 11: return (91); //90-93
        case 12: return (-1); //Not valid
      */

  }
}

//Print a message and wait for user to press a button
//Good for stepping through actions
void messagePause(char* message)
{
  Serial.println(message);
  while (!Serial.available()); //Wait for user input
  Serial.read(); //Throw away character
}

//See if user has pressed a button. If so, pause
void checkForUserPause()
{
  if (Serial.available()) //See if user has pressed a button
  {
    Serial.read(); //Throw out character
    Serial.print("Pausing. Press button to continue.");

    while (!Serial.available()); //Wait for user to press button to continue
  }
}

//Given a spot on the dial, what is the next available indent in the CCW direction
//Takes care of wrap conditions
//Returns the dial position of the next ident
int getNextIndent(int currentDialPosition)
{
  for (int x = 0 ; x < 12 ; x++)
  {
    if (indentsToTry[x] == true) //Are we allowed to use this indent?
    {
      byte nextDialPosition = lookupIndentValues(x);
      if (nextDialPosition > currentDialPosition) return (nextDialPosition);
    }
  }

  //If we never found a next dial value then we have wrap around situation
  //Find the first indent we can use
  for (int x = 0 ; x < 12 ; x++)
  {
    if (indentsToTry[x] == true) //Are we allowed to use this indent?
    {
      return (lookupIndentValues(x));
    }
  }
}

//Takes an average of readings on a given pin
//Returns the average
int averageAnalogRead(byte pinToRead)
{
  byte numberOfReadings = 8;
  unsigned int runningValue = 0;

  for (int x = 0 ; x < numberOfReadings ; x++)
    runningValue += analogRead(pinToRead);
  runningValue /= numberOfReadings;

  return (runningValue);
}
