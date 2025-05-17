#include "FastLED.h"


#define NUM_LEDS 62
#define DATA_PIN 3
#define BAUD_RATE 115200
#define BRIGHTNESS 180

// HEADERS
const char* START_HEADER = "OSAS_START";
const char* END_HEADER = "OSAS_END";
const uint8_t START_HEADER_LEN = 10;
const uint8_t END_HEADER_LEN = 8;

CRGB newState[NUM_LEDS];
CRGB currentState[NUM_LEDS];

// buffers
uint8_t mainBuf[512] = {0};
uint8_t dataBuf[256];

// headers
uint16_t startHeaderIdx = 0;
uint16_t endHeaderIdx = 0;
bool startHeaderFounded = false;
bool endHeaderFounded = false;

// sizes
uint16_t bytesRead = 0;
uint16_t bufCap = 512;

void setLeds(uint8_t* data, uint16_t& dataSize) {
  for(uint16_t i=0; i<dataSize; i+=4) {
    currentState[(uint8_t) data[i]].setRGB(data[i+1], data[i+2], data[i+3]);
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


// headerPtr = mainBuf[byteOffset] | (mainBuf[byteOffset + 1] << 8);
void searchHeaders(uint8_t* buf, uint16_t& bufSize) {
  uint16_t matchIdx = 0;

  for(uint16_t i = 0; i < bufSize; ++i) {
    if(!startHeaderFounded) {
      if(buf[i] == START_HEADER[matchIdx]) {
        matchIdx++;
      } else {
        matchIdx = 0;
      }

      if(matchIdx >= START_HEADER_LEN) {
        matchIdx = 0;
        startHeaderFounded = true;
        startHeaderIdx = i;
      }
    } else {
      if(buf[i] == END_HEADER[matchIdx]) {
        matchIdx++;
      } else {
        matchIdx = 0;
      }

      if(matchIdx >= END_HEADER_LEN) {
        matchIdx = 0;
        endHeaderFounded = true;
        endHeaderIdx = i - END_HEADER_LEN;
      }
    }
   
  }

  return;
}

void getData(uint8_t* buf, uint16_t& bufLen) {
  memcpy(dataBuf, &buf[startHeaderIdx + 1], endHeaderIdx - startHeaderIdx);

  //move data to start
  uint16_t chunkSize = START_HEADER_LEN + (endHeaderIdx - startHeaderIdx) + END_HEADER_LEN;
  memmove(buf, &buf[endHeaderIdx + END_HEADER_LEN + 1], bufLen - chunkSize);
  bufLen -= chunkSize;

  startHeaderFounded = false;
  endHeaderFounded = false;
  startHeaderIdx = 0;
  endHeaderIdx = 0;

  setLeds(dataBuf, chunkSize);
  return;
}

void loop() {
  // read bytes
  // add bytes to buffer
  // find headers
  // repeat

  while(Serial.available() > 0) {
    if(bytesRead >= bufCap) {
      bytesRead = 0;
    }
    bytesRead += Serial.readBytes(&mainBuf[bytesRead], bufCap - bytesRead);

    searchHeaders(mainBuf, bytesRead);
    // Serial.write(mainBuf, bytesRead);
    if(endHeaderFounded) {
      getData(mainBuf, bytesRead);
      
    }
  FastLED.show();
   
  }
}
