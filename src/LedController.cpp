#include "LedController.h"

LedController::LedController(SerialPort* dev) {
    pDev = dev;
    queuePullThread = std::thread(&LedController::queuePuller, this);
}

const uint16_t START_HEADER = 0xFFEF;

void printBytes(uint8_t* buf, int bufSize) {
    int i = 0;
    while (i < bufSize) {
        printf("%d: 0x%02x ", i, *(buf + i));
        // printf("%c", *(buf + i));
        ++i;
    }
    printf("\n");
}

void printMsg(uint8_t* buf, int bufSize) {
    int i = 0;
    while (i < bufSize) {
        printf("%c", *(buf + i));
        ++i;
    }
    printf("\n");
}

void findHeaders(uint8_t* buf, int bufSize) {
    int byteOffset = 0;
    uint16_t val;
    printf("Size: %d\n", bufSize);
    while (byteOffset < bufSize - 1) {
        val = buf[byteOffset] | (buf[byteOffset + 1] << 8);

        printf("0x%04x ", val);
        if (val == START_HEADER) {
            printf("FOUNDED!!!\n");
        }
        byteOffset++;
    }

    printf("\n");
}

void LedController::queuePuller() {
    int bufSize = 128;
    char* arduinoBuff = new char[bufSize];
    int bytesRecv = 0;
    while (true) {
        // Sleep(LED_CALL_DELAY);
        // Sleep(200);
        memset(arduinoBuff, 0, bufSize);
        bytesRecv = pDev->readSerialPort(arduinoBuff, bufSize);
        if (bytesRecv != 0) {
            printf("recv: %d\n", bytesRecv);
            printBytes((uint8_t*)arduinoBuff, bytesRecv);
            // printMsg((uint8_t*)arduinoBuff, bytesRecv);
            // printf("%s\n", arduinoBuff);
        }

        // if (effectsQueue.empty()) {
        //     continue;
        // }
        if (!effectsQueue.empty()) {
            delete currentEffect;
            currentEffect = effectsQueue.front();
            effectsQueue.pop();
        }

        if (currentEffect != nullptr)
            if (currentEffect->update()) {
                delete currentEffect;
                currentEffect = nullptr;
            }

        Pixel::sendData(pPixels, pDev, _numLeds);
    }
}

void LedController::addEffect(Effect* effect) {
    effect->setNumLeds(_numLeds);
    effect->setOutputArr(pPixels);
    effectsQueue.push(effect);
}
