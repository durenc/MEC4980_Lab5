#include <Arduino.h>
#include <P1AM.h>

enum MachineStates{
  Waiting,
  ColorSensing,
  CountedMove,
  Estate,
  ArmDetection
};
MachineStates curState=Waiting;

const int modInput = 1;
const int modOutput = 2;
int modAnalog = 3;

//inputs
int Pulsekey= 1;
const int lbIn = 2;
int lbOut = 3;

//outputs
const int conv=1;
const int RobotArmW=6;
const int RobotArmR=7;
const int RobotArmB=8;

//analog input
int color = 1;



/// vars
int colorValue = 10000;
int distToEject = 0;
int distMoved = 0;
int prevKeyState = 0;
bool curKey=0;

int compressor=2;
int EjectW=3;
int EjectR= 4;
int EjectB= 5;
char targetColor = 'b';

int ColorBarrier = 0;

int lbW=4;
int lbR=5;
int lbB=6;

void setup() {
  delay(1000);
  Serial.begin(9600);
  delay(1000);
  //start up P1am modules!
  while(!P1.init()) {
    delay(10);
  }
  
}

bool InputTriggered(){
  return !P1.readDiscrete(1,2);
}

bool OutputTriggered(){
  return !P1.readDiscrete(1,lbOut);
}

void ToggleConveyor(bool s){
  P1.writeDiscrete(s, 2, 1);
}

int GetColor () {
  return P1.readAnalog(modAnalog,color);
}

bool GetPulseKey (){
  return P1.readDiscrete(modInput, Pulsekey);
}

void ToggleCompressor(bool s) {
  P1.writeDiscrete(s,modOutput,compressor);
}
void ToggleArm(bool s,bool a){
  if (s==1) {P1.writeDiscrete(a,modOutput,RobotArmW);
    Serial.println("White Arm Toggled");
  }
  if (s==2) {P1.writeDiscrete(a,modOutput,RobotArmR);
    Serial.println("Red Arm Toggled");
  }
  if (s==3) {P1.writeDiscrete(a, modOutput,RobotArmB);
    Serial.println("Blue Arm Toggled");
  }
}

void UseEjector(char c) {
  int tempPin;
  if (c == 'w') {
    tempPin=EjectW;
  } else if (c == 'r') {
    tempPin=EjectR;
  } else {
    tempPin=EjectB;
  }
  P1.writeDiscrete(true,modOutput,tempPin);
  delay(2000);
  P1.writeDiscrete(false,modOutput,tempPin);
}

int GetBarrier () {
  bool W= !P1.readDiscrete(1,lbW);
  P1.writeDiscrete(W,modOutput,RobotArmW);
  if (W==1) {Serial.println("White Arm Triggered");}
  bool R= !P1.readDiscrete(1,lbR);
  P1.writeDiscrete(R,modOutput,RobotArmR);
  if (R==1) {Serial.println("Red Arm Triggered");}
  bool B= !P1.readDiscrete(1,lbB);
  P1.writeDiscrete(B,modOutput,RobotArmB);
  if (B==1) {Serial.println("Blue Arm Triggered");}
}

void loop() {
  GetBarrier();

  switch (curState){
  case Waiting:
  //wait for light barrier to be tripped
  //after tripped, switch state and turn on conveyor
  if (InputTriggered()) {
    curState= ColorSensing;
    ToggleConveyor(true);
    colorValue = 10000;
  }
  break;
  case ColorSensing:
    // Get color and find min
    colorValue= min(GetColor(), colorValue);
    // Keep going until second light barrier
    //Then swtich States
    if (OutputTriggered()) {
      curState = CountedMove;
      //Decide how far to move
      if (colorValue <2500) {
        distToEject = 3;
        targetColor='w';
        
      } else if (colorValue<4700) {
        distToEject = 9;
        targetColor='r';
      }
      else {
        distToEject = 14;
        targetColor='b';
      }
      distMoved=0;
      ToggleCompressor(true);
    }
  break;
  case CountedMove:
    // Watch pulse key to move that far
    curKey = GetPulseKey();
    if (curKey && !prevKeyState){
      distMoved++;

    }
    prevKeyState = curKey; 
    // Switch states and turn off conveyor
    if (distMoved >= distToEject) {
      curState = Estate;
      ToggleConveyor(false);
    }
  break;
  case Estate:
   // eject that shit
   UseEjector(targetColor);
  curState=Waiting;
  break;  

  default:
  break;
  }
  delay(10);
}

