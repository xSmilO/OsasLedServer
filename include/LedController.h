#pragma once
#include "SerialPort.hpp"
#include "Pixel.h"

#define NUM_LEDS 62

class LedController {
private:
    uint32_t _numLeds = NUM_LEDS;
    Pixel* pPixels = new Pixel[NUM_LEDS];
    SerialPort* pDev = nullptr;
public:
    LedController(SerialPort* pDev) : pDev(pDev) {};
    void Static(uint8_t r, uint8_t g, uint8_t b);
};