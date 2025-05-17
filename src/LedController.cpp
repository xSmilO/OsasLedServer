#include "LedController.h"

LedController::LedController(SerialPort *dev) {
    pDev = dev;
    queuePullThread = std::thread(&LedController::queuePuller, this);
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
    unsigned int bufSize = 0;
    unsigned int bytesRecv = 0;
    char *arduinoBuff = new char[bufCap];
    while(true) {
        // Sleep(LED_CALL_DELAY);
        // Sleep(500);
        if(GetAsyncKeyState(0x43)) {
            printf("clear buf!\n");
            memset(arduinoBuff, 0, bufCap);
            bufSize = 0;
        }
        if(bufSize >= bufCap)
            bufSize = 0;

        bytesRecv = pDev->readSerialPort(&arduinoBuff[bufSize], bufCap - bufSize);
        if(bytesRecv != 0) {
            bufSize += bytesRecv;
            // printf("%s\n", arduinoBuff);
            printf("recv: %d\n", bytesRecv);
            printBytes((uint8_t *)arduinoBuff, bufSize);
            printMsg((uint8_t *)arduinoBuff, bufSize);
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

        if(currentEffect != nullptr)
            if(currentEffect->update()) {
                delete currentEffect;
                currentEffect = nullptr;
            }

        Pixel::sendData(pPixels, pDev, _numLeds);
    }
}

void LedController::addEffect(Effect *effect) {
    effect->setNumLeds(_numLeds);
    effect->setOutputArr(pPixels);
    effectsQueue.push(effect);
}
