#pragma once
#include <inttypes.h>

class EffectDispatcher {
  public:
    static void Dispatch(uint8_t *data, const uint32_t &dataLength);
};
