#include <C:\Program Files (x86)\Arduino\hardware\teensy\avr\cores\teensy3\core_pins.h>
#include <C:\Program Files (x86)\Arduino\hardware\teensy\avr\cores\teensy3\touch.c>
#define BTserial Serial2
int curr=4;
int ns =9;
int ps = 2;
float mod=pow(2,(ps+1))*(ns+1)*4/(107*(curr+1));
int capPins[]={0,1,15,16,17,18,19,22,23,25,32,33}; // All Capacitive Pins
long typMin[]={640, 640, 570, 563, 763, 519, 560, 698, 755, 491, 528, 503}; // if there is nothing attached
long activeMin[12];
int capRead[12]; 
int smoothedVal[12];
int capSize = sizeof(capPins) / sizeof( int );
int capActive=capSize;
int currentRead=0;
int thresh=100;
int tester=0;
float filterVal=.75;
boolean constRun=false;
boolean portFilt=false;
boolean smoothOn=false;
boolean cont=false;
String capName[]={"Port_0,","Port_1,","Port_15,","Port_16,","Port_17,","Port_18,","Port_19,","Port_22,","Port_23,","Port_25,","Port_32,","Port_33,"};
String nameOut="";

void setup() {
  BTserial.setRX(26); BTserial.setTX(31);
  BTserial.begin(9600);
  setTouchReadSensitivity(curr, ns, ps);
  setRun(); 
}

void loop() {
  if (BTserial.available() > 0){int serialData = BTserial.read(); cont=false;  decider(serialData);}  
  if(constRun) getVals();
}

void setTouchReadSensitivity(uint8_t curr_set, uint8_t ns_set, uint8_t ps_set){
  //check the new values are in range
    CURRENT=0+constrain(curr_set,0,15); //0 to 15 - current to use, value is 2*(current+1), default 2
    NSCAN=0+constrain(ns_set,0,31); //number of times to scan, 0 to 31, value is nscan+1, default 9
    PRESCALE=0+constrain(ps_set,0,7); //prescaler, 0 to 7 - value is 2^(prescaler+1), default 2
    mod=pow(2,(ps+1))*(ns+1)*4/(107*(curr+1));
}

void setRun(){
  capActive=0;
  nameOut="";
  for(int x=0;x<capSize; x++){//cycle through the active capacitor pins and form CSV string of readings
    currentRead=touchRead(capPins[x]);//read all ports if not filtering
    if(portFilt){tester=(typMin[x]+thresh)*mod;}
    else{tester=0;}
    if(currentRead>=tester){
      capRead[capActive]=capPins[x];
      activeMin[capActive]=(tester);
      smoothedVal[capActive]=currentRead-activeMin[capActive];
      nameOut+=capName[x];
      capActive++;
      }
  }
  nameOut.remove(nameOut.length()-1);//removes the trailing comma from the name out string
}

void getVals(){//cycle through the active capacitor pins and form CSV string of readings
  String valOut="";
  for(int x=0;x<capActive; x++){currentRead=touchRead(capRead[x])-activeMin[x];//read active ports
      if(smoothOn){currentRead =  smooth(currentRead, filterVal, smoothedVal[x]);smoothedVal[x]=currentRead;}//apply smoothing if flag set
      valOut+=currentRead;//append reading to value out
      if (x<capActive-1){valOut+=",";}}
 BTserial.println(valOut); 
 }

int smooth(int data, float filterVal, int smoothed){
  if (filterVal > 1){filterVal = .99;}
  else if (filterVal <= 0){filterVal = 0;}
  smoothed = (data * (1 - filterVal)) + (smoothed  *  filterVal);
  return smoothed;
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
    case 'E': //smoothing on
        smoothOn=true;
        break;
    case 'F': //smoothing off
        smoothOn=false;
        break;        
    case 'H': //reset filter ports
        portFilt=true;
        setRun();   
        break;
    case 'I': //we will not filter ports
       portFilt=false;
       setRun();  
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
    case 'T': //set sensitivity
      BTserial.print("The Current is : ");  BTserial.print(curr); BTserial.print("The NSCAN is : "); BTserial.print(ns);BTserial.print("The Prescale is : ");BTserial.println(ps);
    default:
        BTserial.flush();
        break;
    }
}
