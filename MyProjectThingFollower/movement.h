#include <Adafruit_MotorShield.h>

// Create the motor shield object with the default I2C address
Adafruit_MotorShield AFMS = Adafruit_MotorShield();
 
// And connect 2 DC motors to port M3 & M4 !
Adafruit_DCMotor *R_MOTOR = AFMS.getMotor(3);
Adafruit_DCMotor *L_MOTOR = AFMS.getMotor(4);

//for carpet:
//const int floorSpeed = 1450;
//for hard floor:
const int floorSpeed = 900;

//stop motors
void halt() {
  
  // Stop
  L_MOTOR->setSpeed(0);
  L_MOTOR->run( RELEASE );
 
  R_MOTOR->setSpeed(0);
  R_MOTOR->run( RELEASE );
  
}

//move robot forward
void forward() {
  
  L_MOTOR->setSpeed(150);
  L_MOTOR->run( BACKWARD );
 
  R_MOTOR->setSpeed(150);
  R_MOTOR->run( BACKWARD );
  
}

//move robot backwards
void backward() {
  
  // Stop
  L_MOTOR->setSpeed(150);
  L_MOTOR->run( FORWARD );
 
  R_MOTOR->setSpeed(150);
  R_MOTOR->run( FORWARD );
  Serial.println("back");
}

//turn robot 90 degrees clockwise
void turn90Clockwise(){
  // Stop
  L_MOTOR->setSpeed(50);
  L_MOTOR->run( BACKWARD );
 
  R_MOTOR->setSpeed(50);
  R_MOTOR->run( FORWARD );
  delay(floorSpeed);
  //Serial.println("90 Clockwise");
}

//turn robot 180 degrees clockwise
void turn180Clockwise(){
  // Stop
  L_MOTOR->setSpeed(50);
  L_MOTOR->run( BACKWARD );
 
  R_MOTOR->setSpeed(50);
  R_MOTOR->run( FORWARD );
  delay(floorSpeed);
  delay(floorSpeed);
  //Serial.println("90 Clockwise");
}

//turn robot 90 degrees anti-clockwise
void turn90AntiClockwise(){
  // Stop
  L_MOTOR->setSpeed(50);
  L_MOTOR->run( FORWARD );
 
  R_MOTOR->setSpeed(50);
  R_MOTOR->run( BACKWARD );
  delay(floorSpeed);

}

