// Sensorium - Kiwiburn 2014 art installation.
//
// There are some nasty hacks in this code due to time constraints and weekend of
// trying to get something working before the festival!
//
// The set up involves two Teensy 3.1's connected by their SPI pins.
// One has this code loaded on it, with masterTeensy = true,
// the other with masterTeensy = false. The master sends updates with
//
// - 8 sync bytes of 255,
// - dial reading
// - the framecount.
// 
// Both teensy's read the "big button state" which Kiwiburners could
// use to turn the sensorium on and off.
//
// This could all be done much better, but nothing like a time constraint
// to force to make something work!
//
//
// Originally modified from:
// PlazINT  -  Fast Plasma Generator using Integer Math Only
// Edmund "Skorn" Horn
// March 4,2013
// Version 1.0 adapted for OctoWS2811Lib (tested, working...)


#include <OctoWS2811.h>
#include <FastRunningMedian.h>

//OctoWS2811 Defn. Stuff
//#define COLS_LEDs 60  // all of the following params need to be adjusted for screen size
//#define ROWS_LEDs 16  // LED_LAYOUT assumed 0 if ROWS_LEDs > 8
#define LEDS_PER_STRIP 127 //(COLS_LEDs * (ROWS_LEDs / 6))
#define DEBUG false

DMAMEM int displayMemory[LEDS_PER_STRIP*6];
int drawingMemory[LEDS_PER_STRIP*6];
const int config = WS2811_GRB | WS2811_800kHz;
OctoWS2811 leds(LEDS_PER_STRIP, displayMemory, drawingMemory, config);
unsigned long frameCount=25500;  // arbitrary seed to calculate the three time displacement variables t,t2,t3

// 1. update LEDS_PER_STRIP

// master
const int bigButtonAndControlPin = A3;
const int modeSelectorPin = A4;
const int masterPin = A5;
// master teensy reads big button, slave reads commands from control pin.
boolean masterTeensy = true;
//boolean masterTeensy = false;

// button to determine display
#define SHOW_PLASMA 1
#define INACTIVE 0
#define BUTTON_THRESHOLD 100

int displayState = INACTIVE; // 0 means nothing displayed

int bigButtonState;        // the current reading from the input pin
int lastBigButtonState = LOW; // the previous reading from the input pin
// the following variables are long's because the time, measured in miliseconds,
// will quickly become a bigger number than can be stored in an int.
//long lastDebounceTime = 0;  // the last time the output pin was toggled
//long debounceDelay = 5;    // the debounce time; increase if the output flickers

long lastDialTime = 0;  // the last time the output pin was toggled
long dialDelay = 500;    // the debounce time; increase if the output flickers
int dialSpeed = 10;


//Byte val 2PI Cosine Wave, offset by 1 PI 
//supports fast trig calcs and smooth LED fading/pulsing.
uint8_t const cos_wave[256] PROGMEM =  
{0,0,0,0,1,1,1,2,2,3,4,5,6,6,8,9,10,11,12,14,15,17,18,20,22,23,25,27,29,31,33,35,38,40,42,
45,47,49,52,54,57,60,62,65,68,71,73,76,79,82,85,88,91,94,97,100,103,106,109,113,116,119,
122,125,128,131,135,138,141,144,147,150,153,156,159,162,165,168,171,174,177,180,183,186,
189,191,194,197,199,202,204,207,209,212,214,216,218,221,223,225,227,229,231,232,234,236,
238,239,241,242,243,245,246,247,248,249,250,251,252,252,253,253,254,254,255,255,255,255,
255,255,255,255,254,254,253,253,252,252,251,250,249,248,247,246,245,243,242,241,239,238,
236,234,232,231,229,227,225,223,221,218,216,214,212,209,207,204,202,199,197,194,191,189,
186,183,180,177,174,171,168,165,162,159,156,153,150,147,144,141,138,135,131,128,125,122,
119,116,113,109,106,103,100,97,94,91,88,85,82,79,76,73,71,68,65,62,60,57,54,52,49,47,45,
42,40,38,35,33,31,29,27,25,23,22,20,18,17,15,14,12,11,10,9,8,6,6,5,4,3,2,2,1,1,1,0,0,0,0
};


