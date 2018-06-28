//This version is designed to run on the Teensy 3.2
//These files are needed to adjust sensitivity of touch pins
#include <C:\Program Files (x86)\Arduino\hardware\teensy\avr\cores\teensy3\core_pins.h>
#include <C:\Program Files (x86)\Arduino\hardware\teensy\avr\cores\teensy3\touch.c>
//This initializes Bluetooth
#define BTserial Serial2
//This section initializes default touch sensitivity
int curr=4;
int ns =9;
int ps = 2;
float mod=pow(2,(ps+1))*(ns+1)*4/(107*(curr+1));

//This section initializes capacitive touch pis
int capPins[]={0,1,15,16,17,18,19,22,23,25,32,33}; // All Capacitive Pins
long typMin[]={640, 640, 570, 563, 763, 519, 560, 698, 755, 491, 528, 503}; // if there is nothing attached
long activeMin[12];//holds minimum values of pins
int capRead[12]; //holds values read in each sweep of pins
int smoothedVal[12];//holds values to be used for smoothing
int capSize = sizeof(capPins) / sizeof( int );//calculates number of pins being read
int capActive=capSize; //cap active is initially size of all pins. May change depending on number of pins with electrodes
int currentRead=0;
int thresh=100;
int tester=0;

//Booleans
boolean constRun=false;
boolean portFilt=false;
boolean smoothOn=false;
boolean recTime=false;
boolean cont=false;

String capName[]={"Port_0,","Port_1,","Port_15,","Port_16,","Port_17,","Port_18,","Port_19,","Port_22,","Port_23,","Port_25,","Port_32,","Port_33,"};
String nameOut="";
String pinOut="";
String timeOut="Time,";
//Other ints
float filterVal=.75;
unsigned long cur; //this is a time holder used to monitor uptime on battery power
unsigned long starTime;

void setup() {
  BTserial.setRX(26); BTserial.setTX(31);
  BTserial.begin(9600);
  Serial.begin(9600);
  setTouchReadSensitivity(curr, ns, ps);
  setRun(); 
}

void loop() {
  if (BTserial.available() > 0){int serialData = BTserial.read(); cont=false;  decider(serialData);}  
  if (Serial.available() > 0){int serialData = Serial.read(); cont=false;  decider(serialData);}  
  if(constRun) getVals();
  delayMicroseconds(100);
}

void setTouchReadSensitivity(uint8_t curr_set, uint8_t ns_set, uint8_t ps_set){
  //this bit of code updates values in regards to touch sensitivity on teensy. 
  //not yet implemented on the ESP32 as I have not yet figured out how.
  //Is on the todo list.
    CURRENT=0+constrain(curr_set,0,15); //0 to 15 - current to use, value is 2*(current+1), default 2
    NSCAN=0+constrain(ns_set,0,31); //number of times to scan, 0 to 31, value is nscan+1, default 9
    PRESCALE=0+constrain(ps_set,0,7); //prescaler, 0 to 7 - value is 2^(prescaler+1), default 2
    mod=pow(2,(ps+1))*(ns+1)*4/(107*(curr+1));
}

void setRun(){
  //since by default all pins used on ESP32 are hard coded, this code only applies to the Teensy at moment
  capActive=0;
  pinOut="";
  for(int x=0;x<capSize; x++){//cycle through the active capacitor pins and form CSV string of readings
    currentRead=touchRead(capPins[x]);//read all ports if not filtering
    //the next section of code is not implemented for the ESP32 as yet (and may not be given some of the challenges on ESP32)
    //If portFiltering is active, it sets a tester value based on each pins typical capacitive reading without an electrode attached.
    //It then tests to see whether the capacitance exceeds that minimum by some threshold and a multiplier
    //The multiplier should account for touch sensitivity settings, but it is not entirely accurate (may eventually move to a more 'spreadsheet based approach'
    //If the first reading on a pin is higher than the test value, it is assumed that the pin is attached to an electrode, and it is included in set of pins to be read
    if(portFilt){tester=(typMin[x]+thresh)*mod;}//set a test value based on each ports typical un
    else{tester=0;}    
    if(currentRead>=tester){
      capRead[capActive]=capPins[x];
      activeMin[capActive]=(tester);
      smoothedVal[capActive]=currentRead-activeMin[capActive];//this line initializes values to be used for smoothing data.
      pinOut+=capName[x];
      capActive++;
      }
  }
  pinOut=pinOut.substring(0,pinOut.length()-1);//removes the trailing comma from the name out string
  nameOut=pinOut
}

void getVals(){//cycle through the active capacitor pins and form CSV string of readings
  String valOut="";
  if(recTime==true){cur=millis()-starTime; valOut+=cur; valOut+=",";}
  for(int x=0;x<capActive; x++){currentRead=touchRead(capRead[x])-activeMin[x];//read active ports
      if(smoothOn){currentRead =  smooth(currentRead, filterVal, smoothedVal[x]);smoothedVal[x]=currentRead;}//apply smoothing if flag set
      valOut+=currentRead;//append reading to value out
      if (x<capActive-1){valOut+=",";}}
 BTserial.println(valOut); 
 Serial.println(valOut); 
 }

int smooth(int data, float filterVal, int smoothed){
  //note that this is not fully implemented. It was started and the abandoned
  if (filterVal > 1){filterVal = .99;}
  else if (filterVal <= 0){filterVal = 0;}
  smoothed = (data * (1 - filterVal)) + (smoothed  *  filterVal);
  return smoothed;
}

void uptime()
{
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
   if (days>0) // days will displayed only if value is greater than zero
 {
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
    case 'E': //smoothing on
        smoothOn=true;
        break;
    case 'F': //smoothing off
        smoothOn=false;
        break;
    case 'G': //give current uptime
        uptime();
        break;        
    case 'H': //include time
        nameOut=timeOut+pinOut;
        BTserial.println(nameOut);Serial.println(nameOut);
        recTime=true;
        starTime=millis();
        break;
    case 'I': //Do not include time
       nameOut=pinOut
       BTserial.println(nameOut);Serial.println(nameOut);
       recTime=false;
       break;
    case 'L'://filter ports
       portFilt=true;
       setRun();
       if(recTime==true){nameOut=timeOut+nameOut;}
       nameOut=pinOut;
       break;
    case 'M'://do not filter ports   
       portFilt=false;
       setRun();
       nameOut=pinOut;
       if(recTime==true){nameOut=timeOut+nameOut;}
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
