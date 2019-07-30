// MyProjectThing.ino
// COM3505 2018 project template sketch. Do your project code here.
// Out of the box the sketch is configured to kick the tyres on all the
// modules, and allow stepping through tests via the touch screen. Change the 
// TestScreen::activate(true); to false to change this behaviour.

#include "movement.h"
#include "asyncwebserver.h"
#include "unphone.h"
#include "WiFi.h"
#include <Wire.h>
#include <Adafruit_MotorShield.h>
#include <WebServer.h> // simple webserver
#include <HTTPClient.h>   // ESP32 library for making HTTP requests
#include <Update.h>       // OTA update library
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

// Web Client Stuff /////////////////////////////////////////////////////////
const char* myEmail       = "mthorton1@sheffield.ac.uk";
const char *com3505Addr    = "com3505.gate.ac.uk";
const int   com3505Port    = 9191;
const char* guestSsid      = "leonshotspot";
const char* guestPassword  = "robocar55";

void setup() {
  Wire.setClock(100000); // higher rates trigger an IOExpander bug
  UNPHONE_DBG = true;
  Serial.begin(115200);  // init the serial line
  getMAC(MAC_ADDRESS);          // store the MAC address
  Serial.printf("\nMyOTAThing setup...\nESP32 MAC = %s\n", MAC_ADDRESS);

  // fire up I²C, and the unPhone's IOExpander library
  Wire.begin();
  IOExpander::begin();

  checkPowerSwitch(); // check if power switch is now off & if so shutdown

  // which board version are we running?
  int version = IOExpander::getVersionNumber(), spin;
  if(version == 7) spin = 4;
  Serial.printf("starting, running on spin %d\n", spin);

  IOExpander::digitalWrite(IOExpander::BACKLIGHT, LOW);
  tft.begin(HX8357D);
  TestScreen::activate(true);
  TestScreen::init();
  IOExpander::digitalWrite(IOExpander::BACKLIGHT, HIGH);
   
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
  
   // conect to the uni-other network
  dbg(netDBG, "connecting to ");
  dln(netDBG, guestSsid);
  WiFi.begin(guestSsid, guestPassword);
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    dbg(netDBG, ".");
  }

  //print the Ip address of the connection
 dbg(netDBG, "wifi connected; IP address: ");
 dln(netDBG, WiFi.localIP());
  
 // Print the IP address
 Serial.println(WiFi.localIP());

 startAP();            // fire up the AP...
 initWebServer();      // ...and the web server

}

void loop() {
  checkPowerSwitch(); // check if the power switch is now off & if so shutdown
  
  //initialise variable and array to hold rssi reaings
  long newRSSI;
  long RSSIreadings[4] = {0};

  //get the current RSSI reading
  currentRSSI = scanForNetworks(ssidFollowing);
  Serial.println(currentRSSI);
  
  //assume -45 is a good connection, if good connection do not move
  if (currentRSSI >= -45) {
    halt();
  }
  else { 

    //turn 4 times and record rssi of 4 routes
      for(int a = 1; a <= 4; a++ ){
       //logic to turn 90 degrees and move forward
        turn90Clockwise();
        halt();
        forward();
        delay(3500);
        halt();
       //take rssi reading and store in array 
       
        int locationReadings [3] = { };
        int average = 0;
        for(int i = 0; i < 3; i++){
          locationReadings[i] = scanForNetworks(ssidFollowing);
        }

        int totalArrayReadings = 0;
        for(int i = 0; i < 3; i++){
          totalArrayReadings += locationReadings[i];
        }
        average = totalArrayReadings / 3;
        RSSIreadings[a-1] = average;
        //return to original position
        backward();
        delay(3500);
        halt();
        Serial.println(a);
      }
    delay(300);

    int location, maximum;
    
    //get highest RSSI reading and its array position
    int n = sizeof(RSSIreadings)/sizeof(RSSIreadings[0]);
    location = find_maximum(RSSIreadings, n);
    maximum  = RSSIreadings[location];
    //output the highest RSSI and the amount of turns taken to get it for debugging
    Serial.print("Maximum RSSI is ");
    Serial.println(maximum);
    Serial.print("Turn position is ");
    Serial.println(location);
    
    //turn back into position of when highest rssi was found
    for( int b = location; b >= 0; b-- ){
      if (b==3) {
        break;
      }
      turn90Clockwise();
      halt();
      Serial.println(b);
    }
    
    //move forward to position of when highest rssi found
    forward();
    delay(5000);
    halt();

    delay(300);
  }
}
 
