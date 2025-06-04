#include "FastLED.h"

#define NUM_LEDS 62
#define DATA_PIN 3
#define BAUD_RATE 115200
#define BRIGHTNESS 180
#define HEADER1 0xAA
#define HEADER2 0x55
#define FRAME_DATA_SIZE (NUM_LEDS * 4)
#define CHUNK_SIZE 64


CRGB newState[NUM_LEDS];
CRGB currentState[NUM_LEDS];

// TIME
const uint8_t REFRESH_RATE = 1000 / 30;
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
uint8_t chunk[CHUNK_SIZE];
uint16_t frameIndex = 0;
uint8_t chunkPos = 0;

void debugColor(uint8_t r, uint8_t g, uint8_t b) {
  for (uint16_t i = 0; i < NUM_LEDS; ++i) {
    currentState[i].setRGB(r, g, b);
  }

  FastLED.show();
}

void updateLeds() {
  for (uint16_t i = 0; i < NUM_LEDS; ++i) {
    currentState[i].r = lerp8by8(currentState[i].r, newState[i].r, 32);  // 32 = 12.5% toward target
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
        // debugColor(0, 0, 255);
        if (byte == HEADER1) {
          state = WAIT_FOR_HEADER2;
          Serial.write('1');
        }
        break;

      case WAIT_FOR_HEADER2:
        if (byte == HEADER2) {
          state = READ_FRAME;
          frameIndex = 0;
          chunkPos = 0;
        } else
          state = WAIT_FOR_HEADER1;
        break;

      case READ_FRAME:
        chunk[chunkPos++] = byte;
        Serial.write(chunkPos);
        // debugColor(chunkPos, 0,0);
        if (chunkPos >= CHUNK_SIZE || frameIndex + chunkPos >= FRAME_DATA_SIZE) {
          // readed whole chunk
          memcpy(frameBuffer + frameIndex, chunk, chunkPos);
          // debugColor(0,0,255);
          frameIndex += chunkPos;
          // Serial.write('K');
          Serial.write(69);
          // Serial.flush();
          chunkPos = 0;
        }


        if (frameIndex >= FRAME_DATA_SIZE) {
          // whole frame is read
          debugColor(0, 200, 0);
          processFrame(frameBuffer);
          state = WAIT_FOR_HEADER1;
        }

        break;
    }
  }

  elapsedTime = millis();

  if (elapsedTime - lastUpdate >= REFRESH_RATE) {
    lastUpdate = elapsedTime;

    updateLeds();
  }

  // if (elapsedTime - lastReceivedData >= 200) {
  //   state = WAIT_FOR_HEADER1;
  //   lastReceivedData = elapsedTime;
  // }
}

void processFrame(uint8_t* buffer) {
  // debugColor(0,255,0);

  if (buffer[0] != 0) {
    debugColor(255, 255, 0);
    return;
  }
  for (uint16_t i = 0; i < FRAME_DATA_SIZE; i += 4) {
    uint8_t id = buffer[i];

    if (id < NUM_LEDS) {
      newState[id].setRGB(buffer[i + 1], buffer[i + 2], buffer[i + 3]);
      // currentState[id].setRGB(255, 0, 0);
    }
  }
  // FastLED.show();
}