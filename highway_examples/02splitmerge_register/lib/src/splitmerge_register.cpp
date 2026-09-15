#include "hwy/highway.h"
#include <stdint.h>
HWY_BEFORE_NAMESPACE();
namespace HWY_NAMESPACE {
namespace hn = hwy::HWY_NAMESPACE;

#if HWY_TARGET == HWY_RVV

template <class VN, class DN = hn::DFromV<VN>,
         class DW = hn::Twice<DN>, class VW = hn::VFromD<DW>>
VN square_mergesplit(VN a, VN& b) {
    const DW dw;
    const DN dn;
    VW combined = hn::Combine(dw, b, a);
    VW squared = hn::Mul(combined, combined);
    b = hn::UpperHalf(dn, squared);
    return hn::LowerHalf(dn, squared);
}

template <class VW ,class DW = hn::DFromV<VW>, 
          class DN = hn::Half<DW>, class VN = hn::VFromD<DN>>
VW square_splitmerge(VW a) {
    const DW dw;
    const DN dn;
    VN lower = hn::LowerHalf(dn, a);
    VN upper = hn::UpperHalf(dn, a);
    VN lower_squared = hn::Mul(lower, lower);
    VN upper_squared = hn::Mul(upper, upper);
    return hn::Combine(dw, upper_squared, lower_squared);
}

template <class VW>
VW square_whole(VW a) {
    return hn::Mul(a, a);
}

template <class VN>
VN square_part(VN a, VN& b) {
    b = hn::Mul(b, b);
    return hn::Mul(a, a);
}

void test_mergesplit(const uint32_t* HWY_RESTRICT input, uint32_t* HWY_RESTRICT output) {
    using DW = hn::ScalableTag<uint32_t, 1>;
    using DN = hn::Half<DW>;
    const DN dn;
    using VN = hn::Vec<DN>;

    VN a = hn::Load(dn, input);
    input += hn::Lanes(dn);
    VN b = hn::Load(dn, input);
    VN c = square_mergesplit(a, b);
    hn::Store(c, dn, output);
    output += hn::Lanes(dn);
    hn::Store(b, dn, output);
}

void test_splitmerge(const uint32_t* HWY_RESTRICT input, uint32_t* HWY_RESTRICT output) {
    using DW = hn::ScalableTag<uint32_t, 1>;
    const DW dw;
    using VW = hn::Vec<DW>;

    VW a = hn::Load(dw, input);
    VW b = square_splitmerge(a);
    hn::Store(b, dw, output);
}

void test_whole(const uint32_t* HWY_RESTRICT input, uint32_t* HWY_RESTRICT output) {
    using DW = hn::ScalableTag<uint32_t, 1>;
    const DW dw;
    using VW = hn::Vec<DW>;

    VW a = hn::Load(dw, input);
    VW b = square_whole(a);
    hn::Store(b, dw, output);
}
void test_part(const uint32_t* HWY_RESTRICT input, uint32_t* HWY_RESTRICT output) {
    using DW = hn::ScalableTag<uint32_t, 1>;
    using DN = hn::Half<DW>;
    const DN dn;
    using VN = hn::Vec<DN>;

    VN a = hn::Load(dn, input);
    input += hn::Lanes(dn);
    VN b = hn::Load(dn, input);
    VN c = square_part(a, b);
    hn::Store(c, dn, output);
    output += hn::Lanes(dn);
    hn::Store(b, dn, output);
}

#endif // HWY_TARGET == HWY_RVV
} // namespace HWY_NAMESPACE
HWY_AFTER_NAMESPACE();

extern "C" {
    void call_test_mergesplit(const uint32_t* HWY_RESTRICT input, uint32_t* HWY_RESTRICT output) {
        return HWY_STATIC_DISPATCH(test_mergesplit)(input, output);
    }
    void call_test_splitmerge(const uint32_t* HWY_RESTRICT input, uint32_t* HWY_RESTRICT output) {
        return HWY_STATIC_DISPATCH(test_splitmerge)(input, output);
    }
    void call_test_whole(const uint32_t* HWY_RESTRICT input, uint32_t* HWY_RESTRICT output) {
        return HWY_STATIC_DISPATCH(test_whole)(input, output);
    }
    void call_test_part(const uint32_t* HWY_RESTRICT input, uint32_t* HWY_RESTRICT output) {
        return HWY_STATIC_DISPATCH(test_part)(input, output);
    }
}



