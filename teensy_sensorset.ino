//This version is designed to run on the Teensy 3.2
//These files are needed to adjust sensitivity of touch pins
#include <C:\Program Files (x86)\Arduino\hardware\teensy\avr\cores\teensy3\core_pins.h>
#include <C:\Program Files (x86)\Arduino\hardware\teensy\avr\cores\teensy3\touch.c>
//This section initializes default touch sensitivity
int curr=4;
int ns =9;
int ps = 2;
float mod=pow(2,(ps+1))*(ns+1)*4/(107*(curr+1));

//This initializes Bluetooth
#define BTserial Serial2

//This section handles initializing the IMU units.
#include <i2c_t3.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055_t3.h>
#include <utility/imumaths.h>
#include <Encoder.h>
Adafruit_BNO055 arm  = Adafruit_BNO055(WIRE1_BUS, 1, BNO055_ADDRESS_B, I2C_MASTER, I2C_PINS_29_30, I2C_PULLUP_INT, I2C_RATE_100, I2C_OP_MODE_ISR);
Adafruit_BNO055 hand = Adafruit_BNO055(WIRE1_BUS, 2, BNO055_ADDRESS_A, I2C_MASTER, I2C_PINS_29_30, I2C_PULLUP_INT, I2C_RATE_100, I2C_OP_MODE_ISR);
String BNO_out="";

//This section initializes capacitive touch pins
//Note this is all pins available on current board layout and may be modified for individual use.
//String capName[]={"Cap_1,", "Cap_2,", "Cap_3,", "Cap_4,", "Cap_5,","Cap_6,","Cap_7,", "Cap_8,", "Cap_9,"};
//int capPins[]={23,22,19,18,33,17,32,16,15}; // All Capacitive Pins
//this is currently selected pins on teensy
String capName[]={"Cap_1,", "Cap_2,", "Cap_3,", "Cap_4,", "Cap_5,","Cap_6,","Cap_7,"};
int capPins[]={23,22,19,18,32,16,15}; // All Capacitive Pins
long typMin[]={640, 640, 570, 563, 763, 519, 560, 698, 755, 491, 528, 503}; // if there is nothing attached
long activeMin[9];//holds minimum values of pins
int capRead[9]; //holds values read in each sweep of pins
int capSize = sizeof(capPins) / sizeof( int );//calculates number of pins being read
int capActive=capSize; //cap active is initially size of all pins. May change depending on number of pins with electrodes
int currentRead=0;
int thresh=100;
int tester=0;

// Pressure sensor pins
//Note, this is in the order that they will be implemented on pins. 
//Also note that A14 is not in the PCB and will need to be manually wired.
byte pressPins[]={A14, A0, A20, A6, A7, A11};//note that A14 does not have a defined pin refference so we use bytes to refference
String pressName[]={"FRS_0,","FRS_1,","FRS_2,","FRS_3,","FRS_4,","FRS_5,"};
byte pressRead[6];
int pressSize=6;//calculates number of pins being read
int pressActive=pressSize; //number of pressure sensors currently active. May differ from number of pressure sensors available

//Booleans
boolean constRun=false;
boolean portFilt=false;
boolean recTime=false;
boolean cont=false;
boolean pressRun=false;
boolean bno_hand=false;
boolean bno_arm=false;
boolean bno_read=true;
//Strings
String nameOut="";
String pinOut="";
String capVals="";
String timeOut="Time,";
String timeVals="";
String pressOut="";
String pressVals="";
String BNOOut="";
String BNOVals="";

//Other ints
float filterVal=.75;
unsigned long cur; //this is a time holder used to monitor uptime on battery power
unsigned long starTime;
int serialData;
int bnoTime=1; //only run BNO every fifth cycle to speed up process?
void setup() {
  BTserial.setRX(9); BTserial.setTX(10);
  BTserial.begin(9600);
  Serial.begin(9600);
  int tries =0;
  if(!arm.begin()){
    while(tries<20){
    if(Serial){Serial.println("arm not starting up");} 
    if(BTserial){BTserial.println("arm not starting up");}
    tries++;}
  }
  else{
    if(Serial){Serial.println("arm seems to be working!");}
    if(BTserial){BTserial.println("arm seems to be working!");}
    bno_arm=true;
    BNO_out+="BNO_ax,BNO_ay,BNO_az,";
    arm.setExtCrystalUse(true);
  }  
  tries=0;
  if(!hand.begin()){
    while(tries<20){
      if(Serial){Serial.println("hand not starting up");}
      if(BTserial){BTserial.println("hand not starting up");}
      tries++;}
  }
  else{
    if(Serial){Serial.println("hand seems to be working!");}
    if(BTserial){BTserial.println("hand seems to be working!");}
    bno_hand=true;
    BNO_out+="BNO_hx,BNO_hy,BNO_hz,";}
  hand.setExtCrystalUse(true);
  setTouchReadSensitivity(curr, ns, ps);
  setRun(); 
}

