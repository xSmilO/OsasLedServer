#pragma once
#include <cstdint>
#include <inttypes.h>

#define EFFECT_HEADER 0xef

class PacketRouter {
  public:
    static void Route(uint8_t *data, const uint32_t &dataLength);
};
