#include "FastLED.h"


#define NUM_LEDS 62
#define DATA_PIN 3
#define BAUD_RATE 115200
#define BRIGHTNESS 180

// HEADERS
const uint16_t START_HEADER = 0xFFEF;
const uint16_t END_HEADER = 0xFFEE;

CRGB newState[NUM_LEDS];
CRGB currentState[NUM_LEDS];

uint8_t id = 0;
uint8_t red = 0;
uint8_t green = 0;
uint8_t blue = 0;
int leds_setted = 0;
float speed = 0.1;
unsigned long lastUpdate;
unsigned long elapsedTime;

void lerp() {
  for(int i=0; i<NUM_LEDS; ++i) {
    uint8_t dR = newState[i].red - currentState[i].red;
    uint8_t dG = newState[i].green - currentState[i].green;
    uint8_t dB = newState[i].blue - currentState[i].blue;

    currentState[i].lerp16(newState[i], 0.4);
  }
}

void overflowErr() {
  for(int i=0; i<NUM_LEDS; ++i) {
    currentState[i].setRGB(30,30,30);
  }

  FastLED.show();
}

void setGreen() {
  for(int i=0; i<NUM_LEDS; ++i) {
    currentState[i].setRGB(0,255,0);
  }

  FastLED.show();
}

void setLeds(uint8_t* data, int dataSize) {
  for(int i=0; i<dataSize; i+=4) {
    currentState[(int) data[i]].setRGB(data[i+1], data[i+2], data[i+3]);
  }
}

void setup() {
  // sanity check delay - allows reprogramming if accidently blowing power w/leds
  delay(3000);
  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(currentState, NUM_LEDS);  // GRB ordering is typical
  FastLED.setBrightness(BRIGHTNESS);

  //initialize data
  for(int i=0; i<NUM_LEDS; ++i) {
    currentState[i] = CRGB(0,0,0);
    newState[i] = CRGB(0,0,0);
  }

  Serial.begin(BAUD_RATE);
  Serial.setTimeout(1000);
}

uint8_t mainBuf[256] = {0};
bool startHeaderReaded = false;
bool endHeaderReaded = false;
int bufSize = 256;
int bytesRead = 0;
int byteOffset = 0;
int startHeaderIdx = -1;
int endHeaderIdx = -1; 
uint16_t headerPtr;
uint8_t* bufPtr = nullptr;
uint8_t* led_data = new uint8_t[NUM_LEDS];
unsigned int ledDataSize = 0;
unsigned int remainingBytes = 0;
String msg = "";

int temp = 0;

void loop() {
  msg = "";
  // memset(mainBuf, 0, bufSize);
  // temp = Serial.readBytes(mainBuf, bufSize);
  // Serial.write(mainBuf, temp);
  if (Serial.available() > 0) {
    if(bytesRead < bufSize) {
      bytesRead += Serial.readBytes((uint8_t*)(mainBuf + bytesRead), (bufSize - bytesRead));
    } else {
      bytesRead = 0;
    }

    if(bytesRead > 1) {
      while(byteOffset < bytesRead - 1) {
        headerPtr = mainBuf[byteOffset] | (mainBuf[byteOffset + 1] << 8);
        if(!startHeaderReaded) {
          // find START_HEADER
          endHeaderReaded = false;
          if(headerPtr == START_HEADER) {
            startHeaderReaded = true;
            startHeaderIdx = byteOffset;
          }
        } else {
          // find END_HEADER
          if(headerPtr == END_HEADER) {
            endHeaderReaded = true;
            endHeaderIdx = byteOffset;
          }
        }
        byteOffset++;
      }
    }

    if(endHeaderReaded) {
      // copy data to led_data
      endHeaderReaded = false;
      startHeaderReaded = false;
      memset(led_data, 0, NUM_LEDS);
      ledDataSize = endHeaderIdx - (startHeaderIdx + 2);
      if(ledDataSize < 0)
      ledDataSize = 0;
      memcpy(led_data, (mainBuf + 2 + startHeaderIdx), ledDataSize);

      // i need to memmove the rest of main buffer until the end header because i' losing data bro!!!
      remainingBytes = bufSize - endHeaderIdx;
      memmove(mainBuf, &mainBuf[endHeaderIdx], remainingBytes);
      // setLeds(led_data, ledDataSize);
      // memset(mainBuf, 0, bufSize);
      // reset variables
      bytesRead = 0;
      endHeaderIdx = -1;
      startHeaderIdx = -1;
      byteOffset = 0;
    }
    if(endHeaderIdx != -1) {
      msg += "memcpy(led_data, (mainBuf + ";
      msg += String(startHeaderIdx + 2);
      msg += " ), ";
      msg += String(ledDataSize);
      msg += ")    ";
      msg +=  String(startHeaderIdx) + " " + String(endHeaderIdx);
      msg += "Led data size ";
      msg += String(ledDataSize);

      msg += "\n";

      // endHeaderIdx = -1;
      // startHeaderIdx = -1;
    }
    // Serial.write(msg.c_str(), msg.length());
    // Serial.write(mainBuf, bytesRead);

    // FastLED.show();
    Serial.write(led_data, ledDataSize);
  }
}
