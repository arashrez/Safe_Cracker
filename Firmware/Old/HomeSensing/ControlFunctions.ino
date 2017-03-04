//Given a step value, go to that step
//Assume user has set direction prior to calling
//Returns the delta from what you asked and where it ended up
//Adding a full rotation will add a 360 degree full rotation
int gotoStep(int stepGoal, boolean addAFullRotation)
{
  if(stepGoal > 8399)
  {
    Serial.println("Stepgoal out of bounds");
    return(0);
  }
  //Serial.print("Going to: ");
  //Serial.println(stepGoal);

  int coarseWindow = 500;
  int fineWindow = 60;

  setMotorSpeed(255); //Go!

  while (stepsRequired(stepGoal, steps) > coarseWindow) delayMicroseconds(10); //Spin until coarse window is closed

  //After we have gotten close to the first coarse window, proceed past the goal, then proceed to the goal
  if (addAFullRotation == true)
  {
    delay(500); //This should spin us past the goal
    //Now proceed to the goal
    while (stepsRequired(stepGoal, steps) > coarseWindow) delayMicroseconds(10); //Spin until coarse window is closed
  }

  setMotorSpeed(25); //Slowly approach
  while (stepsRequired(stepGoal, steps) > fineWindow) delayMicroseconds(10); //Spin until fine window is closed
  setMotorSpeed(0); //Stop

  delay(50); //Wait for motor to stop

  int delta = steps - stepGoal;
  Serial.print("delta: ");
  Serial.println(delta);

  //Serial.print("Arrived: ");
  //Serial.println(steps);

  return (delta);
}

//Calculate number of steps to get to our goal
//Based on current position, goal, and direction
//Corrects for 8400 roll over
int stepsRequired(int goal, int currentSteps)
{
  if (direction == CW)
  {
    if (currentSteps < goal) return (goal - currentSteps); //s=6, sg=8148, required = 8142
    else if (currentSteps >= goal) return (8400 - currentSteps + goal); //s=6, sg=1, required = 8395
  }
  else if (direction == CCW)
  {
    if (currentSteps < goal) return (8400 - goal + currentSteps); //s=600, sg=1000, required = 8000
    else if (currentSteps >= goal) return (currentSteps - goal); //s=6, sg=1, required = 5
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
  Serial.println();

  Serial.print("Want dialValue: ");
  Serial.println(dialValue);

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


//Given a dial value, covert to an encoder value (0 to 8400)
//If there are 100 numbers on the dial, each number is 84 ticks wide
int convertDialToEncoder(int dialValue)
{
  //Force dial value to the middle of three
  //byte remainder = dialValue / 3; // 14/3 = 4
  //dialValue = (remainder * 3) + 1; //4*3 + 1 = 13

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
void resetDial()
{
  //Turn CW, past zero, then return to zero
  turnCW();
  int dialValue = setDial(0, true); //Turn to zero with an extra spin

  Serial.print("resetDial ended at: ");
  Serial.println(dialValue);
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

  //TODO change as needed
  delay(300); //Wait for servo to move, but don't let it stall for too long and burn out

  //Check if we're there
  int handlePosition = analogRead(servoPosition);

  if (handlePosition > 500) //TODO change as needed
  {
    //Holy smokes we're there!
    handleServo.detach(); //Turn off servo
    return (true);
  }

  //Ok, we failed
  //Return to resting position
  handleServo.write(servoRestingPosition);

  return (false);
}

//Sets the motor speed
void setMotorSpeed(int speedValue)
{
  analogWrite(motorPWM, speedValue);
}

//Tell motor to turn clock wise (looking at shaft)
void turnCW()
{
  direction = CW;
  digitalWrite(motorDIR, LOW); //CW looking at shaft
}

//Tell motor to turn counter-clock wise (looking at shaft)
void turnCCW()
{
  direction = CCW;
  digitalWrite(motorDIR, HIGH); //CCW looking at shaft
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
  if (direction == CW) steps++;
  else steps--;
  if (steps < 0) steps = 8399; //Limit variable to zero
  if (steps > 8399) steps = 0; //Limit variable to 8399
}

//Interrupt routine when encoderB pin changes
void countB()
{
  if (direction == CW) steps++;
  else steps--;
  if (steps < 0) steps = 8399; //Limit variable to zero
  if (steps > 8399) steps = 0; //Limit variable to 8399
}

//Checks to see if we detect the magnet
boolean magnetDetected()
{
  if (digitalRead(reed) == LOW) return (true); //Magnet!
  return (false); //No magnet
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
