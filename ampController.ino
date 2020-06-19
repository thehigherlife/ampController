#include <PT2313.h>
#include <Encoder.h>
#include "Adafruit_MCP23017.h"
#include <Wire.h>


PT2313 audioChip;
Adafruit_MCP23017 mcp;
Encoder volEnc(9, 8);
Encoder bassEnc(7, 6);
Encoder trebEnc(5, 4);
Encoder balEnc(3, 2);
const int volButtonPin=15;//volume rotary button pin on mcp
const int bassButtonPin=14;// bass rotary button pin on mcp
const int trebButtonPin=13;// treble rotart button pin on mcp
const int balButtonPin=12;// balance rotary button pin on mcp
const int ledpin1=4; //ledpin for 1st LED for b/t/b array 
const int ledpin2=3; //ledpin for 2nd LED for b/t/b array 
const int ledpin3=2;//ledpin for 3rd LED for b/t/b array 
const int ledpin4=1;//ledpin for 4th LED for b/t/b array 
const int ledpin5=0;//ledpin for 5th LED for b/t/b array 
const int ledpins[] = {ledpin1,ledpin2,ledpin3,ledpin4,ledpin5};
long bassOldPosition = -999;
long trebOldPosition = -999;
long balOldPosition = -999;
long time = 0;         // the last time the output pin was toggled
long debounce = 200;   // the debounce time, increase if the output flickers
//volumes stuff
int volButton; //the current state of the volume button
int volume = 33;
long volOldPosition  = -999;
bool muted = 0; //determine if we're currently muted
bool trebORbass = 1; //am i changing bass or treb, true  for bass  

void setup(){
  Serial.begin(9600);
  Serial.println("starting program");
  Wire.begin();
  audioChip.initialize(0,true);//source 1,mute on
  audioChip.gain(0);//gain 0...11.27 db
  audioChip.source(0);//select your source 0...3
  audioChip.loudness(true);//true or false
  audioChip.bass(0);//bass -7...+7
  audioChip.treble(0);//treble -7...+7
  audioChip.balance(0);//-31...+31
  audioChip.volume(33);//Vol 0...62 : 63=muted
  Serial.println("made it to the end");
  Serial.println("Preamp setup done");
  //setting up buttons on rotary dials
  delay(5000);
  mcp.begin();      // use default address 0
  //set input pins for the rotary encoders
  mcp.pinMode(volButtonPin, INPUT); 
  mcp.digitalWrite(volButtonPin, HIGH);
  mcp.pinMode(bassButtonPin, INPUT); 
  mcp.pinMode(trebButtonPin, INPUT); 
  mcp.pinMode(balButtonPin, INPUT); 
  volEnc.write(volume); //set the encoder to default volume
  //set output for LED
  mcp.pinMode(ledpin1, OUTPUT);
  mcp.pinMode(ledpin2, OUTPUT);
  mcp.pinMode(ledpin3, OUTPUT);
  mcp.pinMode(ledpin4, OUTPUT);
  mcp.pinMode(ledpin5, OUTPUT);
}



void loop(){
  //Serial.println("i am looping");
  //code for volume (division and multiple is to change sensitivity of encoder)
  long volNewPosition = volEnc.read();
  if (volNewPosition != volOldPosition) {
    volOldPosition = volNewPosition;
    volume = constrain(volOldPosition/2, 0, 62);
    audioChip.volume(volume);
    if (volNewPosition / 2 > 62 | volNewPosition < 0){
      volEnc.write(volume * 2); // don't let that encoder get out of bounds
    }
    Serial.println(volume);
    
  }
  //code for mute (division and multiple is to change sensitivity of encoder)
  volButton = mcp.digitalRead(volButtonPin);
  if (volButton == LOW && millis() - time > debounce) {
    if (muted == 1){
      audioChip.volume(volume);
      muted = 0;
      Serial.print("un muted");
    } else {
      audioChip.volume(63);
      muted = 1;
      Serial.print("muted");
    }
    Serial.println(" volbutton pressed");
    time = millis();
  }
  //code for bass (division and multiple is to change sensitivity of encoder)
  long bassNewPosition = bassEnc.read();
  if (bassNewPosition != bassOldPosition) {
    bassOldPosition = bassNewPosition;
    int bass = constrain(bassOldPosition / 4, -7, 7);
    figureOutLEDarray (bass, -7, 7);
    audioChip.bass(bass);
    if (bassNewPosition / 4 > abs(7)){
      bassEnc.write(bass * 4); // don't let that bass get out of bounds
    }
    Serial.println(bass);
  }
  //code for treble (division and multiple is to change sensitivity of encoder)
  long trebNewPosition = trebEnc.read();
  if (trebNewPosition != trebOldPosition) {
    trebOldPosition = trebNewPosition;
    int treb = constrain(trebOldPosition / 4, -7, 7);
    figureOutLEDarray (treb, -7, 7);
    audioChip.treble(treb);
    if (trebNewPosition / 4 > abs(7)){
      trebEnc.write(treb * 4); //don't let that treb get out of bounds
    }
    Serial.println(treb);
  }
  //code for balance (division and multiple is to change sensitivity of encoder)
  long balNewPosition = balEnc.read();
  if (balNewPosition != balOldPosition) {
    balOldPosition = balNewPosition;
    int bal = constrain(balOldPosition / 2, -31, 31);
    audioChip.balance(bal);
    figureOutLEDarray (bal, -31, 31);
    Serial.println(bal);
    if (balNewPosition /2 > abs(31)){
      balEnc.write(bal * 2); // don't let that bal get out of bounds
    }
    
  }
}  

