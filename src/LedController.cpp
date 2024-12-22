#include "LedController.h"


LedController::LedController(SerialPort* dev) {
    pDev = dev;
    queuePullThread = std::thread(&LedController::queuePuller, this);
}

void LedController::Static(uint8_t r, uint8_t g, uint8_t b) {
    printf("%d %d %d\n", r, g, b);
    for (int i = 0; i < _numLeds; ++i) {
        pPixels[i].setId(i);
        pPixels[i].setColor((float)r, (float)g, (float)b);
    }

    Pixel::sendData(pPixels, pDev, _numLeds);
    return;
}

void LedController::queuePuller() {
    while (true) {
        Sleep(LED_CALL_DELAY);
        callQueue.execute();
    }
}
