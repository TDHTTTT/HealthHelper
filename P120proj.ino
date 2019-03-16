#include <TimeLib.h>
#include <ArduinoJson.h>

#define TIME_HEADER  "T"   // Header tag for serial time sync message
#define TIME_REQUEST  7    // ASCII bell character requests a time sync message 

int fsrAnalogPin = 0; // FSR is connected to analog 0
int LEDpin = 11; // connect Yellow LED to pin 11 (PWM pin)
int IDLEPin = 12;
int WARNPin = 13;
int motorPin = 3;
int fsrReading; // the analog reading from the FSR resistor divider
int LEDbrightness;
int state = 0;
String startTime;
int starTime;
int currTime;

void processSyncMessage() {
  unsigned long pctime;
  const unsigned long DEFAULT_TIME = 1357041600; // Jan 1 2013

  if(Serial.find(TIME_HEADER)) {
     pctime = Serial.parseInt();
     if( pctime >= DEFAULT_TIME) { // check the integer is a valid time (greater than Jan 1 2013)
       setTime(pctime); // Sync Arduino clock to the time received on the serial port
     }
  }
}

time_t requestSync()
{
  Serial.write(TIME_REQUEST);  
  return 0; // the time will be sent later in response to serial mesg
}



void setup()  {
  Serial.begin(9600);
  while (!Serial) ; // Needed for Leonardo only
  
  pinMode(13, OUTPUT);
  setSyncProvider( requestSync);  //set function to call when sync required
  Serial.println("Waiting for sync message");
}

void loop(){
   // sync time pi->arduino (date +T%s\n > /dev/ttyACM0)
  if (Serial.available()) {
    processSyncMessage();
  }
  if (timeStatus()!= timeNotSet) {
   Serial.print(String(year())+'-'+String(month())+'-'+String(day())+'-'+ String(hour())+'-'+String(minute())+'-'+String(second()));
  }
  DynamicJsonDocument  doc(200);
  doc["loc"] = "chair";

  fsrReading = analogRead(fsrAnalogPin);
//  Serial.print("Analog reading = ");
//  Serial.println(fsrReading);

  switch(state)
  {
    case 0:
      digitalWrite(IDLEPin, HIGH);
      digitalWrite(WARNPin, LOW);
      analogWrite(LEDpin, LOW);
      Serial.println("In idle state!");
      //Serial.println("\n");
      if (fsrReading > 200) {
        Serial.println("Someone sits on! Transition to busy state!");
        digitalWrite(IDLEPin, LOW);
        // reading (0-1023) -> write (0-255)
        LEDbrightness = map(fsrReading, 0, 1023, 0, 255);
        analogWrite(LEDpin, LEDbrightness);
        // time is in GMT
        startTime = String(year())+'-'+String(month())+'-'+String(day())+'-'+ String(hour())+'-'+String(minute())+'-'+String(second());
        starTime = hour()*3600 + minute()*60 + second();
//        Serial.println("num");
//        Serial.println(starTime); 
        state = 1;
      }
      else {
        state = 0;
      }
      break;
      
    case 1:
      digitalWrite(IDLEPin, LOW);
      digitalWrite(WARNPin, LOW);
      Serial.println("In busy state!");
      //Serial.println("\n");
      if (fsrReading < 10) {
        analogWrite(LEDpin, LOW);
        Serial.println("Someone leaves! Transition to idle state!");
        doc["start"] = startTime;
        doc["stop"] = String(year())+'-'+String(month())+'-'+String(day())+'-'+ String(hour())+'-'+String(minute())+'-'+String(second());        
        serializeJson(doc, Serial);
        Serial.println("\n");
        state = 0;
      }
      else {
        LEDbrightness = map(fsrReading, 0, 1023, 0, 255);
        analogWrite(LEDpin, LEDbrightness);
        currTime = hour()*3600 + minute()*60 + second();
        if (currTime - starTime > 5) {
          Serial.println("Someone sits for too long! Transition to warning state!");
          state = 2;
        }
        else {
          state = 1;
        } 
      }
      break;
      
    case 2:
      digitalWrite(IDLEPin, LOW);
      digitalWrite(WARNPin, HIGH);
      Serial.println("In warning state!");
     // Serial.println("\n");
      if (fsrReading < 10) {
        analogWrite(LEDpin, LOW);
        Serial.println("Someone leaves from warning! Transition to idle state!");
        doc["start"] = startTime;
        doc["stop"] = String(year())+'-'+String(month())+'-'+String(day())+'-'+ String(hour())+'-'+String(minute())+'-'+String(second());        
        serializeJson(doc, Serial);
        Serial.println("\n");
        state = 0;
      }
      else {
        LEDbrightness = map(fsrReading, 0, 1023, 0, 255);
        analogWrite(LEDpin, LEDbrightness);
        state = 2;
      }
      break;
      
    case 3:
      digitalWrite(IDLEPin, LOW);
      analogWrite(LEDpin, LOW);
      digitalWrite(WARNPin, LOW);      
      Serial.println("In walking state!");
      
      
      
  }

//  JsonArray data = doc.createNestedArray("data");
//  data.add(48.756080);  
//  data.add(2.302038);
  
  delay(100);
}
