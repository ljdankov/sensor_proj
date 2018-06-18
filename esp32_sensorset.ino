//this version of arduino code is designed to run on the ESP32 based sleeve or knee brace. It includes as yet unused stub for the accelerometer, and stub for uptime

#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include <utility/imumaths.h>
#include "BluetoothSerial.h"
#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

BluetoothSerial SerialBT;
int capPins[]={14,32,15,33,27,12,13}; // All Capacitive Pins available on the ESP 32. Edit pins to be used here
String capName[]={"Touch_0,","Touch_2,","Touch_3,","Touch_4,","Touch_5,","Touch_6,","Touch_7,","Touch_8,","Touch_9,"};
String nameOut="Touch_0,Touch_3,Touch_4,Touch_5,Touch_6,Touch_7,Touch_8,Touch_9";
int capActive=7;
unsigned long curr; //this is a time holder used to monitor uptime on battery power
boolean constRun=false; //this tells whether or not to simply run as fast as data can be obtained
boolean accelOn=false; //tells whether or not to run accelerometer
boolean uptimeOn=false; //

void setup() {SerialBT.begin("ESP32accel");bno.setExtCrystalUse(true);}

void loop() {
  if (SerialBT.available() > 0){int serialData = BTserial.read(); 
    cont=false;  
    decider(serialData);}  
  String valOut="";
  if(constRun){getVals();}
  if(accelOn){getDirs();}
  BTserial.println(valOut); 
  delayMicroseconds(100);
}

void getVals(){//cycle through the active capacitor pins and form CSV string of readings 
  int currentRead=0;
  for(int x=0;x<capActive; x++){
    currentRead=touchRead(capPins[x]);//read active ports
    valOut+=currentRead;//append reading to value out
    if (x<capActive-1){valOut+="," ;}
  }
  if(accelOn){valOut+=",":}
}
void getDirs(){
  imu::Vector<3> euler = bno.getVector(Adafruit_BNO055::VECTOR_EULER);
  valOut +=euler.x());
  valOut +=",";
  valOut += euler.y());
  valout += ",";
  valOut +=euler.z());
  }
 }

void uptime()
{
 long days=0;
 long hours=0;
 long mins=0;
 long secs=0;
 curr = millis()/1000; //convect milliseconds to seconds
 mins=secs/60; //convert seconds to minutes
 hours=mins/60; //convert minutes to hours
 days=hours/24; //convert hours to days
 secs=secs-(mins*60); //subtract the coverted seconds to minutes in order to display 59 secs max 
 mins=mins-(hours*60); //subtract the coverted minutes to hours in order to display 59 minutes max
 hours=hours-(days*24); //subtract the coverted hours to days in order to display 23 hours max
 //Display results
 BTserial.println("Running Time");
 BTserial.println("------------");
   if (days>0) // days will displayed only if value is greater than zero
 {
   BTserial.print(days);
   BTserial.print(" days and :");
 }
 Btserial.print(hours);
 BTserial.print(":");
 BTserial.print(mins);
 BTserial.print(":");
 BTserial.println(secs);
}
void decider(int serialData){
  switch(serialData){
    case 'A': //get names
       BTserial.println(nameOut);
        break;
    case 'B': //read active caps
        getVals();
        break;
    case 'C': //run full speed
        constRun=true;
        break;
    case 'D': //stop Run
        constRun=false;
        break;
    case 'E': //turn on accelerometer
        accelOn=true;
        nameOut="Touch_0,Touch_3,Touch_4,Touch_5,Touch_6,Touch_7,Touch_8,Touch_9,X,Y,Z";
    case 'F': //turn off accelerometer
        accelOn=false;
        nameOut="Touch_0,Touch_3,Touch_4,Touch_5,Touch_6,Touch_7,Touch_8,Touch_9";
    case 'G': //give current uptime
        uptime();
    default:
        BTserial.flush();
        break;        
  }
