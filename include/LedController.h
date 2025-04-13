#pragma once
#include "SerialPort.hpp"
#include "Pixel.h"
#include "FunctionQueue.h"
#include <thread>

#include "Effect.h"
#include "Effects/StaticColor.h"

#define NUM_LEDS 62
#define LED_CALL_DELAY 5

class LedController {
private:
    uint32_t _numLeds = NUM_LEDS;
    Pixel* pPixels = new Pixel[NUM_LEDS];
    SerialPort* pDev = nullptr;
    std::queue<Effect*> effectsQueue;
    Effect* currentEffect = nullptr;
    std::thread queuePullThread;
public:
    LedController(SerialPort* dev);
    void queuePuller();
    void addEffect(Effect* effect);

};

