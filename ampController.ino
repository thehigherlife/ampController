#include <PT2313.h>
#include <Encoder.h>
#include "Adafruit_MCP23017.h"
#include <Wire.h>
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
 #include <avr/power.h> // Required for 16 MHz Adafruit Trinket
#endif 

PT2313 audioChip;
Adafruit_MCP23017 mcp;
//these pins are all on the arduino
Encoder sourceEnc(4, 5);
Encoder volEnc(8, 9);
Encoder btEnc(6, 7);
Encoder balEnc(3, 2);
const int relaypin=11; //pin that relay controller is set to 
const int neo_LED =10; //pin for neopixel
const int neoCount = 8; // how many neopixels are on the strip
// Declare our NeoPixel strip object:
Adafruit_NeoPixel strip(neoCount, neo_LED, NEO_GRB + NEO_KHZ800);
// Argument 1 = Number of pixels in NeoPixel strip
// Argument 2 = Arduino pin number (most are valid)
// Argument 3 = Pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
// these pins are all on the mcp
const int ledpin1=4; //ledpin for 1st LED for b/t/b array 
const int ledpin2=3; //ledpin for 2nd LED for b/t/b array 
const int ledpin3=2;//ledpin for 3rd LED for b/t/b array 
const int ledpin4=1;//ledpin for 4th LED for b/t/b array 
const int ledpin5=0;//ledpin for 5th LED for b/t/b array 
const int ledpins[] = {ledpin1,ledpin2,ledpin3,ledpin4,ledpin5};
const int sourceButtonPin=15;// treble rotart button pin on mcp
const int volButtonPin=12;//volume rotary button pin on mcp
const int btButtonPin=13;// bass rotary button pin on mcp
const int balButtonPin=14;// balance rotary button pin on mcp
long bassOldPosition = 0;
long trebOldPosition = 0;
long balOldPosition = -999;
long sourceOldPosition = -999;
int oldSource = 0; // whats the old source, don't want to keep updating for no reason
long time = 0;         // the last time the output pin was toggled
long debounce = 400;   // the debounce time, increase if the output flickers
//volumes stuff
int volButton; //the current state of the volume button
int volume = 33;
long volOldPosition  = -999;
bool muted = 0; //determine if we're currently muted
bool trebORbass = 1; //am i changing bass or treb, true  for bass  
int btButton; // the current stat of the btButton
bool systemon = 0; // is the system on?
int sourceButton; // the current stat of the source button
// sensitvity modifiers
const int moresense = 2; // this is used to make the vol, bal encoders less sensitive
const int lesssense = 4; // this is used to make the bass, treb less sensitive

void setup(){
  Serial.begin(9600);
  Serial.println("starting program");
  Wire.begin();
  Wire.setWireTimeout(1000, true);
  Serial.println("Wire begins");
  
  //setting up buttons on rotary dials
  delay(1000);
  mcp.begin();      // use default address 0
  //set input pins for the rotary encoders
  pinMode(11, OUTPUT);
  mcp.pinMode(volButtonPin, INPUT); 
  mcp.digitalWrite(volButtonPin, HIGH);
  mcp.pinMode(btButtonPin, INPUT); 
  mcp.digitalWrite(btButtonPin, HIGH);
  mcp.pinMode(sourceButtonPin, INPUT); 
  mcp.digitalWrite(sourceButtonPin, HIGH);
  mcp.pinMode(balButtonPin, INPUT); 
  mcp.digitalWrite(balButtonPin, HIGH);
  volEnc.write(volume * moresense); //set the encoder to default volume
  //set output for LED
  mcp.pinMode(ledpin1, OUTPUT);
  mcp.pinMode(ledpin2, OUTPUT);
  mcp.pinMode(ledpin3, OUTPUT);
  mcp.pinMode(ledpin4, OUTPUT);
  mcp.pinMode(ledpin5, OUTPUT);
  //turn off all LEDs
  updateLEDarray (1, false);
  updateLEDarray (2, false);
  updateLEDarray (3, false);
  updateLEDarray (4, false);
  updateLEDarray (5, false);
  //initialize neopixel
  strip.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
  strip.show();            // Turn OFF all pixels ASAP
  strip.setBrightness(25); // Set BRIGHTNESS to about 1/5 (max = 255)
  strip.clear();
  strip.setPixelColor(1,strip.Color(255,0,0));
  strip.show();
  Serial.println("neopixel setup complete");
  Serial.println("setup done");
}



