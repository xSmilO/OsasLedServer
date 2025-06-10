#pragma once
#include "Effect.h"

class ColorWheel : public Effect {
  private:
    float _hue;
    float _speed;

  public:
    ColorWheel(float speed) : _speed(speed) {
    }

    bool update() {
        for(size_t i = 0; i < numLeds; ++i) {
            _hue += _speed;
            if(_hue > 360)
                _hue = 0;
            if(_hue < 0)
                _hue = 360;

            outputArr[i].HSVtoRGB(_hue, 90, 90);
            outputArr[i].setId(i);
        }

        return false;
    }
};