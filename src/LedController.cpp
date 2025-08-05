#include "LedController.h"
#include <cstring>

LedController::LedController(SerialPort *dev) {
    pDev = dev;
    queuePullThread = std::thread(&LedController::queuePuller, this);
}

void logBytes(uint8_t *buf, int bufSize, char *outLog) {
    memset(outLog, 0, 512);
    int length = 0;
    for(uint16_t i = 0; i < bufSize; ++i) {
        if(buf[i] == '\n') {
            length += sprintf(outLog + length, "\n");
        } else {
            length += sprintf(outLog + length, "0x%01x ", buf[i]);
        }
    }
}

void printBytes(uint8_t *buf, int bufSize) {
    int i = 0;
    while(i < bufSize) {
        printf("0x%01x ", buf[i]);
        ++i;
    }
    printf("\n");
}

void printMsg(uint8_t *buf, int bufSize) {
    int i = 0;
    printf("MSG: ");
    while(i < bufSize) {
        printf("%c", buf[i]);
        ++i;
    }
    printf("\n");
}

constexpr double REFRESH_RATE = 1000 / 60;

void LedController::queuePuller() {
    auto currentTime = std::chrono::high_resolution_clock::now();
    while(true) {

        if(currentEffect != nullptr) {
            if(currentEffect->update()) {
                delete currentEffect;
                currentEffect = nullptr;
            }
            Pixel::sendData(pPixels, pDev, _numLeds);
        }

        if(!effectsQueue.empty()) {
            delete currentEffect;
            currentEffect = effectsQueue.front();
            effectsQueue.pop();
        }
    }
}

void LedController::addEffect(Effect *effect) {
    effect->setNumLeds(_numLeds);
    effect->setOutputArr(pPixels);
    effectsQueue.push(effect);
}