void loop() {
  if (BTserial.available() > 0){ serialData = BTserial.read();} 
  if (Serial.available() > 0){ serialData = Serial.read();}  
  if(serialData >=97 && serialData <=122){serialData=serialData-32;}
  cont=false;  
  decider(serialData);  
  if(constRun) getVals();
}

void setTouchReadSensitivity(uint8_t curr_set, uint8_t ns_set, uint8_t ps_set){
  //this bit of code updates values in regards to touch sensitivity on teensy. 
    CURRENT=0+constrain(curr_set,0,15); //0 to 15 - current to use, value is 2*(current+1), default 2
    NSCAN=0+constrain(ns_set,0,31); //number of times to scan, 0 to 31, value is nscan+1, default 9
    PRESCALE=0+constrain(ps_set,0,7); //prescaler, 0 to 7 - value is 2^(prescaler+1), default 2
    mod=pow(2,(ps+1))*(ns+1)*4/(107*(curr+1));
}

void setRun(){
  //since by default all pins used on ESP32 are hard coded, this code only applies to the Teensy at moment
  capActive=0;
  pressActive=0;
  pinOut="";
  for(int x=0;x<capSize; x++){//cycle through the active capacitor pins and form CSV string of readings
    currentRead=touchRead(capPins[x]);//read all ports if not filtering
    if(portFilt){tester=(typMin[x]+thresh)*mod;}//set a test value based on each ports typical un
    else{tester=0;}    
    if(currentRead>=tester){
      capRead[capActive]=capPins[x];
      activeMin[capActive]=(tester);
      pinOut+=capName[x];
      capActive++;
      }
    }
   for(int x=0;x<pressSize; x++){
    currentRead=analogRead(pressPins[x]);
        if(currentRead<=10){
        pressRun=true;
        pressRead[pressActive]=pressPins[x];
        pressOut+=pressName[x];
        pressActive++;
      }
    }
 nameOut=pinOut+pressOut;
 nameOut+=BNO_out;
 nameOut=nameOut.substring(0,nameOut.length()-1);//removes the trailing comma from the name out string
 Serial.println(nameOut);
 BTserial.println(nameOut);
}

void getVals(){//cycle through the active capacitor pins and form CSV string of readings
  String valOut="";
  if(recTime==true){cur=millis()-starTime; valOut+=cur; valOut+=",";}
  for(int x=0;x<capActive; x++){
    currentRead=touchRead(capRead[x])-activeMin[x];//read active ports
    valOut+=currentRead;//append reading to value out
    if (x<capActive-1){valOut+=",";}
    }
  
  if(pressRun==true){
    valOut+=",";
    for(int x=0;x<pressActive;x++){
      int pres=analogRead(pressRead[x]);
      if(pres<=60){pres=0;}
      else{pres=1;}
      valOut+=pres;
      if (x<pressActive-1){valOut+=",";}
      }
  }
  if(bno_read==true){
    if(bno_hand==true || bno_arm==true){
      if(bnoTime==1){
        BNOVals="";
        sensors_event_t event;
        arm.getEvent(&event);
        if (bno_hand==true){
          BNOVals+=",";
          imu::Vector<3> euler = arm.getVector(Adafruit_BNO055::VECTOR_EULER);
          BNOVals+=euler.x();
          BNOVals+=",";
          BNOVals+=euler.y();
          BNOVals+=",";
          BNOVals+=euler.z();
          }
      if(bno_arm==true){
        BNOVals+=",";
        hand.getEvent(&event);
        imu::Vector<3> eulerTwo = hand.getVector(Adafruit_BNO055::VECTOR_EULER);
        BNOVals+=eulerTwo.x();
        BNOVals+=",";
        BNOVals+=eulerTwo.y();
        BNOVals+=",";
        BNOVals+=eulerTwo.z();
        }
      }
    if(bnoTime<=5){bnoTime++;}
    if(bnoTime>5){bnoTime=0;}
    valOut+=BNOVals;
    }
  }
  BTserial.println(valOut); 
  Serial.println(valOut); 
}

