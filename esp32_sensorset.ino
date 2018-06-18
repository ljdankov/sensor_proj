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
String capName[]={"Touch_0,","Touch_2,","Touch_3,","Touch_4,","Touch_5,","Touch_6,","Touch_7,","Touch_8,"};
String nameOut="Touch_0,Touch_3,Touch_4,Touch_5,Touch_6,Touch_7,Touch_8,Time,";
String valOut="";
int capActive=7;
unsigned long curr; //this is a time holder used to monitor uptime on battery power
boolean constRun=false; //this tells whether or not to simply run as fast as data can be obtained
boolean accelOn=false; //tells whether or not to run accelerometer
boolean recTime=false; //toggle to tell whether or not to record timestamp
Adafruit_BNO055 bno = Adafruit_BNO055();

void setup() {
  SerialBT.begin("ESP32accel");
  bno.setExtCrystalUse(true);
}

void loop() {
  if (SerialBT.available() > 0){int serialData = SerialBT.read(); decider(serialData);}  
  if(constRun){getVals();}
  delayMicroseconds(1000);
}

void getVals(){//cycle through the active capacitor pins and form CSV string of readings 
  int currentRead=0;
  String valOut="";
  if(recTime==true){curr=millis(); valOut+=curr; valOut+=",";}
  for(int x=0;x<capActive; x++){
    currentRead=touchRead(capPins[x]);//read active ports
    valOut+=currentRead;//append reading to value out
    if( x <capActive-1){valOut+=",";} 
  }
  
  if(accelOn){
    imu::Vector<3> euler = bno.getVector(Adafruit_BNO055::VECTOR_EULER);
    valOut+=",";
    valOut +=euler.x();
    valOut +=",";
    valOut += euler.y();
    valOut += ",";
    valOut +=euler.z();
    }
SerialBT.println(valOut);  
}

void uptime(){
 long days=0;
 long hours=0;
 long mins=0;
 long secs=0;
 curr = millis();
 secs=curr/1000;//convect milliseconds to seconds
 mins=secs/60; //convert seconds to minutes
 hours=mins/60; //convert minutes to hours
 days=hours/24; //convert hours to days
 secs=secs-(mins*60); //subtract the coverted seconds to minutes in order to display 59 secs max 
 mins=mins-(hours*60); //subtract the coverted minutes to hours in order to display 59 minutes max
 hours=hours-(days*24); //subtract the coverted hours to days in order to display 23 hours max
 //Display results
 SerialBT.println("Running Time");
 SerialBT.println("------------");
   if (days>0) // days will displayed only if value is greater than zero
 {
   SerialBT.print(days);
   SerialBT.print(" days and :");
 }
 SerialBT.print(hours);
 SerialBT.print(":");
 SerialBT.print(mins);
 SerialBT.print(":");
 SerialBT.println(secs);
}

void decider(int serialData){
  switch(serialData){
    case 'A': //get names
       SerialBT.println(nameOut);
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
        SerialBT.println(nameOut);
        break;
    case 'F': //turn off accelerometer
        accelOn=false;
        nameOut="Touch_0,Touch_3,Touch_4,Touch_5,Touch_6,Touch_7,Touch_8,Touch_9";
        SerialBT.println(nameOut);
        break;
    case 'G': //give current uptime
        uptime();
        break;
    case 'H': //include time in output string this is used for the interface with python data collection tool
        nameOut="Time,Touch_0,Touch_3,Touch_4,Touch_5,Touch_6,Touch_7,Touch_8,Touch_9";
        SerialBT.println(nameOut);
        recTime=true;
        break;
    case 'I': //do not inclue time in output string in case we want to toggle off
        nameOut="Touch_0,Touch_3,Touch_4,Touch_5,Touch_6,Touch_7,Touch_8,Touch_9";
        SerialBT.println(nameOut);
        recTime=false;
        break;
    default:
        SerialBT.flush();
        break;        
  }
}    
