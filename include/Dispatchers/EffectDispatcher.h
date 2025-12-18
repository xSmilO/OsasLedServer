#pragma once
#include <LedController.h>
#include <inttypes.h>

namespace EffectDispatcher {
void dispatch(LedController *lc, const uint8_t *data,
              const uint32_t &dataLength);
}
