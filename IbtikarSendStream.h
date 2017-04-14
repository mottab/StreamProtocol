/*
This application is developed @ Ibtikar Technologies
Author: Mostafa A. Khattab
Language: Arduino C/C++
Description: This verison of application is dedicated to calculate the speed of bike
             using Hall Effect Sensor + Calculating the direction of the bike using Compass
             Sensor. Feeding this data to the PC/MAC using serial commuincation using our
             own library IbtikarSendStream
*/

#include <TimerOne.h>
#include <Wire.h>
#include <math.h>
#include "IbtikarSendStream.h"

// compass address over I2C Communication line
#define compass_address 0x1E

/* 
Structure for holding data sent to unity/oculus
@property: bike_speed ==> speed calculated using hall effect sensor
@property: bike_speed_precision ==> this is the number after the precision mark - accuracy
@property: bike_direction_angle ==> angle calculated using compass
*/
typedef struct {
  volatile unsigned int bike_speed;
  volatile unsigned int bike_speed_precision;
  volatile unsigned int bike_direction_angle;
} BikeStream;

// Initialize object from BikeStream Struct
BikeStream bikeStream;

// Setting Global Variables

double angle = 0.0; // the angle calculated by compass sensor
volatile int rpmcount = 0; // number of change in magnet fields
volatile int x, y, z; // dimensions of COMPASS
unsigned long rpm = 0.0; // calculated RPM
unsigned long lastMillis = 0, lastMillis_Angle= 0; // updated values for updated millis
float wSpeed = 0.0, wSpeed_precision=0.0; // speed and precesion calculations saved here
const float wheelRad = 65.0; // the radius of the bike
const float Pi = 3.14159265; // PI
bool isFirstRead = false; // To check for first read
int firstAngleRead; // first angle calculated
int mostAngleRead; // most right value = first angle + 90
int leastAngleRead; // least right value = first angle - 90
int mappedAngle; // mapped angle to 180 degree plane ==> supporting unity movements plane


void setup()
{
  isFirstRead = false ; // for first time only
  // serial initialization
  Serial.begin(9600);
  // attaching interrupt for hall effect sensor change mode
  Timer1.initialize(100000); // run ISR every 0.1 seconds
  Timer1.attachInterrupt(angleCalculation);
  
  attachInterrupt(0, rpm_fan, CHANGE);
  // Beginning I2C Communication
  Wire.begin();
  //Put the HMC5883 IC into the correct operating mode
 Wire.beginTransmission(compass_address); //open communication with HMC5883
 Wire.write(0x02); //select mode register (A)
 Wire.write(0x00); //continuous measurement mode (B)
 Wire.endTransmission(); // ending transmission
}

void angleCalculation(void)
{
  // tell compass where to begine reading data
  Wire.beginTransmission(compass_address);
  Wire.write(0x03); // select register 3, X MSB address
  Wire.endTransmission();
  
   //Read data from each axis, 2 registers per axis
  Wire.requestFrom(compass_address, 6);
  if(Wire.available() >= 6)
  {
    x = Wire.read()<<8; //X msb
    x |= Wire.read(); //X lsb
    z = Wire.read()<<8; //Z msb
    z |= Wire.read(); //Z lsb
    y = Wire.read()<<8; //Y msb
    y |= Wire.read(); //Y lsb
  }
  
}

void rpm_fan(void)
{
  ++rpmcount;
}

void loop()
{
  if (millis()-lastMillis == 1000) // calculate speed every one second
  {
      detachInterrupt(0); // deattaching interrupt to calculate speed
      rpm = rpmcount * 3.75; // rpm is counter * (60 second / number of ma8nateeses)
      wSpeed = ((Pi * wheelRad * rpm) / (10000.0/6.0)); // speed = rpm *   mo7eet el kawet4
      wSpeed_precision = (wSpeed - (int)wSpeed)*100;
      bikeStream.bike_speed_precision = wSpeed_precision;
      bikeStream.bike_speed = (int)wSpeed;
      rpmcount = 0;
      lastMillis = millis();  // updating millis
      attachInterrupt(0, rpm_fan, CHANGE);   // attach interrupt again
  }
  noInterrupts();
  replanAndSend();
  interrupts();
}

void replanAndSend(void)
{
   // perform calculations and sending
  angle = atan2((double)y, (double)x) * (180/Pi) ;
  if (angle < 0)
  {
    angle = 360 + angle;
  }
  
  if( ! isFirstRead )
  {
    isFirstRead = true;
    firstAngleRead = angle;
    leastAngleRead = firstAngleRead - 90;
    mostAngleRead = firstAngleRead + 90;
  }
   // check for first read to set the new mapping
  if(leastAngleRead < 0 && angle > mostAngleRead)
  {
    angle = - (360-angle); 
  }
  
  if(mostAngleRead > 360 && angle < leastAngleRead)
  {
    angle += 360;
  }
  mappedAngle = map((int)angle, leastAngleRead, mostAngleRead, 0, 180);
  bikeStream.bike_direction_angle = mappedAngle;
  IbtikarSendStream::sendObject(Serial, &bikeStream, sizeof(bikeStream));
}