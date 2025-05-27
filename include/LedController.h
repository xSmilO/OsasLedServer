#pragma once
#include "FunctionQueue.h"
#include "Pixel.h"
#include "SerialPort.hpp"
#include <thread>

#include "Effect.h"
#include "Effects/StaticColor.h"

#define NUM_LEDS 62
#define LED_CALL_DELAY 5

class LedController {
  private:
    uint32_t _numLeds = NUM_LEDS;
    Pixel *pPixels = new Pixel[NUM_LEDS];
    SerialPort *pDev = nullptr;
    std::queue<Effect *> effectsQueue;
    Effect *currentEffect = nullptr;
    std::thread queuePullThread;
    bool _ledUpdated = false;

  public:
    LedController(SerialPort *dev);
    void queuePuller();
    void addEffect(Effect *effect);
};