//method to find the index of the highest value in an array
long find_maximum(long a[], int n) {
  int c, max, index;
 
  max = a[0];
  index = 0;
 
  for (c = 1; c < n; c++) {
    if (a[c] > max) {
       index = c;
       max = a[c];
    }
  }
 
  return index;
} 

//returns the RSSI value of the network ssid passed in
long scanForNetworks(String apSSID) {
    //Serial.println("scan start");
    // WiFi.scanNetworks will return the number of networks found
    int n = WiFi.scanNetworks();
    //Serial.println("scan done");
    long updatedRSSI = 0;
    if (n == 0) {
        //Serial.println("no networks found");
        return updatedRSSI;
    } else {
        //Serial.print(n);
        Serial.println(" networks found");
        for (int i = 0; i < 9; ++i) {
            // set the rssi to the scanned ssid rssi
            if(WiFi.SSID(i) == apSSID){
              updatedRSSI = WiFi.RSSI(i);
              Serial.println(updatedRSSI);
            }
        }
    }

    return updatedRSSI;
}

/////////////////////////////////////////////////
/////////////////////////////////////////////////
/////  ACESS POINT CODE BELOW HERE //////////////
/////////////////////////////////////////////////
/////////////////////////////////////////////////

void initWebServer() {
  // register callbacks to handle different paths


  webServer.on("/", hndlRoot);              // slash
  webServer.onNotFound(hndlNotFound);       // 404s...
  webServer.on("/generate_204", hndlRoot);  // Android captive portal support
  webServer.on("/L0", hndlRoot);            // TODO is this...
  webServer.on("/L2", hndlRoot);            // ...IoS captive portal...
  webServer.on("/ALL", hndlRoot);           // ...stuff?
  webServer.on("/wifi", hndlWifi);          // page for choosing an AP
  webServer.on("/wifichz", hndlWifichz);    // landing page for AP form submit
  webServer.on("/status", hndlStatus);      // status check, e.g. IP address

  webServer.begin();

  dln(startupDBG, "HTTP server started");
}

/////  Methods to handle Paths  /////////////////////////////////////////////

// webserver handler callbacks
void hndlNotFound(AsyncWebServerRequest *request) {
  request->send(200, "text/plain", "URI Not Found");
}

void hndlRoot(AsyncWebServerRequest *request) {
  dln(netDBG, "serving page notionally at /");
  replacement_t repls[] = { // the elements to replace in the boilerplate
    {  1, apSSID.c_str() },
    {  8, "" },
    {  9, "<p>Choose a <a href=\"wifi\">wifi access point</a>.</p>" },
    { 10, "<p>Check <a href='/status'>wifi status</a>.</p>" },
  };
  String htmlPage = ""; // a String to hold the resultant page
  GET_HTML(htmlPage, templatePage, repls); // GET_HTML sneakily added to Ex07
  request->send(200, "text/html", htmlPage);
}

void hndlWifi(AsyncWebServerRequest *request) {
  dln(netDBG, "serving page at /wifi");

  String form = ""; // a form for choosing an access point and entering key
  apListForm(form);
  replacement_t repls[] = { // the elements to replace in the boilerplate
    { 1, apSSID.c_str() },
    { 7, "<h2>Network configuration</h2>\n" },
    { 8, "" },
    { 9, form.c_str() },
  };
  String htmlPage = ""; // a String to hold the resultant page
  GET_HTML(htmlPage, templatePage, repls); // GET_HTML sneakily added to Ex07

  request->send(200, "text/html", htmlPage);
}

