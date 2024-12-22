#include "LedController.h"


void LedController::Static(uint8_t r, uint8_t g, uint8_t b) {
    for (int i = 0; i < NUM_LEDS; ++i) {
        pPixels[i].setId(i);
        pPixels[i].setColor((float)r, (float)g, (float)b);
    }

    Pixel::sendData(pPixels, pDev, _numLeds);
    return;
}
