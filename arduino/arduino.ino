#include "FastLED.h"

#define NUM_LEDS 62
#define DATA_PIN 3
#define BAUD_RATE 400000
#define BRIGHTNESS 180
#define HEADER1 0xAA
#define HEADER2 0x55
#define FRAME_DATA_SIZE (NUM_LEDS * 4)

// HEADERS
const char* START_HEADER = "OSAS_START";
const char* END_HEADER = "OSAS_END";
const uint8_t START_HEADER_LEN = 10;
const uint8_t END_HEADER_LEN = 8;

CRGB newState[NUM_LEDS];
CRGB currentState[NUM_LEDS];

// TIME
const uint8_t REFRESH_RATE = 1000 / 20;
uint32_t elapsedTime = 0;
uint32_t lastUpdate = 0;
uint32_t lastReceivedData = 0;


enum ReadState {
  WAIT_FOR_HEADER1,
  WAIT_FOR_HEADER2,
  READ_FRAME
};

ReadState state = WAIT_FOR_HEADER1;

uint8_t frameBuffer[FRAME_DATA_SIZE];
uint16_t frameIndex = 0;

void updateLeds() {
  for (uint16_t i = 0; i < NUM_LEDS; ++i) {
    currentState[i].r = lerp8by8(currentState[i].r, newState[i].r, 32); // 32 = 12.5% toward target
    currentState[i].g = lerp8by8(currentState[i].g, newState[i].g, 32);
    currentState[i].b = lerp8by8(currentState[i].b, newState[i].b, 32);
    // currentState[i].setRGB(newState[i].r, newState[i].g, newState[i].b);
  }
  FastLED.show();
}

void setup() {
  // sanity check delay - allows reprogramming if accidently blowing power w/leds
  delay(3000);
  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(currentState, NUM_LEDS);  // GRB ordering is typical
  FastLED.setBrightness(BRIGHTNESS);


  Serial.begin(BAUD_RATE);
  FastLED.clear();
  FastLED.show();
}

void loop() {
  while (Serial.available()) {
    lastReceivedData = millis();
    uint8_t byte = Serial.read();

    switch (state) {
      case WAIT_FOR_HEADER1:
        if (byte == HEADER1) {
          frameIndex = 0;
          state = WAIT_FOR_HEADER2;
          Serial.write("Header1 found");
        }
        break;
      case WAIT_FOR_HEADER2:
        if (byte == HEADER2) {
          frameIndex = 0;
          state = READ_FRAME;
          Serial.write("Header2 found");
        } else {
          state = WAIT_FOR_HEADER1;
        }
        break;
      case READ_FRAME:
        // Serial.write("Reading frame");
        frameBuffer[frameIndex++] = byte;
        if (frameIndex >= FRAME_DATA_SIZE) {
          processFrame(frameBuffer);
          state = WAIT_FOR_HEADER1;
        }
        break;
    }
  }

  elapsedTime = millis();

  if(elapsedTime - lastReceivedData >= 200 && state == READ_FRAME) {
    Serial.print("reset");
    state = WAIT_FOR_HEADER1;
    frameIndex = 0;
    lastReceivedData = elapsedTime;
  }

  if (elapsedTime - lastUpdate >= REFRESH_RATE) {
    lastUpdate = elapsedTime;
    updateLeds();
  }
}

void processFrame(uint8_t* buffer) {
  Serial.write("processing");
  for (uint16_t i = 0; i < FRAME_DATA_SIZE; i += 4) {
    uint8_t id = buffer[i];

    if (id < NUM_LEDS) {
      newState[id].setRGB(buffer[i+1], buffer[i+2], buffer[i+3]);
    }
  }

  // FastLED.show();
}