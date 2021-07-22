/*

 * Project Title : Switchable-light-system-Between-Alexa-and-three-way-switch
 *  Description: This code allows to switching a lamp on and off in a switchable way using a 
    manual three-Way regular switch in addition to a relay module controlled by Alexa 
    using SinricPro IoT Cloud Platform. Part of the source example"Switch.ino" by SinricPro 
    was modified, the original example can be found at: https://github.com/sinricpro/esp8266-esp32-sdk/tree/master/examples.
 *  Project Author: Norberto Moreno Vélez.
 *  17/03/2021
  */



#ifdef ENABLE_DEBUG
       #define DEBUG_ESP_PORT Serial
       #define NODEBUG_WEBSOCKETS
       #define NDEBUG
#endif 

#include <Arduino.h>
#ifdef ESP8266 
       #include <ESP8266WiFi.h>
#endif 
#ifdef ESP32   
       #include <WiFi.h>
#endif

#include "SinricPro.h"
#include "SinricProSwitch.h"
#include <secrets.h>

#define WIFI_SSID         SECRET_SSID    
#define WIFI_PASS         SECRET_PASS
#define APP_KEY           "xxxx"      // Should look like "de0bxxxx-1x3x-4x3x-ax2x-5dabxxxxxxxx"
#define APP_SECRET        "xxxx"   // Should look like "5f36xxxx-x3x7-4x3x-xexe-e86724a9xxxx-4c4axxxx-3x3x-x5xe-x9x3-333d65xxxxxx"
#define SWITCH_ID         "xxxx"    // Should look like "5dc1564130xxxxxxxxxxxxxx"
#define BAUD_RATE         115200   // Change baudrate to your need

#define RELAY_PIN         D5                  // Pin where the relay is connected (D5 = GPIO 14 on ESP8266)

#define BUTTON_PIN 0   // GPIO for BUTTON (inverted: LOW = pressed, HIGH = released) D3
#define LED_PIN   2   // GPIO for LED (inverted) D4

bool myPowerState = false;
unsigned long lastBtnPress = 0;

/* bool onPowerState(String deviceId, bool &state) 
 *
 * Callback for setPowerState request
 * parameters
 *  String deviceId (r)
 *    contains deviceId (useful if this callback used by multiple devices)
 *  bool &state (r/w)
 *    contains the requested state (true:on / false:off)
 *    must return the new state
 * 
 * return
 *  true if request should be marked as handled correctly / false if not
 */
bool onPowerState(const String &deviceId, bool &state) {  // Request function
  Serial.printf("Device %s turned %s (via SinricPro) \r\n", deviceId.c_str(), state?"on":"off");
  myPowerState = state;
  Serial.println(myPowerState);
  digitalWrite(LED_PIN, myPowerState?LOW:HIGH);

  // Logic conditionals
  bool alexaRelayState = digitalRead(RELAY_PIN);
  if (myPowerState != alexaRelayState) { // If the requested state and the current state of the RELAY_PIN are different
    Serial.print("Whe are here, when the request and the relay state are: ");
    if (myPowerState) {                 // If the requested state is true 
      digitalWrite(RELAY_PIN, HIGH);    // The HIGH value deactivate the relay (If using a active HIGH relay change for digitalWrite(RELAY_PIN, LOW);  this logic solves the following situation: || Alexa request=1 | manual switch=1 | RELAY_PIN=1 | light relay=0 ||
      Serial.println("requested value is true | the relay state value was false!");
    } 
    else if (!myPowerState) {           // // If the requested state is false
      digitalWrite(RELAY_PIN, LOW);    // The LOW value activate the relay (If using a active HIGH relay change for digitalWrite(RELAY_PIN, HIGH);  this logic solves the following situation: || Alexa request=0 | manual switch=1 | RELAY_PIN=0 | light relay=1 ||
      Serial.println("requested value is false | the relay state value was true!");
    }
  }

  else if (myPowerState == alexaRelayState) { // When the requested state and the current state of the RELAY_PIN are equals 
    if (!myPowerState) {     // If the request is false then this statement become true( For active high relay mode is used change to true)
      Serial.print("The request was: ");
      Serial.println(myPowerState);
      bool poww = !myPowerState;
      Serial.print("The NOT logical convert resoults in: ");
      Serial.println(poww);
      digitalWrite(RELAY_PIN, HIGH);    
    } else { 
      Serial.print("The request was: ");
      Serial.println(myPowerState);
      bool poww = !myPowerState;
      Serial.print("The NOT logical convert resoults in: ");
      Serial.println(poww);
      digitalWrite(RELAY_PIN, LOW); 
      }
  }
  return true; // request handled properly
}

void handleButtonPress() {
  unsigned long actualMillis = millis(); // get actual millis() and keep it in variable actualMillis
  if (digitalRead(BUTTON_PIN)== LOW && actualMillis - lastBtnPress > 1000)  { // is button pressed (inverted logic! button pressed = LOW) and debounced?
    if (myPowerState) {     // flip myPowerState: if it was true, set it to false, vice versa
      myPowerState = false;
      bool switchRead = digitalRead(BUTTON_PIN);
      
    } else {
      myPowerState = true;
      bool switchRead = digitalRead(BUTTON_PIN);
      Serial.print("The switch state is: ");
      Serial.println(switchRead);
    }
    digitalWrite(LED_PIN, myPowerState?LOW:HIGH); // if myPowerState indicates device turned on: turn on led (builtin led uses inverted logic: LOW = LED ON / HIGH = LED OFF)

    // get Switch device back
    SinricProSwitch& mySwitch = SinricPro[SWITCH_ID];
    // send powerstate event
    mySwitch.sendPowerStateEvent(myPowerState); // send the new powerState to SinricPro server
    Serial.printf("Device %s turned %s (manually via flashbutton)\r\n", mySwitch.getDeviceId().toString().c_str(), myPowerState?"on":"off");

    //lastBtnPress = actualMillis;  // update last button press variable
  } 
  delay(500);
}

// setup function for WiFi connection
void setupWiFi() {
  Serial.printf("\r\n[Wifi]: Connecting");
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.printf(".");
    delay(250);
  }
  Serial.printf("connected!\r\n[WiFi]: IP-Address is %s\r\n", WiFi.localIP().toString().c_str());
}

// setup function for SinricPro
void setupSinricPro() {
  // add device to SinricPro
  SinricProSwitch& mySwitch = SinricPro[SWITCH_ID];

  // set callback function to device
  mySwitch.onPowerState(onPowerState);

  // setup SinricPro
  SinricPro.onConnected([](){ Serial.printf("Connected to SinricPro\r\n"); }); 
  SinricPro.onDisconnected([](){ Serial.printf("Disconnected from SinricPro\r\n"); });
  SinricPro.begin(APP_KEY, APP_SECRET);
}

// main setup function
void setup() {
  pinMode(BUTTON_PIN, INPUT_PULLUP); // GPIO 0 as input, pulled high
  pinMode(LED_PIN, OUTPUT); // define LED GPIO as output
  digitalWrite(LED_PIN, HIGH); // turn off LED on bootup
  pinMode(RELAY_PIN, OUTPUT);                 // set relay-pin to output mode
  digitalWrite(RELAY_PIN, HIGH);

  Serial.begin(BAUD_RATE); Serial.printf("\r\n\r\n");
  setupWiFi();
  setupSinricPro();
}

void loop() {
  handleButtonPress();
  SinricPro.handle();
}
