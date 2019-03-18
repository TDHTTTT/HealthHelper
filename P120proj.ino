#include <TimeLib.h>
#include <ArduinoJson.h>
#include <Wire.h> // This library allows you to communicate with I2C devices.

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
int steps;

const int MPU_ADDR = 0x68; // I2C address of the MPU-6050. If AD0 pin is set to HIGH, the I2C address will be 0x69.

int16_t accelerometer_x, accelerometer_y, accelerometer_z; // variables for accelerometer raw data
int16_t gyro_x, gyro_y, gyro_z; // variables for gyro raw data
int16_t temperature; // variables for temperature data

char tmp_str[7]; // temporary variable used in convert function

char* convert_int16_to_str(int16_t i) { // converts int16 to string. Moreover, resulting strings will have the same length in the debug monitor.
  sprintf(tmp_str, "%6d", i);
  return tmp_str;
}

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

// extract determining sit/move as function

void setup()  {
  Serial.begin(9600);
  while (!Serial) ; // Needed for Leonardo only

  pinMode(LEDpin, OUTPUT);
  pinMode(IDLEPin, OUTPUT);
  pinMode(WARNPin, OUTPUT);
  pinMode(motorPin, OUTPUT);

  setSyncProvider( requestSync);  //set function to call when sync required
  Serial.println("Waiting for sync message");

  Wire.begin();
  Wire.beginTransmission(MPU_ADDR); // Begins a transmission to the I2C slave (GY-521 board)
  Wire.write(0x6B); // PWR_MGMT_1 register
  Wire.write(0); // set to zero (wakes up the MPU-6050)
  Wire.endTransmission(true);
}

