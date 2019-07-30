#include <Adafruit_MotorShield.h>

// Create the motor shield object with the default I2C address
Adafruit_MotorShield AFMS = Adafruit_MotorShield();

// And connect 2 DC motors to port M3 & M4 !
Adafruit_DCMotor *L_MOTOR = AFMS.getMotor(3);
Adafruit_DCMotor *R_MOTOR = AFMS.getMotor(4);

// Function
int halt(String message);
int forward(String message);
int right(String message);
int left(String message);
int backward(String message);
void stopRobot();


int halt(String command) {
  
  // Stop
  L_MOTOR->setSpeed(0);
  L_MOTOR->run( FORWARD );
 
  R_MOTOR->setSpeed(0);
  R_MOTOR->run( FORWARD );
  
}

int forward(String command) {
  
  // Stop
  L_MOTOR->setSpeed(200);
  L_MOTOR->run( BACKWARD );
 
  R_MOTOR->setSpeed(200);
  R_MOTOR->run( BACKWARD );
  
}

int left(String command) {
  
  // Stop
  L_MOTOR->setSpeed(50);
  L_MOTOR->run( FORWARD );
 
  R_MOTOR->setSpeed(50);
  R_MOTOR->run( BACKWARD );

  delay(1127);
  stopRobot();
  
}

int right(String command) {
  
  // Stop
  L_MOTOR->setSpeed(50);
  L_MOTOR->run( BACKWARD );
 
  R_MOTOR->setSpeed(50);
  R_MOTOR->run( FORWARD );

  delay(1127);
  stopRobot();
  
}

int backward(String command) {
  
  // Stop
  L_MOTOR->setSpeed(150);
  L_MOTOR->run( FORWARD );
 
  R_MOTOR->setSpeed(150);
  R_MOTOR->run( FORWARD );
  
}

void stopRobot(){
  
  // Stop
  L_MOTOR->setSpeed(0);
  L_MOTOR->run( FORWARD );
 
  R_MOTOR->setSpeed(0);
  R_MOTOR->run( FORWARD );
  
}
