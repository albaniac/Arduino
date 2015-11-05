// Implements PIR to control a LED strip

#include <SPI.h>
#include <MySensor.h>  
#include <Wire.h> 

#define CHILD_ID_MOVE 1
#define CHILD_ID_DIMMER 10

#define DIGITAL_INPUT_SENSOR 3   // The digital input you attached your motion sensor.  (Only 2 and 3 generates interrupt!)
#define INTERRUPT DIGITAL_INPUT_SENSOR-2 // Usually the interrupt = pin -2 (on uno/nano anyway)

#define LED_PIN 6                    // DIGITAL PIN 6

static boolean tripped = false;
static boolean lastTripped = false;

unsigned long TIMER = 0;
unsigned long FADE_TIMER = 0;

#define FADE_UP_DELAY 30 // milliseconds between value changes (delay of 10 means one second from 1 to 100)
#define FADE_DOWN_DELAY 60

static int currentLedLevel = 5;  // Current dim level...
static int newLedLevel = 0;
static int minimumLedLevel = 0;
static int targetLevelOnMovement = 60;

unsigned long TEMPORARY_ON_TIMER = 0;
unsigned long TEMPORARY_ON_SLEEP_TIME = 60000;

MySensor gw;

MyMessage lightMsg(CHILD_ID_DIMMER, V_DIMMER);
MyMessage msgMove(CHILD_ID_MOVE, V_TRIPPED);

void setup()  
{ 
  gw.begin(incomingMessage, AUTO, true);

  pinMode(DIGITAL_INPUT_SENSOR, INPUT);      // sets the motion sensor digital pin as input

  // Send the Sketch Version Information to the Gateway
  gw.sendSketchInfo("Dimmer", "1.0");
  
  Serial.println("Present S_DIMMER");
  gw.present(CHILD_ID_DIMMER, S_DIMMER );
  Serial.println("Present S_MOTION");
  gw.present(CHILD_ID_MOVE, S_MOTION);

  // Set level to 0
 fadeToLevel(0);
}

void loop()      
{  
  gw.process();

  // Read digital motion value
  tripped = digitalRead(DIGITAL_INPUT_SENSOR) == HIGH; 

  if (tripped == true && minimumLedLevel > 0 && newLedLevel < targetLevelOnMovement)
  {
    Serial.println("Movement, Light On!");
    TEMPORARY_ON_TIMER = millis();
    fadeToLevel(targetLevelOnMovement);
  }

  if (tripped != lastTripped) {
     lastTripped = tripped;
     Serial.println("tripped");
     gw.send(msgMove.set(tripped?"1":"0"));  // Send tripped value to gw 
  }
  
  // If temporaryOn 
  if (millis() - TEMPORARY_ON_TIMER >= TEMPORARY_ON_SLEEP_TIME && TEMPORARY_ON_TIMER > 0)
  {
    TEMPORARY_ON_TIMER = 0;
    fadeToLevel(minimumLedLevel);
  }
  // Fade routine
  if (currentLedLevel < newLedLevel && (millis() - FADE_TIMER >= FADE_UP_DELAY))
  {
    currentLedLevel += 1;
    analogWrite( LED_PIN, (int)(currentLedLevel / 100. * 255) );
    FADE_TIMER = millis();
    Serial.print("LED up: ");      
    Serial.println(currentLedLevel);
  }
  if (currentLedLevel > newLedLevel && (millis() - FADE_TIMER >= FADE_DOWN_DELAY))
  {
    currentLedLevel -= 1;
    analogWrite( LED_PIN, (int)(currentLedLevel / 100. * 255) );
    FADE_TIMER = millis();
    Serial.print("LED down: ");      
    Serial.println(currentLedLevel);
  }
}

void incomingMessage(const MyMessage &message) {

Serial.println("incoming");
     
  if (message.type == V_LIGHT || message.type == V_DIMMER) {
    
    //  Retrieve the power or dim level from the incoming request message
    int requestedLevel = atoi( message.data );
    
    // Adjust incoming level if this is a V_LIGHT variable update [0 == off, 1 == on]
    requestedLevel *= ( message.type == V_LIGHT ? 100 : 1 );
    
    // Clip incoming level to valid range of 0 to 100
    requestedLevel = constrain(requestedLevel, 0, 100);

    minimumLedLevel = requestedLevel;
    
    Serial.print( "Changing level to " );
    Serial.print( requestedLevel );
    Serial.print( ", from " ); 
    Serial.println( currentLedLevel );

    fadeToLevel( requestedLevel );
    }
    
  
}

void fadeToLevel( int toLevel ) {
  // Inform the gateway of the current DimmableLED's SwitchPower1 and LoadLevelStatus value...
  gw.send(lightMsg.set(toLevel));
  newLedLevel = toLevel;
}



