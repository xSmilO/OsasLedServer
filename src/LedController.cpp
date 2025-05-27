#include "LedController.h"

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

void LedController::queuePuller() {
    unsigned int bufCap = 512;
    uint16_t bufSize = 0;
    unsigned int bytesRecv = 0;
    char arduinoBuff[bufCap];

    while(true) {
        // Sleep(LED_CALL_DELAY);

        // bytesRecv = pDev->readSerialPort(&arduinoBuff[bufSize], bufCap - bufSize);
        bytesRecv = pDev->readSerialPort(arduinoBuff, bufCap);

        if(bytesRecv != 0) {
            printMsg((uint8_t *)arduinoBuff, bytesRecv);
            bytesRecv = 0;
        }

        if(effectsQueue.empty()) {
            continue;
        }
        if(!effectsQueue.empty()) {
            delete currentEffect;
            currentEffect = effectsQueue.front();
            effectsQueue.pop();
        }

        if(currentEffect != nullptr) {
            if(currentEffect->update()) {
                delete currentEffect;
                currentEffect = nullptr;
            }
        }

        Pixel::sendData(pPixels, pDev, _numLeds);
    }
}

void LedController::addEffect(Effect *effect) {
    effect->setNumLeds(_numLeds);
    effect->setOutputArr(pPixels);
    effectsQueue.push(effect);
}