void hndlWifichz(AsyncWebServerRequest *request) {
  dln(netDBG, "serving page at /wifichz");

  String assid = "";
  String key = "";

  for(uint8_t i = 0; i < request->params(); i++ ) {
    AsyncWebParameter* p = request->getParam(i);
    if(p->name() == "ssid") {
      assid = p->value();
      ssidFollowing = p->value();
      Serial.println("Changed to ssid: " + ssidFollowing);
    }
  }

  String title = "<h2>Now Following " + ssidFollowing + "</h2>";
  String message = "<p>Check <a href='/status'>wifi status</a>.</p>";

  if(assid == "") {
    message = "<h2>Ooops, no SSID...?</h2>\n<p>Looks like a bug :-(</p>";
  } else {
    char ssidchars[assid.length()+1];
    char keychars[key.length()+1];
    assid.toCharArray(ssidchars, assid.length()+1);
  }

  replacement_t repls[] = { // the elements to replace in the template
    { 1, apSSID.c_str() },
    { 7, title.c_str() },
    { 8, "" },
    { 9, message.c_str() },
  };
  String htmlPage = "";     // a String to hold the resultant page
  GET_HTML(htmlPage, templatePage, repls);

  request->send(200, "text/html", htmlPage);
}

void hndlStatus(AsyncWebServerRequest *request) {         // UI for checking connectivity etc.
  dln(netDBG, "serving page at /status");

  String s = "";
  s += "<ul>\n";
  s += "\n<li>SSID: ";
  s += WiFi.SSID();
  s += "</li>";
  s += "\n<li>Status: ";
  switch(WiFi.status()) {
    case WL_IDLE_STATUS:
      s += "WL_IDLE_STATUS</li>"; break;
    case WL_NO_SSID_AVAIL:
      s += "WL_NO_SSID_AVAIL</li>"; break;
    case WL_SCAN_COMPLETED:
      s += "WL_SCAN_COMPLETED</li>"; break;
    case WL_CONNECTED:
      s += "WL_CONNECTED</li>"; break;
    case WL_CONNECT_FAILED:
      s += "WL_CONNECT_FAILED</li>"; break;
    case WL_CONNECTION_LOST:
      s += "WL_CONNECTION_LOST</li>"; break;
    case WL_DISCONNECTED:
      s += "WL_DISCONNECTED</li>"; break;
    default:
      s += "unknown</li>";
  }

  s += "\n<li>Local IP: ";     s += ip2str(WiFi.localIP());
  s += "</li>\n";
  s += "\n<li>Soft AP IP: ";   s += ip2str(WiFi.softAPIP());
  s += "</li>\n";
  s += "\n<li>AP SSID name: "; s += apSSID;
  s += "</li>\n";

  s += "</ul></p>";

  replacement_t repls[] = { // the elements to replace in the boilerplate
    { 1, apSSID.c_str() },
    { 7, "<h2>Status</h2>\n" },
    { 8, "" },
    { 9, s.c_str() },
  };
  String htmlPage = ""; // a String to hold the resultant page
  GET_HTML(htmlPage, templatePage, repls); // GET_HTML sneakily added to Ex07

  request->send(200, "text/html", htmlPage);
}

void apListForm(String& f) { // utility to create a form for choosing AP
  const char *checked = " checked";
  int n = WiFi.scanNetworks();
  dbg(netDBG, "scan done: ");

  if(n == 0) {
    dln(netDBG, "no networks found");
    f += "No wifi access points found :-( ";
    f += "<a href='/'>Back</a><br/><a href='/wifi'>Try again?</a></p>\n";
  } else {
    dbg(netDBG, n); dln(netDBG, " networks found");
    f += "<p>Wifi access points available:</p>\n"
         "<p><form method='POST' action='wifichz'> ";
    for(int i = 0; i < n; ++i) {
      f.concat("<input type='radio' name='ssid' value='");
      f.concat(WiFi.SSID(i));
      f.concat("'");
      f.concat(checked);
      f.concat(">");
      f.concat(WiFi.SSID(i));
      f.concat(" (");
      f.concat(WiFi.RSSI(i));
      f.concat(" dBm)");
      f.concat("<br/>\n");
      checked = "";
    }
    f += "<input type='submit' value='Submit'></form></p>";
  }
}
