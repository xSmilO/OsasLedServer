#pragma once
#include "Pixel.h"

class Effect {
public:
    size_t numLeds;
    Pixel* outputArr;

    virtual bool update() = 0;
    virtual ~Effect() = default;
    void setNumLeds(size_t num) {
        numLeds = num;
    };

    void setOutputArr(Pixel* pixels) {
        outputArr = pixels;
    };

};