//Gamma Correction Curve
uint8_t const exp_gamma[256] PROGMEM =
{0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,3,3,3,3,3,
4,4,4,4,4,5,5,5,5,5,6,6,6,7,7,7,7,8,8,8,9,9,9,10,10,10,11,11,12,12,12,13,13,14,14,14,15,15,
16,16,17,17,18,18,19,19,20,20,21,21,22,23,23,24,24,25,26,26,27,28,28,29,30,30,31,32,32,33,
34,35,35,36,37,38,39,39,40,41,42,43,44,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,
61,62,63,64,65,66,67,68,70,71,72,73,74,75,77,78,79,80,82,83,84,85,87,89,91,92,93,95,96,98,
99,100,101,102,105,106,108,109,111,112,114,115,117,118,120,121,123,125,126,128,130,131,133,
135,136,138,140,142,143,145,147,149,151,152,154,156,158,160,162,164,165,167,169,171,173,175,
177,179,181,183,185,187,190,192,194,196,198,200,202,204,207,209,211,213,216,218,220,222,225,
227,229,232,234,236,239,241,244,246,249,251,253,254,255
};

const int numDialReadings = 4;
int dialReadings[numDialReadings];      // the readings from the analog input
int dialIndex = 0;                  // the index of the current reading
int dialTotal = 0;                  // the running total
int dialAverage = 0;                // the average
FastRunningMedian<unsigned int, 5, 50> dialReadingsMedian;

float readDial() {
  int theValue = analogRead(modeSelectorPin);
  dialReadingsMedian.addValue(theValue);
  
  theValue = dialReadingsMedian.getMedian();

  // subtract the last reading:
  dialTotal = dialTotal - dialReadings[dialIndex];        
  dialReadings[dialIndex] = theValue;
  
  // add the reading to the total:
  dialTotal = dialTotal + dialReadings[dialIndex];      
  dialIndex++;

  if (dialIndex >= numDialReadings) dialIndex = 0;                          

  // calculate the average:
  dialAverage = dialTotal / numDialReadings;
  return dialAverage; 
  
}

void setup()
{
  delay(100);
  Serial.begin(38400);
  Serial.write('\n');
  Serial.println("---------------------------------");
  Serial.println("Starting Sensorium, version 1");
  
  Serial1.begin(9600);

  if (masterTeensy) {
    Serial.println("Master mode");
    pinMode(masterPin, OUTPUT);
    digitalWrite(masterPin, HIGH);
    // initialize all the readings to 0:
    for (int thisReading = 0; thisReading < numDialReadings; thisReading++)
      dialReadings[thisReading] = 0;          
  } else {
    Serial.println("Slave mode");   
  }

  Serial.println("---------------------------------");

  pinMode(bigButtonAndControlPin, INPUT);
  pinMode(modeSelectorPin, INPUT);
 
  pinMode(13, OUTPUT);

  leds.begin();
  leds.show();
}

void clearPixels() {
    for (uint8_t y = 0; y < 8; y++) {
       for (uint8_t x = 0; x < LEDS_PER_STRIP ; x++) {
          int pixelIndex = (y * LEDS_PER_STRIP) + x;
          leds.setPixel(pixelIndex, 0);
        }
    }
}

int circleRows = 19;
int maxRowLength = 72;
// format is { teensy led pin, reversed?, num leds }
const int rowLayout[][3] = {
  { 0, 0, 36, }, {0, 1, 46},
  { 1, 0, 51  }, {1, 1, 57},
  { 2, 0, 60  }, {2, 1, 65},  
  { 3, 0, 69 },
  { 4, 0, 70 },
  { 5, 0, 72 },
  { 6, 0, 72 },
  { 7, 0, 72 },
  { 8, 0, 71 },
  { 9, 0, 68 },
  { 10, 0, 65 }, {10, 1, 61 },
  { 11, 0, 56 }, {11, 1, 52 },
  { 12, 0, 43 }, {12, 1, 38 },
};

