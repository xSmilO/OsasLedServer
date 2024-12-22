#pragma once
#include "SerialPort.hpp"
#include "Pixel.h"
#include "FunctionQueue.h"
#include <thread>

#define NUM_LEDS 62
#define LED_CALL_DELAY 5

class LedController {
private:
    uint32_t _numLeds = NUM_LEDS;
    Pixel* pPixels = new Pixel[NUM_LEDS];
    SerialPort* pDev = nullptr;
    FunctionQueue callQueue;
    std::thread queuePullThread;
public:
    LedController(SerialPort* dev);
    void Static(uint8_t r, uint8_t g, uint8_t b);
    void queuePuller();

    template<typename Func, typename... Args>
    void addCall(Func f, Args... args);

};

template<typename Func, typename ...Args>
inline void LedController::addCall(Func f, Args ...args) {
    callQueue.push(f, this, args...);
}
