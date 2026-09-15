#include "hwy/highway.h"
#include <stdint.h>
extern "C" {
    void call_splitmerge(const uint32_t* HWY_RESTRICT input, uint32_t* HWY_RESTRICT output);
    void call_mergesplit(const uint32_t* HWY_RESTRICT input, uint32_t* HWY_RESTRICT output);
    void call_whole(const uint32_t* HWY_RESTRICT input, uint32_t* HWY_RESTRICT output);
    void call_merge(const uint32_t* HWY_RESTRICT input, uint32_t* HWY_RESTRICT output);
}