void loop(){
  if (systemon){
    //Serial.println("i am looping");
    //code for volume (division and multiple is to change sensitivity of encoder)
    long volNewPosition = volEnc.read();
    if (volNewPosition != volOldPosition) {
      volOldPosition = volNewPosition;
      volume = constrain(volOldPosition/2, 0, 62);
      audioChip.volume(volume);
      volumeLED(volume);
      if (volNewPosition / moresense > 62 | volNewPosition < 0){
        volEnc.write(volume * moresense); // don't let that encoder get out of bounds
      }
      Serial.println(volume);
      
    }
    //code for mute (division and multiple is to change sensitivity of encoder)
    volButton = mcp.digitalRead(volButtonPin);
    if (volButton == LOW && millis() - time > debounce) {
      if (muted == 1){
        audioChip.volume(volume);
        volumeLED(volume);
        muted = 0;
        Serial.print("un muted");
      } else {
        audioChip.volume(63);
        muted = 1;
        Serial.print("muted");
        volumeLED(volume);
      }
      Serial.println(" volbutton pressed");
      time = millis();
    }
    //code for bass (division and multiple is to change sensitivity of encoder)
    
    if (trebORbass) {
      long bassNewPosition = btEnc.read();
      if (bassNewPosition != bassOldPosition) {
        bassOldPosition = bassNewPosition;
        int bass = constrain(bassOldPosition / lesssense, -7, 7);
        figureOutLEDarray (bass, -7, 7);
        audioChip.bass(bass);
        if (abs(bassNewPosition / lesssense) > abs(7)){
          btEnc.write(bass * lesssense); // don't let that bass get out of bounds
        }
        Serial.print("bass set: ");
        Serial.println(bass);
      }
    }
    if (trebORbass == false) {
      //code for treble (division and multiple is to change sensitivity of encoder)
      long trebNewPosition = btEnc.read();
      if (trebNewPosition != trebOldPosition) {
        trebOldPosition = trebNewPosition;
        int treb = constrain(trebOldPosition / lesssense, -7, 7);
        figureOutLEDarray (treb, -7, 7);
        audioChip.treble(treb);
        if (abs(trebNewPosition / lesssense) > abs(7)){
          btEnc.write(treb * lesssense); //don't let that treb get out of bounds
        }
        Serial.print("treb set: ");
        Serial.println(treb);
      }
    }
    //code for treb or bass
    btButton = mcp.digitalRead(btButtonPin);
    if (btButton == LOW && millis() - time > debounce) {
      if (trebORbass == 1){
        trebORbass = 0;
        btEnc.write(trebOldPosition);
        Serial.print("treble flag set");
        updateLEDarray (1, true);
        updateLEDarray (2, true);
        updateLEDarray (3, true);
        updateLEDarray (4, true);
        updateLEDarray (5, true);
        delay(50);
        updateLEDarray (1, false);
        updateLEDarray (2, false);
        updateLEDarray (3, false);
        updateLEDarray (4, false);
        updateLEDarray (5, false);
        delay(50);
        updateLEDarray (1, true);
        updateLEDarray (2, true);
        updateLEDarray (3, true);
        updateLEDarray (4, true);
        updateLEDarray (5, true);
        figureOutLEDarray (trebOldPosition / lesssense, -7, 7);
      } else {
        trebORbass = 1;
        btEnc.write(bassOldPosition);
        Serial.print("bass flag set");
        updateLEDarray (1, true);
        updateLEDarray (2, true);
        updateLEDarray (3, true);
        updateLEDarray (4, true);
        updateLEDarray (5, true);
        delay(50);
        updateLEDarray (1, false);
        updateLEDarray (2, false);
        updateLEDarray (3, false);
        updateLEDarray (4, false);
        updateLEDarray (5, false);
        figureOutLEDarray (bassOldPosition / lesssense, -7, 7);
      }
      Serial.println(" btButton pressed");
      time = millis();
    }
      //code for source control (division and multiple is to change sensitivity of encoder)
    long sourceNewPosition = sourceEnc.read();
    if (sourceNewPosition != sourceOldPosition) {
      sourceOldPosition = sourceNewPosition;
      int source = constrain(sourceOldPosition / lesssense, 0, 2);
      Serial.print(oldSource);
      Serial.print(" --- ");
      Serial.println(source);
      if (oldSource != source){
        oldSource = source;
        switch (source)
        {
        case 0:
          updateLEDarray (1, true);
          updateLEDarray (2, false);
          updateLEDarray (3, false);
          updateLEDarray (4, false);
          updateLEDarray (5, false);
          break;
        case 1:
          updateLEDarray (1, true);
          updateLEDarray (2, true);
          updateLEDarray (3, false);
          updateLEDarray (4, false);
          updateLEDarray (5, false);
          break;
        case 2:
          updateLEDarray (1, true);
          updateLEDarray (2, true);
          updateLEDarray (3, true);
          updateLEDarray (4, false);
          updateLEDarray (5, false);
        default:
          break;
        }
      audioChip.source(source);
      }
      
      if (sourceNewPosition / lesssense > abs(2)){
        sourceEnc.write(source * lesssense); //don't let that source get out of bounds
      }
      Serial.print("Source:");
      Serial.println(source);
    }
    //code for balance (division and multiple is to change sensitivity of encoder)
    long balNewPosition = balEnc.read();
    if (balNewPosition != balOldPosition) {
      balOldPosition = balNewPosition;
      int bal = constrain(balOldPosition / moresense, -31, 31);
      audioChip.balance(bal);
      figureOutLEDarray (bal, -31, 31);
      Serial.print("balance: ");
      Serial.println(bal);
      if (balNewPosition /2 > abs(31)){
        balEnc.write(bal * moresense); // don't let that bal get out of bounds
      }
    }
  }
  sourceButton = mcp.digitalRead(sourceButtonPin);
  if (sourceButton == LOW && millis() - time > debounce) {
    if (systemon == 0){
      systemon = 1;
      Serial.print("system powering on, ");
      digitalWrite(relaypin, HIGH);
      strip.setPixelColor(1,strip.Color(0,255,0));
      strip.show();
      delay(2000);
      rainbow(5);
      strip.clear();
      strip.show();
      setupAudioChip();
      Serial.println("power up complete!");
    } else {
      systemon = 0;
      digitalWrite(relaypin, LOW);
      updateLEDarray (1, false);
      updateLEDarray (2, false);
      updateLEDarray (3, false);
      updateLEDarray (4, false);
      updateLEDarray (5, false);
      strip.clear();
      strip.setPixelColor(1,strip.Color(255,0,0));
      strip.show();
      Serial.print("system powering off");

    }
    Serial.println(" sourceButton pressed");
    time = millis();
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
  if (currentPOS == 0) {
    ledBucket = 5; // can do this because i want to force it to be in the middle 
  } else {
    currentPOS = map(currentPOS, minValue, maxValue, 1, totalValue);
    if (currentPOS > (totalValue/2)+1){ // if its greater than the mean, it needs to push to higher number
       ledBucket = ((float)currentPOS/numPerBucket)+1.9;
    } else // its less than half, so it doesn't need to bump as much 
      {
       ledBucket = ((float)currentPOS/numPerBucket)+1;
      }
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
      updateLEDarray (5, true);
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

void setupAudioChip () {
  Serial.println("Starting the Audio audioChip");
  audioChip.initialize(0,true);//source 1,mute on
  audioChip.gain(0);//gain 0...11.27 db
  audioChip.source(0);//select your source 0...3
  audioChip.bass(0);//bass -7...+7
  audioChip.treble(0);//treble -7...+7
  audioChip.balance(0);//-31...+31
  audioChip.volume(33);//Vol 0...62 : 63=muted
  audioChip.loudness(true);//true or false
  Serial.println("AudioChip setup complete");
}

void rainbow(int wait) {
  // Hue of first pixel runs 5 complete loops through the color wheel.
  // Color wheel has a range of 65536 but it's OK if we roll over, so
  // just count from 0 to 5*65536. Adding 256 to firstPixelHue each time
  // means we'll make 5*65536/256 = 1280 passes through this outer loop:
  for(long firstPixelHue = 0; firstPixelHue < 5*65536; firstPixelHue += 256) {
    for(int i=0; i<strip.numPixels(); i++) { // For each pixel in strip...
      // Offset pixel hue by an amount to make one full revolution of the
      // color wheel (range of 65536) along the length of the strip
      // (strip.numPixels() steps):
      int pixelHue = firstPixelHue + (i * 65536L / strip.numPixels());
      // strip.ColorHSV() can take 1 or 3 arguments: a hue (0 to 65535) or
      // optionally add saturation and value (brightness) (each 0 to 255).
      // Here we're using just the single-argument hue variant. The result
      // is passed through strip.gamma32() to provide 'truer' colors
      // before assigning to each pixel:
      strip.setPixelColor(i, strip.gamma32(strip.ColorHSV(pixelHue)));
    }
    strip.show(); // Update strip with new contents
    delay(wait);  // Pause for a moment
  }
}

void volumeLED(int volume){
  int howmanypixels = volume / 8;
  Serial.print("which bucket:");
  Serial.println(howmanypixels);
  strip.clear();
  if (muted == 1){
    for (int i = 0; i <= howmanypixels; i++)
    {
      strip.setPixelColor(i, strip.Color(map(volume,0,62,0,255),0,0));  
    }
    strip.show();
  } else
  {  
    for (int i = 0; i <= howmanypixels; i++)
    {
      strip.setPixelColor(i, strip.Color(0,map(volume,0,62,0,255),0));  
    }
    strip.show();
  }  
}
