#include "FastLED.h"

#define NUM_LEDS 62
#define DATA_PIN 4
#define BAUD_RATE 500000
#define BRIGHTNESS 180

#define HEADER_BYTE 0xAA

#define CMD_FRAME_DATA 0x10
#define CMD_SET_SETTINGS 0x20
#define CMD_GET_SETTINGS 0x30

#define SET_ID_LERP 0x01

#define FRAME_DATA_SIZE (NUM_LEDS * 4)
#define CHUNK_SIZE 64

CRGB newState[NUM_LEDS];
CRGB currentState[NUM_LEDS];

// TIME
const uint8_t REFRESH_RATE = 1000 / 60;
uint32_t elapsedTime = 0;
uint32_t lastUpdate = 0;
uint32_t lastReceivedData = 0;

enum State {
  WAIT_FOR_HEADER,
  WAIT_FOR_COMMAND,
  READ_LED_FRAME,
  READ_SET_TYPE,
  READ_SET_PAYLOAD
};

State state = WAIT_FOR_HEADER;

uint8_t frameBuffer[FRAME_DATA_SIZE];
uint8_t chunk[CHUNK_SIZE];
uint16_t frameIndex = 0;
uint8_t chunkPos = 0;
float lerpSpeed = 0.1;

uint8_t settingId = 0;
uint8_t settingBuffer[8];
uint8_t settingBytesRead = 0;
uint8_t settingTargetLen = 0;

float lerp(float a, float b) {
  return a + (b - a) * lerpSpeed;
}
void updateLeds() {
  for (uint16_t i = 0; i < NUM_LEDS; ++i) {
    currentState[i].r = (uint8_t)lerp((float)currentState[i].r, (float)newState[i].r);
    currentState[i].g = (uint8_t)lerp((float)currentState[i].g, (float)newState[i].g);
    currentState[i].b = (uint8_t)lerp((float)currentState[i].b, (float)newState[i].b);
  }
  FastLED.show();
}

void processFrame(uint8_t *buffer) {
  for (uint16_t i = 0; i < FRAME_DATA_SIZE; i += 4) {
    uint8_t id = buffer[i];

    if (id < NUM_LEDS) {
      newState[id].setRGB(buffer[i + 1], buffer[i + 2], buffer[i + 3]);
    }
  }
}

void applySetting() {
  switch (settingId) {
    case SET_ID_LERP:
      lerpSpeed = (float)settingBuffer[0] / 255.0f;
      break;
  }
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
      case WAIT_FOR_HEADER:
        if (byte == HEADER_BYTE) {
          state = WAIT_FOR_COMMAND;
          Serial.write('1');
          break;
        }
      case WAIT_FOR_COMMAND:
        if (byte == CMD_FRAME_DATA) {
          state = READ_LED_FRAME;
          frameIndex = 0;
          chunkPos = 0;

        } else if (byte == CMD_SET_SETTINGS) {
          state = READ_SET_TYPE;
        } else {
          state = WAIT_FOR_HEADER;
        }
        break;

      case READ_LED_FRAME:
        chunk[chunkPos++] = byte;
        Serial.write(chunkPos);
        if (chunkPos >= CHUNK_SIZE || frameIndex + chunkPos >= FRAME_DATA_SIZE) {
          memcpy(frameBuffer + frameIndex, chunk, chunkPos);
          frameIndex += chunkPos;
          chunkPos = 0;
        }

        if (frameIndex >= FRAME_DATA_SIZE) {
          // whole frame is read
          processFrame(frameBuffer);
          state = WAIT_FOR_HEADER;
        }

        break;

      case READ_SET_TYPE:
        settingId = byte;
        settingBytesRead = 0;

        switch(settingId) {
          case SET_ID_LERP: settingTargetLen = 1; break;
          default:
            state = WAIT_FOR_HEADER;
            return;
        }

        state = READ_SET_PAYLOAD;
        break;

      case READ_SET_PAYLOAD:
        settingBuffer[settingBytesRead++] = byte;

        if(settingBytesRead >= settingTargetLen) {
          applySetting();
          state = WAIT_FOR_HEADER;
        }
        break;
    }
  }

  elapsedTime = millis();

  if (elapsedTime - lastUpdate >= REFRESH_RATE) {
    lastUpdate = elapsedTime;

    updateLeds();
  }

  if (elapsedTime - lastReceivedData >= 1000) {
    state = WAIT_FOR_HEADER;
    lastReceivedData = elapsedTime;
  }
}