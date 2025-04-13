#pragma once
#include "Effect.h"

class StaticColor : public Effect {
    uint8_t _r, _g, _b;
public:
    StaticColor(uint8_t& r, uint8_t& g, uint8_t& b) : _r(r), _g(g), _b(b) {
    }

    bool update() {
        // printf("%d %d %d\n", _r, _g, _b);
        for (size_t i = 0; i < numLeds; ++i) {
            outputArr[i].setId(i);
            outputArr[i].setColor((float)_r, (float)_g, (float)_b);
        }
        return true;
    }
};