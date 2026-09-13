#include "hwy/highway.h"
#include <stdint.h>
HWY_BEFORE_NAMESPACE();
namespace HWY_NAMESPACE {
namespace hn = hwy::HWY_NAMESPACE;

#if HWY_TARGET == HWY_RVV

template <class DN, class VN = hn::VFromD<DN>,
         class DW = hn::Twice<DN>, class VW = hn::VFromD<DW>>
VW concat_with_d(DN dn, VN v) {
    const DW dw;
    return hn::Combine(dw, v, v);
}

template <class VN, class DN = hn::DFromV<VN>,
         class DW = hn::Twice<DN>, class VW = hn::VFromD<DW>>
VW concat_without_d(VN v) {
    const DW dw;
    return hn::Combine(dw, v, v);
}

void test_concat_with_d(const uint32_t* HWY_RESTRICT input, uint32_t* HWY_RESTRICT output) {
    using DW = hn::ScalableTag<uint32_t, -1>;
    const DW dw;
    using VW = hn::Vec<DW>;
    using DN = hn::Half<DW>;
    const DN dn;
    using VN = hn::Vec<DN>;

    VW a = hn::LoadN(dw, input, 2);
    VN a_lower = hn::LowerHalf(dn, a);
    VW b = concat_with_d(dn, a_lower);
    hn::StoreN(b, dw, output, hn::Lanes(dw));
}

void test_concat_without_d(const uint32_t* HWY_RESTRICT input, uint32_t* HWY_RESTRICT output) {
    using DW = hn::ScalableTag<uint32_t, -1>;
    const DW dw;
    using VW = hn::Vec<DW>;
    using DN = hn::Half<DW>;
    const DN dn;
    using VN = hn::Vec<DN>;
    using DW_AFTER = hn::Twice<hn::DFromV<VN>>;
    const DW_AFTER dw_after;
    using VW_AFTER = hn::Vec<DW_AFTER>;

    VW a = hn::LoadN(dw, input, 2);
    VN a_lower = hn::LowerHalf(dn, a);
    VW_AFTER b = concat_without_d(a_lower);
    hn::StoreN(b, dw_after, output, hn::Lanes(dw_after));
}

#endif // HWY_TARGET == HWY_RVV
} // namespace HWY_NAMESPACE
HWY_AFTER_NAMESPACE();

extern "C" {
    void call_test_concat_with_d(const uint32_t* HWY_RESTRICT input, uint32_t* HWY_RESTRICT output) {
        return HWY_STATIC_DISPATCH(test_concat_with_d)(input, output);
    }
    void call_test_concat_without_d(const uint32_t* HWY_RESTRICT input, uint32_t* HWY_RESTRICT output) {
        return HWY_STATIC_DISPATCH(test_concat_without_d)(input, output);
    }
}