int updateLEDarray (int lednum, bool ledstate){
  lednum = lednum - 1; //so i don't have to remember array pos vs led number
  if (ledstate == false){
    mcp.digitalWrite(ledpins[lednum], LOW);
  } else {
    mcp.digitalWrite(ledpins[lednum], HIGH);
  }
}

int figureOutLEDarray (int currentPOS, int minValue, int maxValue ){
  int absminValue = abs(minValue); //if value is negative make it positive
  int ledBucket = 0;
  int totalValue = absminValue + maxValue; // find out how many positions there are
  int numPerBucket = totalValue / 8 + (totalValue % 9 !=0); // round up if there is a remainder, find out how many numbers are in each of the  buckets
  currentPOS = map(currentPOS, minValue, maxValue, 1, totalValue);
  if (currentPOS > (totalValue/2)+1){ // if its greater than the mean, it needs to push to higher number
     ledBucket = ((float)currentPOS/numPerBucket)+1.9;
  } else // its less than half, so it doesn't need to bump as much 
  {
     ledBucket = ((float)currentPOS/numPerBucket)+1;
  }
  //start the logic based on which bucket it goes into
  switch (ledBucket){
    case 1:
      updateLEDarray (1, true);
      updateLEDarray (2, false);
      updateLEDarray (3, false);
      updateLEDarray (4, false);
      updateLEDarray (5, false);
      break;
    case 2:
      updateLEDarray (1, true);
      updateLEDarray (2, true);
      updateLEDarray (3, false);
      updateLEDarray (4, false);
      updateLEDarray (5, false);
      break;
    case 3:
      updateLEDarray (1, false);
      updateLEDarray (2, true);
      updateLEDarray (3, false);
      updateLEDarray (4, false);
      updateLEDarray (5, false);
      break;
    case 4:
      updateLEDarray (1, false);
      updateLEDarray (2, true);
      updateLEDarray (3, true);
      updateLEDarray (4, false);
      updateLEDarray (5, false);
      break;
    case 5:
      updateLEDarray (1, false);
      updateLEDarray (2, false);
      updateLEDarray (3, true);
      updateLEDarray (4, false);
      updateLEDarray (5, false);
      break;
    case 6:
      updateLEDarray (1, false);
      updateLEDarray (2, false);
      updateLEDarray (3, true);
      updateLEDarray (4, true);
      updateLEDarray (5, false);
      break;
    case 7:
      updateLEDarray (1, false);
      updateLEDarray (2, false);
      updateLEDarray (3, false);
      updateLEDarray (4, true);
      updateLEDarray (5, false);
      break;
    case 8:
      updateLEDarray (1, false);
      updateLEDarray (2, false);
      updateLEDarray (3, false);
      updateLEDarray (4, true);
      updateLEDarray (5, true);
      break;
    case 9:
      updateLEDarray (1, false);
      updateLEDarray (2, false);
      updateLEDarray (3, false);
      updateLEDarray (4, false);
      updateLEDarray (5, false);
      break;
    default:
      updateLEDarray (1, true);
      updateLEDarray (2, true);
      updateLEDarray (3, true);
      updateLEDarray (4, true);
      updateLEDarray (5, true);
      break;
  }

}