int getPixelIndex(int row, int col) {
  boolean reverse = false;
  int offset = 0, offsetRow = 0, actualRow = 0, rowLength = 0, diff = 0, col_offset=0;

  if (row >= circleRows) return -1;
  rowLength = rowLayout[row][2];
  col_offset = (maxRowLength - rowLength) / 2;
  if (col < col_offset || col > maxRowLength - col_offset) return -1;

  actualRow = rowLayout[row][0];
  reverse = rowLayout[row][1];

  
  offsetRow = row - 1;
  while(offsetRow >= 0 && rowLayout[offsetRow][0] == actualRow) {
    offset += rowLayout[offsetRow][2];
    offsetRow --;
  }
  
  if (masterTeensy) { actualRow -= 7; }
  
  diff = maxRowLength - rowLength;

  if (DEBUG) {
    Serial.print(row);
    Serial.print(" ");
    Serial.print(col);  
    Serial.print(" actualRow ");
    Serial.print(actualRow);
    Serial.print(" offset ");
    Serial.print(offset);
    Serial.print("\n");
  }
  
  if (reverse == 1) {
    return offset + (rowLength - (col - col_offset)) + actualRow*LEDS_PER_STRIP;
  } else {
    return offset + (col - col_offset) + actualRow*LEDS_PER_STRIP;
  }
    
}


void plasma() {
  frameCount += (dialSpeed/18.0) + 1;
  uint16_t t = fastCosineCalc((42 * frameCount)/100);  //time displacement - fiddle with these til it looks good...
  uint16_t t2 = fastCosineCalc((25 * frameCount)/100); 
  uint16_t t3 = fastCosineCalc((38 * frameCount)/100);
  uint16_t pixelIndex = 0;

  uint8_t startRow = 0;
  uint8_t endRow = 10;
  if (masterTeensy) { startRow += 10; endRow += 9; }

  for (uint8_t y = startRow; y < endRow; y++) {
    pixelIndex = getPixelIndex(y, 0);
    if (DEBUG) {
      Serial.print("pixelIndex y x i ");
      Serial.print(y);
      Serial.print(" ");
      Serial.print(0);
      Serial.print(" ");
      Serial.print(pixelIndex);
      Serial.print("\n");
    }
    for (uint8_t x = 0; x < maxRowLength ; x++) {
      pixelIndex = getPixelIndex(y, x);
      if (pixelIndex < 0) continue;
      //Calculate 3 seperate plasma waves, one for each color channel
      uint8_t r = fastCosineCalc(((x << 3) + (t >> 1) + fastCosineCalc((t2 + (y << 3)))));
      uint8_t g = fastCosineCalc(((y << 3) + t + fastCosineCalc(((t3 >> 2) + (x << 3)))));
      uint8_t b = fastCosineCalc(((y << 3) + t2 + fastCosineCalc((t + x + (g >> 2)))));
      //uncomment the following to enable gamma correction
      r=pgm_read_byte_near(exp_gamma+r);  
      g=pgm_read_byte_near(exp_gamma+g);
      b=pgm_read_byte_near(exp_gamma+b);
      r *= max(0.05,(dialSpeed / 140.0));
      g *= max(0.05,(dialSpeed / 140.0));
      b *= max(0.05,(dialSpeed / 140.0));          
      leds.setPixel(pixelIndex, ((r << 16) | (g << 8) | b));
      // pixelIndex += left2Right; optimisation!
    }
  }
  digitalWrite(13, HIGH);
  leds.show();  // not sure if this function is needed  to update each frame
  digitalWrite(13, LOW);  
}



