//This section gets Bluetooth working
#include "BluetoothSerial.h"
#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif
BluetoothSerial BTserial;

//This section is used to initialize the accelerometer. 
//Not currently applicale to the teensy as required pins not exposed on teensy
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include <utility/imumaths.h>
Adafruit_BNO055 bno = Adafruit_BNO055();
//This section initializes the touch pins
int capPins[]={A6,A9,A8,A7,A10,A11}; // All Capacitive Pins available on the ESP 32. 
//Note that A12 does not produce any signal but does inject a lot of random noise. 
//When electrode connected to A12 was removed was removed, signal stabilized.
//Also note A9 and A7 backwards from diagram
int capSize = sizeof(capPins) / sizeof( int );
int smoothedVal[12];//holds values to be used for smoothing
int capActive=capSize;
//Placeholders for strings
String timeOut="Time,";
String pinOut="Touch_6,Touch_8,Touch_3,Touch_9,Touch_7,Touch_5";
String accOut=",X,Y,Z";
String nameOut="";
String valOut="";
//Booleans
boolean constRun=false; //toggles constnt vs requeste data collection
boolean smoothOn=false; //toggles simple smoothing algarithm
boolean accelOn=false; //toggles accelerometer on/off
boolean recTime=false; //toggles timestamp recording
//Timing values
unsigned long cur; //this is a time holder used to monitor uptime on battery power
unsigned long starTime;
float filterVal=.75;

void setup() {
  BTserial.begin("ESP32accel");
  bno.setExtCrystalUse(true);
  setRun(); 
  nameOut=pinOut;     
}

void loop() {
  if (BTserial.available() > 0){
    int serialData = BTserial.read(); 
    if(serialData >=97 && serialData <=122){serialData=serialData-32;}
      decider(serialData);}  
  if(constRun){getVals();}
  delayMicroseconds(100);
}

void setRun(){
  //included so doing crude filtering on the ESP32 will be an option
  for(int x=0;x<capSize; x++){//cycle through the active capacitor pins and form CSV string of readings
      smoothedVal[x]=touchRead(capPins[x]);
  }
}

void getVals(){//cycle through the active capacitor pins and form CSV string of readings 
  int currentRead=0;
  String valOut="";
  if(recTime==true){cur=millis()-starTime; valOut+=cur; valOut+=",";}
  for(int x=0;x<capActive; x++){
    currentRead=touchRead(capPins[x]);//read active ports
    if(smoothOn){currentRead =  smooth(currentRead, filterVal, smoothedVal[x]);smoothedVal[x]=currentRead;}//apply smoothing if flag set
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
BTserial.println(valOut);  
}

int smooth(int data, float filterVal, int smoothed){
  //note that this is not fully implemented. It was started and the abandoned
  if (filterVal > 1){filterVal = .99;}
  else if (filterVal <= 0){filterVal = 0;}
  smoothed = (data * (1 - filterVal)) + (smoothed  *  filterVal);
  return smoothed;
}

void uptime(){
 long days=0;
 long hours=0;
 long mins=0;
 long secs=0;
 cur = millis();
 secs=cur/1000;//convect milliseconds to seconds
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
 BTserial.print(hours);
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
        starTime=millis();
        break;
    case 'D': //stop Run
        constRun=false;
        break;
    case 'E': //smoothing on
        smoothOn=true;
        break;
    case 'F': //smoothing off
        smoothOn=false;
        break;
    case 'G': //give curent uptime
        uptime();
        break;
    case 'H': //include time in output string this is used for the interface with python data collection tool
        nameOut=timeOut+pinOut;
        if(accelOn){nameOut=nameOut+accOut;}
        BTserial.println(nameOut);
        recTime=true;
        starTime=millis();
        break;
    case 'I': //do not inclue time in output string in case we want to toggle off
        nameOut=pinOut;
        if(accelOn){nameOut=nameOut+accOut;}
        BTserial.println(nameOut);
        recTime=false;
        break;
    case 'J': //turn on accelerometer
        accelOn=true;
        nameOut=pinOut+accOut;
        if(recTime){nameOut=timeOut+pinOut+accOut;}
        BTserial.println(nameOut);
        break;
    case 'K': //turn off accelerometer
        accelOn=false;
        nameOut=pinOut;
        if(recTime){nameOut=timeOut+pinOut;}
        BTserial.println(nameOut);
        break;
    default:
        BTserial.flush();
        break;        
  }
}    
