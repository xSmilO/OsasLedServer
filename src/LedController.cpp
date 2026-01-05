#include "LedController.h"
#include <chrono>
#include <cstring>
#include <stdafx.h>
#include <thread>

LedController::LedController(SerialPort *dev) {
    pDev = dev;
    queuePullThread = std::thread(&LedController::queuePuller, this);
}

void logBytes(uint8_t *buf, int bufSize, char *outLog) {
    memset(outLog, 0, 512);
    int length = 0;
    for (uint16_t i = 0; i < bufSize; ++i) {
        if (buf[i] == '\n') {
            length += sprintf(outLog + length, "\n");
        } else {
            length += sprintf(outLog + length, "0x%01x ", buf[i]);
        }
    }
}

void printBytes(uint8_t *buf, int bufSize) {
    int i = 0;
    while (i < bufSize) {
        printf("0x%01x ", buf[i]);
        ++i;
    }
    printf("\n");
}

void printMsg(uint8_t *buf, int bufSize) {
    int i = 0;
    printf("MSG: ");
    while (i < bufSize) {
        printf("%c", buf[i]);
        ++i;
    }
    printf("\n");
}

constexpr double REFRESH_RATE = 1000. / 60.;

void LedController::queuePuller() {
    using namespace std::chrono;

    const auto frameDuration = milliseconds((int) REFRESH_RATE);

    while (true) {
        auto startTime = high_resolution_clock::now();

        if (currentEffect != nullptr) {
            bool isFinished = currentEffect->update();

            Pixel::sendData(pPixels, pDev, _numLeds);
            if (isFinished) {
                printf("Effect finished\n");
                delete currentEffect;
                currentEffect = nullptr;
            }
        }

        if (!effectsQueue.empty() && currentEffect == nullptr) {
            currentEffect = effectsQueue.front();
            effectsQueue.pop();
        }

        auto endTime = high_resolution_clock::now();
        auto elapsedTime = duration_cast<milliseconds>(endTime - startTime);

        if(elapsedTime < frameDuration) {
            std::this_thread::sleep_for(frameDuration - elapsedTime);
        }
    }
}

void LedController::addEffect(Effect *effect) {
    effect->setNumLeds(_numLeds);
    effect->setOutputArr(pPixels);
    printf("Adding effect\n");

    if (effectsQueue.size() < 10)
        effectsQueue.push(effect);
}

void LedController::changeLedSpeed(uint8_t speed) {
    Pixel::setLerpSpeed(pDev, speed);
}