void loop()
{
    if (displayState == SHOW_PLASMA) {
      plasma();
    } else {
      delay(100);
    }
  // read the state of the switch into a local variable:
  int reading = digitalRead(bigButtonAndControlPin);

  if (masterTeensy) {
    // check to see if you just pressed the button
    // (i.e. the input went from LOW to HIGH),  and you've waited
    // long enough since the last press to ignore any noise:  

    // If the switch changed, due to noise or pressing:
    /*if (reading != lastBigButtonState) {
      // reset the debouncing timer
      lastDebounceTime = millis();
    }
    if (!reading) {
      Serial.println("beep");
    }*/ 

    if (1) { //(millis() - lastDebounceTime) > debounceDelay) {
      // whatever the reading is at, it's been there for longer
      // than the debounce delay, so take it as the actual current state:

      // if the button state has changed:
      if (reading != bigButtonState) {
        bigButtonState = reading;
        // only toggle the LED if the new button state is HIGH
        if (bigButtonState == LOW) {
          if (DEBUG) Serial.println("Button pushed.");
          if (displayState == INACTIVE) {
            displayState = SHOW_PLASMA;
            if (DEBUG) Serial.println("Write low!");
            digitalWrite(masterPin, LOW);
          } else {
            clearPixels();
            leds.show();
            displayState = INACTIVE;
            if (DEBUG) Serial.println("Write high!");
            digitalWrite(masterPin, HIGH);
          }
        }
      }
    }
    
    if (masterTeensy && (millis() - lastDialTime) > dialDelay) {
      dialSpeed = readDial();
      Serial.println(dialSpeed);
      Serial1.write(255);
      Serial1.write(255);
      Serial1.write(255);
      Serial1.write(255);
      Serial1.write(255);
      Serial1.write(255);
      Serial1.write(255);
      Serial1.write(255);
      Serial1.write((uint8_t) dialSpeed);
      unsigned long val = frameCount;
      byte b[4];
      b[3] = (byte )((val >> 24) & 0xff);
      b[2] = (byte )((val >> 16) & 0xff);
      b[1] = (byte )((val >> 8) & 0xff);
      b[0] = (byte )(val & 0xff);

      unsigned long val2;
      memcpy(&val2, b, sizeof(unsigned long));
      Serial.print("Sent frameCount == ");
      Serial.println(val2);
      
      for (int i=0; i<4; ++i) {
        Serial1.write(b[i]);
      }
      lastDialTime = millis();
    }
  } else {
    if (Serial1.available() >= 13) {
      int data = Serial1.read();
      int sync = 0;
      while ((char)data == 255) {
        sync++;
        while (!Serial1.available() ) {};
        data = Serial1.read();
      }
      if (sync >= 7) {     
        dialSpeed = data;
        byte b[4];
        int data_count = 0;
        while (data_count < 4) {
           if (Serial1.available()) {
             data = Serial1.read();
             b[data_count] = data;
             data_count += 1;
           }
        }
        unsigned long val;
        memcpy(&val,b,sizeof(unsigned long));
        
        if (DEBUG) {
          Serial.print("Got dialspeed == ");
          Serial.print(dialSpeed);
          Serial.print(" frameCount == ");
          Serial.print(val);
          Serial.print(" drift=");        
          Serial.println(val - frameCount);
          }
        frameCount = val;
      }
    }
    // Slave mode
    if (bigButtonState != reading) {
      bigButtonState = reading;
      if (bigButtonState == LOW) {
        if (DEBUG) Serial.println("Read low");
        displayState = SHOW_PLASMA;
 
      } else {
        if (DEBUG) Serial.println("Read high");
        clearPixels();
        leds.show();
        displayState = INACTIVE;

      } 
    }
  }
 
  // save the reading.  Next time through the loop,
  // it'll be the lastButtonState:
  lastBigButtonState = reading;

}

inline uint8_t fastCosineCalc( uint16_t preWrapVal)
{
  uint8_t wrapVal = (preWrapVal % 255);
  if (wrapVal<0) wrapVal=255+wrapVal;
  return (pgm_read_byte_near(cos_wave+wrapVal)); 
}
