#include <PacketRouter.h>
#include <Dispatchers/EffectDispatcher.h>

void PacketRouter::Route(uint8_t *data, const uint32_t &dataLength) {
    switch (data[0]) {
    case EFFECT_HEADER:
        EffectDispatcher::Dispatch(data, dataLength);
        break;
    }
}
