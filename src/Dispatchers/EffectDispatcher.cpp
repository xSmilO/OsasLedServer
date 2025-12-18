#include "Effects/StaticColor.h"
#include <Dispatchers/EffectDispatcher.h>

#define STATIC_EFFECT 0x1

namespace EffectDispatcher {
void dispatch(LedController *lc, const uint8_t *data, const uint32_t &dataLength) {
    // Effect flag
    switch (data[1]) {
    case STATIC_EFFECT:
        if (dataLength >= 5) {
            lc->addEffect(new StaticColor(data[2], data[3], data[4]));
        }
        break;
    }
}
} // namespace EffectDispatcher