void loop(){
  // sync time pi->arduino (date +T%s\n > /dev/ttyACM0)
  if (Serial.available()) {
    processSyncMessage();
  }
  if (timeStatus()!= timeNotSet) {
    Serial.print(String(year())+'-'+String(month())+'-'+String(day())+'-'+ String(hour())+'-'+String(minute())+'-'+String(second())+": ");
  }
  DynamicJsonDocument  doc(200);

  fsrReading = analogRead(fsrAnalogPin);
  //  Serial.print("Analog reading = ");
  //  Serial.println(fsrReading);
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B); // starting with register 0x3B (ACCEL_XOUT_H) [MPU-6000 and MPU-6050 Register Map and Descriptions Revision 4.2, p.40]
  Wire.endTransmission(false); // the parameter indicates that the Arduino will send a restart. As a result, the connection is kept active.
  Wire.requestFrom(MPU_ADDR, 7*2, true); // request a total of 7*2=14 registers

  // "Wire.read()<<8 | Wire.read();" means two registers are read and stored in the same variable
  accelerometer_x = Wire.read()<<8 | Wire.read(); // reading registers: 0x3B (ACCEL_XOUT_H) and 0x3C (ACCEL_XOUT_L)
  accelerometer_y = Wire.read()<<8 | Wire.read(); // reading registers: 0x3D (ACCEL_YOUT_H) and 0x3E (ACCEL_YOUT_L)
  accelerometer_z = Wire.read()<<8 | Wire.read(); // reading registers: 0x3F (ACCEL_ZOUT_H) and 0x40 (ACCEL_ZOUT_L)
  temperature = Wire.read()<<8 | Wire.read(); // reading registers: 0x41 (TEMP_OUT_H) and 0x42 (TEMP_OUT_L)
  gyro_x = Wire.read()<<8 | Wire.read(); // reading registers: 0x43 (GYRO_XOUT_H) and 0x44 (GYRO_XOUT_L)
  gyro_y = Wire.read()<<8 | Wire.read(); // reading registers: 0x45 (GYRO_YOUT_H) and 0x46 (GYRO_YOUT_L)
  gyro_z = Wire.read()<<8 | Wire.read(); // reading registers: 0x47 (GYRO_ZOUT_H) and 0x48 (GYRO_ZOUT_L)
  Serial.print("aX = "); Serial.print(convert_int16_to_str(accelerometer_x));
  Serial.print(" | aY = "); Serial.print(convert_int16_to_str(accelerometer_y));
  Serial.print(" | aZ = "); Serial.print(convert_int16_to_str(accelerometer_z));
  // the following equation was taken from the documentation [MPU-6000/MPU-6050 Register Map and Description, p.30]
  Serial.print(" | tmp = "); Serial.print(temperature/340.00+36.53);
  Serial.print(" | gX = "); Serial.print(convert_int16_to_str(gyro_x));
  Serial.print(" | gY = "); Serial.print(convert_int16_to_str(gyro_y));
  Serial.print(" | gZ = "); Serial.print(convert_int16_to_str(gyro_z));
  Serial.println();
  switch(state)
  {
  case 0:
    digitalWrite(IDLEPin, HIGH);
    digitalWrite(WARNPin, LOW);
    analogWrite(LEDpin, LOW);
    analogWrite(motorPin, 0);
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
    else if (accelerometer_y<-10000 && accelerometer_x<5000) {
      state = 3;
    }
    else {
      state = 0;
    }
    break;

  case 1:
    digitalWrite(IDLEPin, LOW);
    digitalWrite(WARNPin, LOW);
    analogWrite(motorPin, 0);
    Serial.println("In busy state!");
    //Serial.println("\n");
    if (fsrReading < 10) {
      //analogWrite(LEDpin, LOW);
      Serial.println("Someone leaves! Transition to idle state!");
      doc["activity"] = "Sitting";
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
    analogWrite(motorPin, 0);
    Serial.println("In warning state!");
    // Serial.println("\n");
    if (fsrReading < 10) {
      //analogWrite(LEDpin, LOW);
      Serial.println("Someone leaves from warning! Transition to idle state!");
      doc["activity"] = "Sitting";
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
    analogWrite(motorPin, 200);
    // count steps!
    Serial.println("In walking state!");
    Serial.print("Steps:"); Serial.print(steps);
    Serial.print("\n");
//    int avg_ay = 0;
//    int i=0;
//    while (i<10) {
//      accelerometer_x = Wire.read()<<8 | Wire.read(); // reading registers: 0x3B (ACCEL_XOUT_H) and 0x3C (ACCEL_XOUT_L)
//      accelerometer_y = Wire.read()<<8 | Wire.read(); // reading registers: 0x3D (ACCEL_YOUT_H) and 0x3E (ACCEL_YOUT_L)
//      accelerometer_z = Wire.read()<<8 | Wire.read(); // reading registers: 0x3F (ACCEL_ZOUT_H) and 0x40 (ACCEL_ZOUT_L)
//      temperature = Wire.read()<<8 | Wire.read(); // reading registers: 0x41 (TEMP_OUT_H) and 0x42 (TEMP_OUT_L)
//      gyro_x = Wire.read()<<8 | Wire.read(); // reading registers: 0x43 (GYRO_XOUT_H) and 0x44 (GYRO_XOUT_L)
//      gyro_y = Wire.read()<<8 | Wire.read(); // reading registers: 0x45 (GYRO_YOUT_H) and 0x46 (GYRO_YOUT_L)
//      gyro_z = Wire.read()<<8 | Wire.read(); // reading registers: 0x47 (GYRO_ZOUT_H) and 0x48 (GYRO_ZOUT_L)
//      avg_ay += accelerometer_y;
//      delay(10);
//      i+=1;
//      Serial.print("aX = "); Serial.print(convert_int16_to_str(accelerometer_x));
//      Serial.print(" | aY = "); Serial.print(convert_int16_to_str(accelerometer_y));
//      Serial.print(" | aZ = "); Serial.print(convert_int16_to_str(accelerometer_z));
//      // the following equation was taken from the documentation [MPU-6000/MPU-6050 Register Map and Description, p.30]
//      Serial.print(" | tmp = "); Serial.print(temperature/340.00+36.53);
//      Serial.print(" | gX = "); Serial.print(convert_int16_to_str(gyro_x));
//      Serial.print(" | gY = "); Serial.print(convert_int16_to_str(gyro_y));
//      Serial.print(" | gZ = "); Serial.print(convert_int16_to_str(gyro_z));
//      Serial.println();
//    }
//    avg_ay = avg_ay / 10;
//    Serial.println(avg_ay);

    int acc = sqrt(sq(accelerometer_x)+sq(accelerometer_y)+sq(accelerometer_z));
    if (acc > 80) {
      steps += 1;
    }
    if (accelerometer_y>-10000 && accelerometer_x>5000) {
      // send json as well
      Serial.println("Someone leaves from walking! Transition to idle state!");
      doc["activity"] = "Walking";
      doc["steps"] = steps;
      doc["start"] = startTime;
      doc["stop"] = String(year())+'-'+String(month())+'-'+String(day())+'-'+ String(hour())+'-'+String(minute())+'-'+String(second());        
      serializeJson(doc, Serial);
      Serial.println("\n");
      steps = 0;
      state = 0;
    }
    else {
      state = 3;
    }
    break;

  }

  //  JsonArray data = doc.createNestedArray("data");
  //  data.add(48.756080);  
  //  data.add(2.302038);

  delay(100);
}
