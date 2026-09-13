#include "hwy/highway.h"
#include <stdint.h>
extern "C" {
    void call_test_concat_with_d(const uint32_t* HWY_RESTRICT input, uint32_t* HWY_RESTRICT output);
    void call_test_concat_without_d(const uint32_t* HWY_RESTRICT input, uint32_t* HWY_RESTRICT output);
}
