#include "hwy/highway.h"
#include <stdio.h>
#include <vector>
#include <stdint.h>
#include <numeric>


HWY_BEFORE_NAMESPACE();
namespace HWY_NAMESPACE {
namespace hn = hwy::HWY_NAMESPACE;

#if HWY_TARGET == HWY_RVV

hn::Vec<hn::ScalableTag<uint16_t, 1>> WidenMulAcc(
        hn::Vec<hn::ScalableTag<uint8_t, 0>> sum,
        hn::Vec<hn::ScalableTag<uint8_t, 0>> a,
        hn::Vec<hn::ScalableTag<uint8_t, 0>> b,
        hn::Mask<hn::ScalableTag<uint8_t, 0>> mask) {

    const hn::ScalableTag<uint16_t, 1> dout;
    const hn::ScalableTag<uint8_t, 0> din;
    
    return hn::MaskedMulAddOr(
            hn::PromoteTo(dout, sum),
            hn::PromoteMaskTo(dout, din, mask),
            hn::PromoteTo(dout, a),
            hn::PromoteTo(dout, b),
            hn::PromoteTo(dout, sum));
}
hn::Vec<hn::ScalableTag<uint32_t, 2>> WidenMulAcc(
        hn::Vec<hn::ScalableTag<uint16_t, 1>> sum,
        hn::Vec<hn::ScalableTag<uint16_t, 1>> a,
        hn::Vec<hn::ScalableTag<uint16_t,1>> b,
        hn::Mask<hn::ScalableTag<uint16_t, 1>> mask) {

    const hn::ScalableTag<uint32_t, 2> dout;
    const hn::ScalableTag<uint16_t, 1> din;
    
    return hn::MaskedMulAddOr(
            hn::PromoteTo(dout, sum),
            hn::PromoteMaskTo(dout, din, mask),
            hn::PromoteTo(dout, a),
            hn::PromoteTo(dout, b),
            hn::PromoteTo(dout, sum));
}



size_t varint_decode_hwy(const uint8_t* HWY_RESTRICT input, size_t length, uint32_t* HWY_RESTRICT output) {
    using D8  = hn::ScalableTag<uint8_t>;
    const D8 d8;
    using V8  = hn::Vec<D8>;
    using M8  = hn::Mask<D8>;

    using D16M2 = hn::ScalableTag<uint16_t, 1>;
    const D16M2 d16;
    using V16M2 = hn::Vec<D16M2>;
    using M16M2 = hn::Mask<D16M2>;

    using D32M4 = hn::ScalableTag<uint32_t, 2>;
    const D32M4 d32;
    using V32M4 = hn::Vec<D32M4>;
    using M32M4 = hn::Mask<D32M4>;

    size_t processed = 0;

    V8 thresh = hn::Set(d8, static_cast<uint8_t>(0x7F));
    // number of currently active lanes
    size_t N8 = hn::Lanes(d8);
    while (length > 0) {
        size_t num_viable_bytes = (length > N8) ? N8 : length;
        M8 mask_viable_bytes = hn::FirstN(d8, length);
        V8 in = hn::LoadN(d8, input, num_viable_bytes);
        // mask set if element has termination bit (MSB==0) set
        M8 termination_mask = hn::MaskedLe(mask_viable_bytes, in, thresh);

        size_t num_varints = hn::CountTrue(d8, termination_mask);

        // fast path, no continuation bits (MSB==1) set
        if (num_varints == num_viable_bytes) {
            // widen uint8 to uint32
            V32M4 in32 = hn::PromoteTo(d32, in);

            hn::StoreN(in32, d32, output, num_varints);

            output += num_varints;
            length -= num_varints;
            input  += num_varints;
            processed += num_varints;
        }
        else {
            M8 mask_first_num_varints_8 = hn::FirstN(d8, num_varints);

            V8 input_1d = hn::ShiftRightBytes<1>(d8, in);
            V8 input_2d = hn::ShiftRightBytes<1>(d8, input_1d);

            // every byte after a termination byte is as first byte
            V8 input_1u = hn::ShiftLeftBytes<1>(d8, in);
            M8 mask_first_bytes = hn::Le(input_1u, thresh);

            // compress the slided input data vectors with the first_byte mask.
            // That way we get the second, third, fourth and fifth byte after each first byte.
            V8 first_bytes  = hn::Compress(in, mask_first_bytes);
            V8 second_bytes = hn::Compress(input_1d, mask_first_bytes);

            M8 mask_second_bytes = hn::Gt(first_bytes, thresh);
            mask_second_bytes    = hn::And(mask_second_bytes, mask_first_num_varints_8);
            M8 mask_third_bytes  = hn::And(mask_second_bytes, hn::Gt(second_bytes, thresh));

            M16M2 mask_second_bytes_16 = hn::PromoteMaskTo(d16, d8, mask_second_bytes);

            // remove continuation bits (bit 7) from payload bytes
            V8 b1 = hn::And(first_bytes, thresh);
            V8 b2 = hn::And(second_bytes, thresh);

            // Build result in 16-bit first (fits 14 bits for 2-byte varints)
            // b1: bits 0-6 b2: bits 7-13 (shift by 7, i.e., multiply by 128)
            V16M2 result12_16 = WidenMulAcc(b1, hn::Set(d8, 128), b2, mask_second_bytes_16);

            // Compute byte counts for each varint length
            size_t count2 = hn::CountTrue(d8, mask_second_bytes);
            size_t count3 = hn::CountTrue(d8, mask_third_bytes);

            // Total bytes = sum of bytes per varint
            size_t number_of_bytes = num_varints + count2 + count3;

            if (count3 == 0) {
                V32M4 result12 = hn::PromoteTo(d32, result12_16);

                StoreN(result12, d32, output, num_varints);
                output += num_varints;
            }
            else {
            V8 input_3d = hn::ShiftRightBytes<1>(d8, input_2d);
            V8 input_4d = hn::ShiftRightBytes<1>(d8, input_3d);

            V8 third_bytes  = hn::Compress(input_2d, mask_first_bytes);
            V8 fourth_bytes = hn::Compress(input_3d, mask_first_bytes);

            M8 mask_fourth_bytes = hn::And(mask_third_bytes, hn::Gt(third_bytes, thresh));
            M8 mask_fifth_bytes  = hn::And(mask_fourth_bytes, hn::Gt(fourth_bytes, thresh));

            size_t count4 = hn::CountTrue(d8, mask_fourth_bytes);
            size_t count5 = hn::CountTrue(d8, mask_fifth_bytes);
        
            number_of_bytes += count4;

            V8 b3 = hn::And(third_bytes, thresh);
            V8 b4 = hn::And(fourth_bytes, thresh);

            // b3: bits 0-6 b4: bits 7-13 (shift by 7, i.e., multiply by 128)
            V16M2 result34_16 = WidenMulAcc(b3, hn::Set(d8, 128), b4, mask_fourth_bytes);


            // shift result12 left by 14 (multiply with 16384) and add it to result34
            V32M4 result_1234 = WidenMulAcc(result12_16, hn::Set(d16, 16384), result34_16, hn::PromoteMaskTo(d16, d8, mask_third_bytes));

            if (count5 > 0) {
                number_of_bytes += count5;

                V8 fifth_bytes = hn::Compress(input_4d, mask_first_bytes);

                M32M4 mask_fifth_bytes_32 = hn::PromoteMaskTo(d32, d8, mask_fifth_bytes);

                V8 b5 = hn::And(fifth_bytes, thresh);

                // b5: bits 28-31 (shift by 28)
                // vwmaccu cannot be used as the scalar shift value is too large.
                V32M4 b5_32 = hn::PromoteTo(d32, b5);
                V32M4 b5_mul = hn::ShiftLeft<28>(b5_32);
                result_1234 = hn::MaskedAddOr(result_1234, mask_fifth_bytes_32, result_1234, b5_mul);

            }
            // Store decoded varints
            StoreN(result_1234, d32, output, num_varints);
            output +=  num_varints;
            }
            length -= number_of_bytes;
            input += number_of_bytes;
            processed += num_varints;
        }
    } // while
    return processed;
}

#endif // HWY_TARGET == HWY_RVV

#if HWY_TARGET != HWY_RVV


hn::Vec<hn::ScalableTag<uint16_t>> WidenMulAccTo16(
        hn::Vec<hn::ScalableTag<uint8_t>> a, 
        hn::Vec<hn::ScalableTag<uint8_t>> b, 
        hn::Vec<hn::ScalableTag<uint16_t>> sum0, 
        hn::Vec<hn::ScalableTag<uint16_t>>& sum1, 
        hn::Mask<hn::ScalableTag<uint8_t>> mask) {

    using D8  = hn::ScalableTag<uint8_t>;
    using D8h = hn::Half<D8>;
    const D8h d8h;
    
    using D16 = hn::ScalableTag<uint16_t>;
    const D16 d16;
    using V16 = hn::Vec<D16>;
    using M16 = hn::Mask<D16>;

    V16 a16_lo = hn::PromoteLowerTo(d16, a);
    V16 a16_up = hn::PromoteUpperTo(d16, a);
    V16 b16_lo = hn::PromoteLowerTo(d16, b);
    V16 b16_up = hn::PromoteUpperTo(d16, b);
    M16 mask16_lo = hn::PromoteMaskTo(d16, d8h, hn::LowerHalfOfMask(d8h, mask));
    M16 mask16_up = hn::PromoteMaskTo(d16, d8h, hn::UpperHalfOfMask(d8h, mask));

    sum1 = hn::MaskedMulAddOr(sum1, mask16_up, a16_up, b16_up, sum1);
    return hn::MaskedMulAddOr(sum0, mask16_lo, a16_lo, b16_lo, sum0);
}

hn::Vec<hn::ScalableTag<uint32_t>> WidenMulAccTo32 (
        hn::Vec<hn::ScalableTag<uint16_t>> a, 
        hn::Vec<hn::ScalableTag<uint16_t>> b, 
        hn::Vec<hn::ScalableTag<uint32_t>> sum0, 
        hn::Vec<hn::ScalableTag<uint32_t>>& sum1, 
        hn::Mask<hn::ScalableTag<uint16_t>> mask) {

    using D16  = hn::ScalableTag<uint16_t>;
    using D16h = hn::Half<D16>;
    const D16h d16h;

    using D32 = hn::ScalableTag<uint32_t>;
    const D32 d32;
    using V32 = hn::Vec<D32>;
    using M32 = hn::Mask<D32>;
    
    V32 a32_lo = hn::PromoteLowerTo(d32, a);
    V32 a32_up = hn::PromoteUpperTo(d32, a);
    V32 b32_lo = hn::PromoteLowerTo(d32, b);
    V32 b32_up = hn::PromoteUpperTo(d32, b);
    M32 mask32_lo = hn::PromoteMaskTo(d32, d16h, hn::LowerHalfOfMask(d16h, mask));
    M32 mask32_up = hn::PromoteMaskTo(d32, d16h, hn::UpperHalfOfMask(d16h, mask));

    sum1 = hn::MaskedMulAddOr(sum1, mask32_up, a32_up, b32_up, sum1);
    return hn::MaskedMulAddOr(sum0, mask32_lo, a32_lo, b32_lo, sum0);
}
    
size_t varint_decode_hwy(const uint8_t* HWY_RESTRICT input, size_t length, uint32_t* HWY_RESTRICT output) {
    using D8  = hn::ScalableTag<uint8_t>;
    const D8 d8;
    using V8  = hn::Vec<D8>;
    using M8  = hn::Mask<D8>;
    // used to name upper/lower halfs of masks
    using D8h = hn::Half<D8>;
    const D8h d8h;

    using D16 = hn::ScalableTag<uint16_t>;
    const D16 d16;
    using V16 = hn::Vec<D16>;
    // used to name upper/lower halfs of masks
    using D16h = hn::Half<D16>;
    const D16h d16h;

    using D32 = hn::ScalableTag<uint32_t>;
    const D32 d32;
    using V32 = hn::Vec<D32>;
    using M32 = hn::Mask<D32>;

    size_t processed = 0;

    V8 thresh = hn::Set(d8, static_cast<uint8_t>(0x7F));
    // number of lanes
    size_t N8 = hn::Lanes(d8);

    while (length >= N8) {
        V8 in = hn::LoadU(d8, input);
        // mask set if element has termination bit (MSB==0) set
        M8 termination_mask = hn::Le(in, thresh);

        size_t num_varints = hn::CountTrue(d8, termination_mask);

        // fast path, no continuation bits (MSB==1) set
        if (num_varints == N8) {
            // widen uint8 to uint32
            V32 in32_lolo = hn::PromoteLowerTo(d32, hn::PromoteLowerTo(d16, in));
            V32 in32_loup = hn::PromoteUpperTo(d32, hn::PromoteLowerTo(d16, in));
            V32 in32_uplo = hn::PromoteLowerTo(d32, hn::PromoteUpperTo(d16, in));
            V32 in32_upup = hn::PromoteUpperTo(d32, hn::PromoteUpperTo(d16, in));

            hn::StoreU(in32_lolo, d32, output);
            hn::StoreU(in32_loup, d32, output +   N8/4);
            hn::StoreU(in32_uplo, d32, output + 2*N8/4);
            hn::StoreU(in32_upup, d32, output + 3*N8/4);

            output += num_varints;
            length -= N8;
            input  += N8;
            processed += num_varints;
        }
        else {
            M8 mask_first_num_varints_8 = hn::FirstN(d8, num_varints);
            
            V8 input_1d = hn::ShiftRightBytes<1>(d8, in);
            V8 input_2d = hn::ShiftRightBytes<1>(d8, input_1d);

            // every byte after a termination byte is a first byte
            V8 input_1u = hn::ShiftLeftBytes<1>(d8, in);
            M8 mask_first_bytes = hn::Le(input_1u, thresh);

            // compress the slided input data vectors with the first_byte mask.
            // That way we get the second, third, fourth, fifth byte after each first byte.
            V8 first_bytes  = hn::Compress(in, mask_first_bytes);
            V8 second_bytes = hn::Compress(input_1d, mask_first_bytes);

            M8 mask_second_bytes = hn::Gt(first_bytes, thresh);
            mask_second_bytes    = hn::And(mask_second_bytes, mask_first_num_varints_8);
            M8 mask_third_bytes  = hn::And(mask_second_bytes, hn::Gt(second_bytes, thresh));

            // remove contiuation bits (bit 7) from payload bytes
            V8 b1 = hn::And(first_bytes, thresh);
            V8 b2 = hn::And(second_bytes, thresh);

            //Build result in 16-bit first (fits 14bits for 2-byte varints)
            // b1: bits 0-6 b2: bits 7-13 (shift by 7, i.e., multiply by 128)
            V16 b1_16_lo = hn::PromoteLowerTo(d16, b1);
            V16 b1_16_up = hn::PromoteUpperTo(d16, b1);

            V16 result12_16_lo = WidenMulAccTo16(
                    b2, 
                    hn::Set(d8, 128), 
                    b1_16_lo, 
                    b1_16_up, 
                    mask_second_bytes);

            V16 result12_16_up = b1_16_up;

            // Compute byte counts for each varint length
            size_t count2 = hn::CountTrue(d8, mask_second_bytes);
            size_t count3 = hn::CountTrue(d8, mask_third_bytes);

            // Total bytes = sum of bytes per varint
            size_t number_of_bytes = num_varints + count2 + count3;

            if (count3 == 0) {
                V32 result12_lolo = hn::PromoteLowerTo(d32, result12_16_lo);
                V32 result12_loup = hn::PromoteUpperTo(d32, result12_16_lo);
                V32 result12_uplo = hn::PromoteLowerTo(d32, result12_16_up);
                V32 result12_upup = hn::PromoteUpperTo(d32, result12_16_up);

                StoreN(result12_lolo, d32, output, num_varints);
                StoreN(result12_loup, d32, output +   N8/4, (num_varints <=   N8/4)? 0 : (num_varints -   N8/4));
                StoreN(result12_uplo, d32, output + 2*N8/4, (num_varints <= 2*N8/4)? 0 : (num_varints - 2*N8/4));
                StoreN(result12_upup, d32, output + 3*N8/4, (num_varints <= 3*N8/4)? 0 : (num_varints - 3*N8/4));

                output += num_varints;
            }
            else {
                V8 input_3d = hn::ShiftRightBytes<1>(d8, input_2d);
                V8 input_4d = hn::ShiftRightBytes<1>(d8, input_3d);

                V8 third_bytes  = hn::Compress(input_2d, mask_first_bytes);
                V8 fourth_bytes = hn::Compress(input_3d, mask_first_bytes);

                M8 mask_fourth_bytes = hn::And(mask_third_bytes,  hn::Gt(third_bytes, thresh));
                M8 mask_fifth_bytes  = hn::And(mask_fourth_bytes, hn::Gt(fourth_bytes, thresh));

                size_t count4 = hn::CountTrue(d8, mask_fourth_bytes);
                size_t count5 = hn::CountTrue(d8, mask_fifth_bytes);

                number_of_bytes += count4;

                V8 b3 = hn::And(third_bytes,  thresh);
                V8 b4 = hn::And(fourth_bytes, thresh);

                // b3: bits 0-6 b4: bits 7-13 (shift by 7, i.e., multiply by 128)
                V16 b3_16_lo = hn::PromoteLowerTo(d16, b3);
                V16 b3_16_up = hn::PromoteUpperTo(d16, b3);

                V16 result34_16_lo = WidenMulAccTo16(
                        b4, 
                        hn::Set(d8, 128), 
                        b3_16_lo, 
                        b3_16_up,
                        mask_fourth_bytes);

                V16 result34_16_up = b3_16_up;

                // result34: bits 0-13 result 12: bits 14-27 (shift by 14, i.e., multiply by 16384)
                // shift result 12 left by 14 (multiply with 16384) and add it to result34
                V32 result12_32_lolo = hn::PromoteLowerTo(d32, result12_16_lo); 
                V32 result12_32_loup = hn::PromoteUpperTo(d32, result12_16_lo); 

                V32 result1234_32_lolo = WidenMulAccTo32(
                        result34_16_lo,
                        hn::Set(d16, 16384),
                        result12_32_lolo,
                        result12_32_loup,
                        hn::PromoteMaskTo(d16, d8h, hn::LowerHalfOfMask(d8h, mask_third_bytes)));

                V32 result1234_32_loup = result12_32_loup; 

                V32 result12_32_uplo = hn::PromoteLowerTo(d32, result12_16_up); 
                V32 result12_32_upup = hn::PromoteUpperTo(d32, result12_16_up); 

                V32 result1234_32_uplo = WidenMulAccTo32(
                        result34_16_up,
                        hn::Set(d16, 16384),
                        result12_32_uplo,
                        result12_32_upup,
                        hn::PromoteMaskTo(d16, d8h, hn::UpperHalfOfMask(d8h, mask_third_bytes)));

                V32 result1234_32_upup = result12_32_upup; 


                if (count5 > 0) {
                    number_of_bytes += count5;
                    V8 fifth_bytes = hn::Compress(input_4d, mask_first_bytes);

                    V8 b5 = hn::And(fifth_bytes, thresh);

                    // b5: bits 28-31 (shift by 28)
                    // WidenMulAccTo32 can not be used because 2<<28 > uint16_max
                    V32 b5_32_lolo = hn::ShiftLeft<28>(hn::PromoteLowerTo(d32, hn::PromoteLowerTo(d16, b5))); 
                    M32 mask_fifth_bytes_32_lolo = hn::PromoteMaskTo(d32, d16h, hn::LowerHalfOfMask(d16h, hn::PromoteMaskTo(d16, d8h, hn::LowerHalfOfMask(d8h, mask_fifth_bytes))));
                    result1234_32_lolo = hn::MaskedAddOr(
                            result1234_32_lolo,
                            mask_fifth_bytes_32_lolo,
                            result1234_32_lolo,
                            b5_32_lolo);

                    V32 b5_32_loup = hn::ShiftLeft<28>(hn::PromoteUpperTo(d32, hn::PromoteLowerTo(d16, b5))); 
                    M32 mask_fifth_bytes_32_loup = hn::PromoteMaskTo(d32, d16h, hn::UpperHalfOfMask(d16h, hn::PromoteMaskTo(d16, d8h, hn::LowerHalfOfMask(d8h, mask_fifth_bytes))));
                    result1234_32_loup = hn::MaskedAddOr(
                            result1234_32_loup,
                            mask_fifth_bytes_32_loup,
                            result1234_32_loup,
                            b5_32_loup);

                    V32 b5_32_uplo = hn::ShiftLeft<28>(hn::PromoteLowerTo(d32, hn::PromoteUpperTo(d16, b5))); 
                    M32 mask_fifth_bytes_32_uplo = hn::PromoteMaskTo(d32, d16h, hn::LowerHalfOfMask(d16h, hn::PromoteMaskTo(d16, d8h, hn::UpperHalfOfMask(d8h, mask_fifth_bytes))));
                    result1234_32_uplo = hn::MaskedAddOr(
                            result1234_32_uplo,
                            mask_fifth_bytes_32_uplo,
                            result1234_32_uplo,
                            b5_32_uplo);

                    V32 b5_32_upup = hn::ShiftLeft<28>(hn::PromoteUpperTo(d32, hn::PromoteUpperTo(d16, b5))); 
                    M32 mask_fifth_bytes_32_upup = hn::PromoteMaskTo(d32, d16h, hn::UpperHalfOfMask(d16h, hn::PromoteMaskTo(d16, d8h, hn::UpperHalfOfMask(d8h, mask_fifth_bytes))));
                    result1234_32_upup = hn::MaskedAddOr(
                            result1234_32_upup,
                            mask_fifth_bytes_32_upup,
                            result1234_32_upup,
                            b5_32_upup);

                }
                // Store decoded varints
                StoreN(result1234_32_lolo, d32, output, num_varints);
                StoreN(result1234_32_loup, d32, output +   N8/4, (num_varints <=   N8/4)? 0 : (num_varints -   N8/4));
                StoreN(result1234_32_uplo, d32, output + 2*N8/4, (num_varints <= 2*N8/4)? 0 : (num_varints - 2*N8/4));
                StoreN(result1234_32_upup, d32, output + 3*N8/4, (num_varints <= 3*N8/4)? 0 : (num_varints - 3*N8/4));
                output += num_varints;
            }
            length -= number_of_bytes;
            input  += number_of_bytes;
            processed += num_varints;
        }
    } // while length >= N8
    // tail handling
    while (length > 0) {
        uint32_t result = 0;
        uint8_t shift = 0;

        while(1) {
            uint8_t byte = *input;
            ++input;
            --length;
            result |= uint32_t(byte & 0x7f) << shift;
            shift += 7;
            if(!(byte & 0x80) || length == 0) {
                break;
            }
        }
        *output = result;
        ++output;
        ++processed;
    }
    return processed;
}
#endif // HWY_TARGET != HWY_RVV

} // namespace HWY_NAMESPACE
HWY_AFTER_NAMESPACE();

extern "C" {
    size_t call_varint_decode_hwy(const uint8_t* HWY_RESTRICT input, size_t length, uint32_t* HWY_RESTRICT output) {
        return HWY_STATIC_DISPATCH(varint_decode_hwy)(input, length, output);
    }
}