void uptime(){
  long days=0;
  long hours=0;
  long mins=0;
  long secs=0;
  cur = millis()/1000; //convect milliseconds to seconds
  mins=secs/60; //convert seconds to minutes
  hours=mins/60; //convert minutes to hours
  days=hours/24; //convert hours to days
  secs=secs-(mins*60); //subtract the coverted seconds to minutes in order to display 59 secs max 
  mins=mins-(hours*60); //subtract the coverted minutes to hours in order to display 59 minutes max
  hours=hours-(days*24); //subtract the coverted hours to days in order to display 23 hours max
  //Display results
  BTserial.println("Running Time"); Serial.println("Running Time");
  BTserial.println("------------"); Serial.println("------------");
  if (days>0){ // days will displayed only if value is greater than zero
   BTserial.print(days);   Serial.print(days);
   BTserial.print(" days and :"); Serial.print(" days and :");
   }
  BTserial.print(hours); Serial.print(hours);
  BTserial.print(":");Serial.print(":");
  BTserial.print(mins); Serial.print(mins);
  BTserial.print(":"); Serial.print(":");
  BTserial.println(secs); Serial.println(secs);
}

void decider(int serialData){
  switch(serialData){
    case 'A': //get names
        BTserial.println(nameOut); Serial.println(nameOut);
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
    case 'G': //give current uptime
        uptime();
        break;        
    case 'H': //include time
       nameOut=timeOut+pinOut;
       nameOut=nameOut.substring(0,nameOut.length()-1);//removes the trailing comma from the name out string
       Serial.println(nameOut); BTserial.println(nameOut);       
       recTime=true;
        starTime=millis();
        break;
    case 'I': //Do not include time
       nameOut=pinOut;
       BTserial.println(nameOut);Serial.println(nameOut);
       recTime=false;
       break;
    case 'L'://filter ports
       portFilt=true;
       setRun();
       if(recTime==true){nameOut=timeOut+nameOut;}
       nameOut=nameOut.substring(0,nameOut.length()-1);//removes the trailing comma from the name out string
       Serial.println(nameOut); BTserial.println(nameOut);       
       break;
    case 'M'://do not filter ports   
       portFilt=false;
       setRun();
       nameOut=pinOut;
       if(recTime==true){nameOut=timeOut+nameOut;}
       nameOut=nameOut.substring(0,nameOut.length()-1);//removes the trailing comma from the name out string
       Serial.println(nameOut); BTserial.println(nameOut);       
       break;
    case 'N': // enable touch sensor
       pressRun=true;
       nameOut=nameOut+=pressOut;
       nameOut=nameOut.substring(0,nameOut.length()-1);//removes the trailing comma from the name out string
       Serial.println(nameOut); BTserial.println(nameOut);       
       break;
    case 'O'://disable
       pressRun=false;
       nameOut=pinOut;
       if(recTime==true){nameOut=timeOut+nameOut;}   
       nameOut=nameOut.substring(0,nameOut.length()-1);//removes the trailing comma from the name out string
       Serial.println(nameOut); BTserial.println(nameOut);       
       break;
    case 'P'://disable BNO
       bno_read=false;
       nameOut=pinOut;
       if(recTime==true){nameOut=timeOut+nameOut;}   
       if(pressRun==true){nameOut=nameOut+pressOut;}
       nameOut=nameOut.substring(0,nameOut.length()-1);//removes the trailing comma from the name out string
       Serial.println(nameOut); BTserial.println(nameOut);       
       break;
    case 'Q'://enable BNO
        bno_read=true;
        nameOut=pinOut;
        if(recTime==true){nameOut=timeOut+nameOut;}   
        if(pressRun==true){nameOut=nameOut+pressOut;}
        nameOut=nameOut+BNO_out;
        nameOut=nameOut.substring(0,nameOut.length()-1);//removes the trailing comma from the name out string
        Serial.println(nameOut); BTserial.println(nameOut);
        break;     
    case 'R': //set sensitivity
      BTserial.println("Enter selections for Current(0-15), NSCAN(0-31) and PrescaleR(0-7)");
      while (cont==false){
        if(BTserial.available() > 0) {curr = BTserial.parseInt(); ns= BTserial.parseInt(); ps = BTserial.parseInt();
          if (BTserial.read() == '\n') {
            setTouchReadSensitivity(curr, ps, ns);   
            BTserial.print("Current Set To :"); BTserial.println(CURRENT);
            BTserial.print("NS Set To :"); BTserial.println(NSCAN);
            BTserial.print("PS Set To :"); BTserial.println(PRESCALE);
            BTserial.print("Modifier Set To :"); BTserial.println(mod);
            BTserial.println("Sensitivity Set");
            setRun();     
            cont=true;
            break;
            }
        }
      } 
    case 'T': //print sensitivity
      BTserial.print("The Current is : ");  BTserial.print(curr); BTserial.print("The NSCAN is : "); BTserial.print(ns);BTserial.print("The Prescale is : ");BTserial.println(ps);
    default:
        BTserial.flush();
        break;
    }
}
