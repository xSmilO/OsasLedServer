#include "FastLED.h"

#define NUM_LEDS 62
#define DATA_PIN 4
#define BAUD_RATE 250000
#define BRIGHTNESS 180
#define HEADER1 0xAA
#define HEADER2 0x55
#define FRAME_DATA_SIZE (NUM_LEDS * 4)
#define CHUNK_SIZE 64

CRGB newState[NUM_LEDS];
CRGB currentState[NUM_LEDS];

// TIME
const uint8_t REFRESH_RATE = 1000 / 60;
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

float lerpColor(float a, float b, float t) {
    return a + (b - a) * t;
}
void updateLeds() {
    for(uint16_t i = 0; i < NUM_LEDS; ++i) {
        currentState[i].r = (uint8_t)lerpColor((float)currentState[i].r, (float)newState[i].r, 0.1);
        currentState[i].g = (uint8_t)lerpColor((float)currentState[i].g, (float)newState[i].g, 0.1);
        currentState[i].b = (uint8_t)lerpColor((float)currentState[i].b, (float)newState[i].b, 0.1);
    }
    FastLED.show();
}

void setup() {
    // sanity check delay - allows reprogramming if accidently blowing power w/leds
    delay(3000);
    FastLED.addLeds<WS2812B, DATA_PIN, GRB>(currentState, NUM_LEDS); // GRB ordering is typical
    FastLED.setBrightness(BRIGHTNESS);

    Serial.begin(BAUD_RATE);
    FastLED.clear();
    FastLED.show();
}

void loop() {
    while(Serial.available()) {
        lastReceivedData = millis();
        uint8_t byte = Serial.read();

        switch(state) {
        case WAIT_FOR_HEADER1:
            if(byte == HEADER1) {
                state = WAIT_FOR_HEADER2;
                Serial.write('1');
            }
            break;

        case WAIT_FOR_HEADER2:
            if(byte == HEADER2) {
                state = READ_FRAME;
                frameIndex = 0;
                chunkPos = 0;
            } else
                state = WAIT_FOR_HEADER1;
            break;

        case READ_FRAME:
            chunk[chunkPos++] = byte;
            Serial.write(chunkPos);
            if(chunkPos >= CHUNK_SIZE || frameIndex + chunkPos >= FRAME_DATA_SIZE) {
                memcpy(frameBuffer + frameIndex, chunk, chunkPos);
                frameIndex += chunkPos;
                chunkPos = 0;
            }

            if(frameIndex >= FRAME_DATA_SIZE) {
                // whole frame is read
                processFrame(frameBuffer);
                state = WAIT_FOR_HEADER1;
            }

            break;
        }
    }

    elapsedTime = millis();

    if(elapsedTime - lastUpdate >= REFRESH_RATE) {
        lastUpdate = elapsedTime;

        updateLeds();
    }

    if(elapsedTime - lastReceivedData >= 1000) {
        state = WAIT_FOR_HEADER1;
        lastReceivedData = elapsedTime;
    }
}

void processFrame(uint8_t *buffer) {
    for(uint16_t i = 0; i < FRAME_DATA_SIZE; i += 4) {
        uint8_t id = buffer[i];

        if(id < NUM_LEDS) {
            newState[id].setRGB(buffer[i + 1], buffer[i + 2], buffer[i + 3]);
        }
    }
}