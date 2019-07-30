// MyProjectThingControllable.ino
// Robocar controlled via speech commands sent via aRest from
// a web page. Ultrasonic sensor takes readings each loop and
// prevents car from crashing.

#include "motorFunctions.h"
#include "unphone.h"
#include "WiFi.h"
#include <aREST.h>
#include <Wire.h>
#include <Adafruit_MotorShield.h>
#include "esp_system.h"

const int wdtTimeout = 6000;  //time in ms to trigger the watchdog
hw_timer_t *timer = NULL;

// Watch Dog 
void IRAM_ATTR resetModule() {
  ets_printf("reboot\n");
  esp_restart();
}

// Create aREST instance
aREST rest = aREST();

// WiFi parameters
const char* ssid = "mattshotspot";
const char* password = "robocar55";

// The port to listen for incoming TCP connections 
#define LISTEN_PORT           80

// Create an instance of the server
WiFiServer server(LISTEN_PORT);

//Ultrasonic Sensor Variables
int counter = 0;
const int trigPin = 15;
const int echoPin = 32;
int distanceReadings [10] = { };
int average = 0;

// Reset I2C Bus
void recoverI2C();

void setup() {
  Wire.setClock(100000); // higher rates trigger an IOExpander bug
  UNPHONE_DBG = true;
  Serial.begin(115200);  // init the serial line

  recoverI2C();
  
  Serial.println();
  Serial.println("running setup");

  timer = timerBegin(0, 80, true);                  //timer 0, div 80
  timerAttachInterrupt(timer, &resetModule, true);  //attach callback
  timerAlarmWrite(timer, wdtTimeout * 1000, false); //set time in us
  timerAlarmEnable(timer);                          //enable interrupt

  // fire up I²C, and the unPhone's IOExpander library
  Wire.begin();
  IOExpander::begin();

  // Set Pins for ultrasonic sensor
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  //checkPowerSwitch(); // check if power switch is now off & if so shutdown

  // which board version are we running?
  int version = IOExpander::getVersionNumber(), spin;
  if(version == 7) spin = 4;
  Serial.printf("starting, running on spin %d\n", spin);

  
  i2s_config(); // configure the I2S bus
  
  // set up the SD card
  IOExpander::digitalWrite(IOExpander::SD_CS, LOW);
  if(!SD.begin(-1)) {
    D("Card Mount Failed");
    TestScreen::fail("SD CARD");
    delay(3000);
  }
  IOExpander::digitalWrite(IOExpander::SD_CS, HIGH);

  // Init motor shield
  AFMS.begin();  

  // Functions          
  rest.function("stop", halt);
  rest.function("forward", forward);
  rest.function("left", left);
  rest.function("right", right);
  rest.function("backward", backward);
      
  // Give name and ID to device
  rest.set_id("1");
  rest.set_name("robot");
  
  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
 
  // Start the server
  server.begin();
  Serial.println("Server started");
  
  // Print the IP address
  Serial.println(WiFi.localIP());

}

void loop() {
  bool usbPowerOn = checkPowerSwitch(); // shutdown if switch off

  //Serial.println("running main loop");

  timerWrite(timer, 0); //reset timer (feed watchdog)
  long loopTime = millis();
  
  counter++; // Increment loop counter by one
  average = 0; // Reset average
  
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  //delayMicroseconds(1000); //- Removed this line
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  long duration = pulseIn(echoPin, HIGH);
  long distance = (duration/2) / 29.1;
  //Serial.print(distance);
  //Serial.println(" cm");

  // Print Last 10 Readings
  Serial.print("[");
  for(int i = 0; i < 10; i++){
    Serial.print(distanceReadings[i]);
    Serial.print(" ,");
  }
  Serial.print("]");
  
  if(counter < 10){
    // Shift all values in the array to the left
    for(int i = 0; i < 9; i++){
      distanceReadings[i] = distanceReadings[i+1];
    }
    if(distance > 500){
      distanceReadings[9] = 0;
    }
    else{
      distanceReadings[9] = distance;
    }
  }
  else{
    // Calculate average of last 10 readings
    int count;
    for(int i = 0; i < 10; i++ ){
      average += distanceReadings[i];
    }
    
    average = average / 10;
    Serial.print("Average: ");
    Serial.println(average);

    // Check to see if new reading is reasonable
    if(distance <= 2 || distance > 400){
      return;
    }
    else if(distance >= 3 * average || distance <= average / 3){
      return;
    }
    else{
      // Shift all values in the array to the left
      for(int i = 0; i < 9; i++){
        distanceReadings[i] = distanceReadings[i+1];
      }
  
      distanceReadings[9] = distance;
    }
  }
  // Stop robot if close to a wall
  if(average <= 9){
    stopRobot();
  }

  loopTime = millis() - loopTime; // Calculate Loop time
  
  Serial.print("loop time is = ");
  Serial.println(loopTime); //should be under 3000

  // Handle REST calls
  WiFiClient client = server.available();
  if (!client) {
    return;
  }
  while(!client.available()){
    delay(1);
  }
  rest.handle(client);
}

void recoverI2C() {   // try to recover I2C bus in case it's locked up...
  pinMode(SCL, OUTPUT);
  pinMode(SDA, OUTPUT);
  digitalWrite(SDA, HIGH);

  for(int i = 0; i < 10; i++) { // 9th cycle acts as NACK
    digitalWrite(SCL, HIGH);
    delayMicroseconds(5);
    digitalWrite(SCL, LOW);
    delayMicroseconds(5);
  }

  // a STOP signal (SDA from low to high while SCL is high)
  digitalWrite(SDA, LOW);
  delayMicroseconds(5);
  digitalWrite(SCL, HIGH);
  delayMicroseconds(2);
  digitalWrite(SDA, HIGH);
  delayMicroseconds(2);

  // I2C bus should be free now... a short delay to help things settle
  delay(200);
}
