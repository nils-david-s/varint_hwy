#include <stddef.h>
#include <arm_sve.h>
#include <stdio.h>

void varint_decode_arm(const uint8_t *data, size_t length, uint32_t *output) {
    const uint64_t N8 = svcntb();
    while(length > 0) {
        svbool_t vl_pred = svwhilelt_b8_u32(0, length); 
        uint64_t vl = svcntp_b8(vl_pred,vl_pred);
        svbool_t vl_pred_u32_0 = svunpklo(svunpklo(vl_pred));
        svbool_t vl_pred_u32_1 = svunpkhi(svunpklo(vl_pred));
        svbool_t vl_pred_u32_2 = svunpklo(svunpkhi(vl_pred));
        svbool_t vl_pred_u32_3 = svunpkhi(svunpkhi(vl_pred));

        svuint8_t input = svld1_u8(vl_pred, data);

        // mask set when element has termiation bit (MSB==0) set
        svbool_t termination_mask = svcmple_n_u8(vl_pred, input, 0x7F);

        uint64_t num_varints = svcntp_b8(vl_pred, termination_mask);

        if (num_varints == vl) {
            //printf("in 1Byte case with num_varints: %zu\n", num_varints);
            //printf("Storemask 0: %zu\n", vl_0);
            //printf("Storemask 1: %zu\n", vl_1);
            //printf("Storemask 2: %zu\n", vl_2);
            //printf("Storemask 3: %zu\n", vl_3);
            // widen uint8 to uint32 and store 
            svst1_u32(vl_pred_u32_0, output         , svunpklo_u32(svunpklo_u16(input)));
            svst1_u32(vl_pred_u32_1, output + 1*N8/4, svunpkhi_u32(svunpklo_u16(input)));
            svst1_u32(vl_pred_u32_2, output + 2*N8/4, svunpklo_u32(svunpkhi_u16(input)));
            svst1_u32(vl_pred_u32_3, output + 3*N8/4, svunpkhi_u32(svunpkhi_u16(input)));

            output += num_varints;
            length -= vl;
            data +=  vl;
        }
        else {
            // predicate to limit to num_varints lanes
            svbool_t num_varints_pred = svwhilelt_b8_u32(0, num_varints);
            svbool_t num_varints_pred_u32_0 = svunpklo(svunpklo(num_varints_pred));
            svbool_t num_varints_pred_u32_1 = svunpkhi(svunpklo(num_varints_pred));
            svbool_t num_varints_pred_u32_2 = svunpklo(svunpkhi(num_varints_pred));
            svbool_t num_varints_pred_u32_3 = svunpkhi(svunpkhi(num_varints_pred));

            svuint8_t input_1d = svext_u8(input, svdup_u8(0), 1);
            svuint8_t input_2d = svext_u8(input, svdup_u8(0), 2);

            // every byte after a termination byte is a first byte
            // TODO: Maybe svext with vl-1 (wrapping)
            uint8_t idata[] = {255,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
            svuint8_t input_1u = svtbl_u8(input, svld1_u8(svptrue_b8(), idata));
            svbool_t mask_first_bytes = svcmple_n_u8(vl_pred, input_1u, 0x7F);
            
            // SVE: Compression is expensive for u8, only u32 and u64 have a
            //      specialised intruction for that.
            //      So we will compress an index list once and use it to gather 
            //      from the vectors.
            //      An alternatives:
            //      1:Do a loop to compress with something like svlastb_u8.
            //      2:No compression keep book of valid indices 
            //
            // compress the slided input data vectors with the first_byte mask.
            // That way we get the second, third, fourth, fifth byte after each first byte.
            // begin: compress
            svbool_t mask_first_bytes_u32_0 = svunpklo(svunpklo(mask_first_bytes));
            svbool_t mask_first_bytes_u32_1 = svunpkhi(svunpklo(mask_first_bytes));
            svbool_t mask_first_bytes_u32_2 = svunpklo(svunpkhi(mask_first_bytes));
            svbool_t mask_first_bytes_u32_3 = svunpkhi(svunpkhi(mask_first_bytes));

            svuint32_t cmp_indices_0 = svindex_u32(0     , 1);
            svuint32_t cmp_indices_1 = svindex_u32(1*N8/4, 1);
            svuint32_t cmp_indices_2 = svindex_u32(2*N8/4, 1);
            svuint32_t cmp_indices_3 = svindex_u32(3*N8/4, 1);

            cmp_indices_0 = svcompact_u32(mask_first_bytes_u32_0, cmp_indices_0);
            cmp_indices_1 = svcompact_u32(mask_first_bytes_u32_1, cmp_indices_1);
            cmp_indices_2 = svcompact_u32(mask_first_bytes_u32_2, cmp_indices_2);
            cmp_indices_3 = svcompact_u32(mask_first_bytes_u32_3, cmp_indices_3);

            // truncate the u32 indices to u8, sve is limited to 2048bits = 256 bytes fits into u8
            svuint8_t cmp_indices_u8_0 = svtbl(svreinterpret_u8_u32(cmp_indices_0), svindex_u8(0, 4));
            svuint8_t cmp_indices_u8_1 = svtbl(svreinterpret_u8_u32(cmp_indices_1), svindex_u8(0, 4));
            svuint8_t cmp_indices_u8_2 = svtbl(svreinterpret_u8_u32(cmp_indices_2), svindex_u8(0, 4));
            svuint8_t cmp_indices_u8_3 = svtbl(svreinterpret_u8_u32(cmp_indices_3), svindex_u8(0, 4));
            
            // combine them to a single svuint8_t by:
            // count valid indices in first part
            // shift second part by that amount
            // merge the two parts
            // Note: svext would would fit better but requires the shift_value to be const
            uint8_t shift_value      = svcntp_b32(vl_pred_u32_0, mask_first_bytes_u32_0);
            cmp_indices_u8_1         = svtbl_u8(cmp_indices_u8_1, svsub_n_u8_x(svptrue_b8(), svindex_u8(0,1), shift_value));
            svuint8_t cmp_indices_u8 = svsel_u8(svwhilelt_b8(0, shift_value), cmp_indices_u8_0, cmp_indices_u8_1); 

            shift_value             += svcntp_b32(vl_pred_u32_1, mask_first_bytes_u32_1);
            cmp_indices_u8_2         = svtbl_u8(cmp_indices_u8_2, svsub_n_u8_x(svptrue_b8(), svindex_u8(0,1), shift_value));
            cmp_indices_u8           = svsel_u8(svwhilelt_b8(0, shift_value), cmp_indices_u8, cmp_indices_u8_2); 

            shift_value             += svcntp_b32(vl_pred_u32_2, mask_first_bytes_u32_2);
            cmp_indices_u8_3         = svtbl_u8(cmp_indices_u8_3, svsub_n_u8_x(svptrue_b8(), svindex_u8(0,1), shift_value));
            cmp_indices_u8           = svsel_u8(svwhilelt_b8(0, shift_value), cmp_indices_u8, cmp_indices_u8_3); 
            // now cmp_indicies_u8 contains the lookup indices for compressing the nth_bytes 
            
            svuint8_t first_bytes  = svtbl_u8(input,    cmp_indices_u8);
            svuint8_t second_bytes = svtbl_u8(input_1d, cmp_indices_u8);
            // end: compress
    
            svbool_t mask_second_bytes = svcmpgt_n_u8(vl_pred, first_bytes, 0x7F);
            svbool_t mask_third_bytes  = svand_b_z(vl_pred, mask_second_bytes, svcmpgt_n_u8(vl_pred, second_bytes, 0x7F));

            // remove continuation bits (bit 7) from payload bytes
            svuint8_t b1 = svand_n_u8_x(vl_pred, first_bytes, 0x7F);
            svuint8_t b2 = svand_n_u8_x(vl_pred, second_bytes, 0x7F);

            // widen add(u16+u8) / widen mul acc only exist for even/odd lanes (u16 + u8_even/odd * const)
            // we would need to rearange the order of b2 first to use it or 
            // keep book of the indices
            svuint16_t b2_mul_widen_0 = svmul_n_u16_x(svunpklo(mask_second_bytes), svunpklo_u16(b2), 128);
            svuint16_t b2_mul_widen_1 = svmul_n_u16_x(svunpkhi(mask_second_bytes), svunpkhi_u16(b2), 128);

            svuint16_t result12_16_0 = svadd_u16_m(svunpklo(mask_second_bytes), svunpklo_u16(b1), b2_mul_widen_0);
            svuint16_t result12_16_1 = svadd_u16_m(svunpkhi(mask_second_bytes), svunpkhi_u16(b1), b2_mul_widen_1);

            // Compute byte counts for each varint length
            uint64_t count2 = svcntp_b8(num_varints_pred, mask_second_bytes);
            uint64_t count3 = svcntp_b8(num_varints_pred, mask_third_bytes);
           
            uint64_t number_of_bytes = num_varints + count2 + count3;

            if (count3 == 0) {
                svst1_u32(num_varints_pred_u32_0, output         , svunpklo_u32(result12_16_0));
                svst1_u32(num_varints_pred_u32_1, output + 1*N8/4, svunpkhi_u32(result12_16_0));
                svst1_u32(num_varints_pred_u32_2, output + 2*N8/4, svunpklo_u32(result12_16_1));
                svst1_u32(num_varints_pred_u32_3, output + 3*N8/4, svunpkhi_u32(result12_16_1));

                output += num_varints;
            }
            else {
                svuint8_t input_3d = svext_u8(input, svdup_u8(0), 3);
                svuint8_t input_4d = svext_u8(input, svdup_u8(0), 4);

                // Compress by reusing the compressed indicies from the last compression
                svuint8_t third_bytes  = svtbl_u8(input_2d, cmp_indices_u8);
                svuint8_t fourth_bytes = svtbl_u8(input_3d, cmp_indices_u8);

                svbool_t mask_fourth_bytes = svand_b_z(vl_pred, mask_third_bytes,  svcmpgt_n_u8(vl_pred, third_bytes,  0x7F));
                svbool_t mask_fifth_bytes  = svand_b_z(vl_pred, mask_fourth_bytes, svcmpgt_n_u8(vl_pred, fourth_bytes, 0x7F));
                
                uint64_t count4 = svcntp_b8(num_varints_pred, mask_fourth_bytes);
                uint64_t count5 = svcntp_b8(num_varints_pred, mask_fifth_bytes);
                
                number_of_bytes += count4;

                // remove continuation bits (bit 7) from payload bytes
                svuint8_t b3 = svand_n_u8_x(vl_pred, third_bytes,  0x7F);
                svuint8_t b4 = svand_n_u8_x(vl_pred, fourth_bytes, 0x7F);

                // b3: bits 0-6 b4: bits 7-13 (shift by 7, i.e., multiply by 128)
                svuint16_t b4_mul_widen_0 = svmul_n_u16_x(svunpklo(mask_fourth_bytes), svunpklo_u16(b4), 128);
                svuint16_t b4_mul_widen_1 = svmul_n_u16_x(svunpkhi(mask_fourth_bytes), svunpkhi_u16(b4), 128);

                svuint16_t result34_16_0 = svadd_u16_m(svunpklo(mask_fourth_bytes), svunpklo_u16(b3), b4_mul_widen_0);
                svuint16_t result34_16_1 = svadd_u16_m(svunpkhi(mask_fourth_bytes), svunpkhi_u16(b3), b4_mul_widen_1);

                // result34: bits 0-13 result 12: bits 14-27 (shift by 14, i.e., multiply by 16384)
                // shift result 12 left by 14 (multiply with 16384) and add it to result34
                svuint32_t result34_mul_widen_0 = svmul_n_u32_x(svunpklo(svunpklo(mask_third_bytes)), svunpklo_u32(result34_16_0), 16384);
                svuint32_t result34_mul_widen_1 = svmul_n_u32_x(svunpkhi(svunpklo(mask_third_bytes)), svunpkhi_u32(result34_16_0), 16384);
                svuint32_t result34_mul_widen_2 = svmul_n_u32_x(svunpklo(svunpkhi(mask_third_bytes)), svunpklo_u32(result34_16_1), 16384);
                svuint32_t result34_mul_widen_3 = svmul_n_u32_x(svunpkhi(svunpkhi(mask_third_bytes)), svunpkhi_u32(result34_16_1), 16384);

                svuint32_t result1234_32_0 = svadd_u32_m(svunpklo(svunpklo(mask_third_bytes)), svunpklo_u32(result12_16_0), result34_mul_widen_0);
                svuint32_t result1234_32_1 = svadd_u32_m(svunpkhi(svunpklo(mask_third_bytes)), svunpkhi_u32(result12_16_0), result34_mul_widen_1);
                svuint32_t result1234_32_2 = svadd_u32_m(svunpklo(svunpkhi(mask_third_bytes)), svunpklo_u32(result12_16_1), result34_mul_widen_2);
                svuint32_t result1234_32_3 = svadd_u32_m(svunpkhi(svunpkhi(mask_third_bytes)), svunpkhi_u32(result12_16_1), result34_mul_widen_3);

                if (count5 > 0) {
                    number_of_bytes += count5;
                    // Compress by reusing the compressed indicies from earlier
                    svuint8_t fifth_bytes = svtbl_u8(input_4d, cmp_indices_u8);

                    svuint8_t b5 = svand_n_u8_x(vl_pred, fifth_bytes, 0x7F);

                    // b5: bits 28-31 (shift by 28)
                    result1234_32_0 = svadd_u32_m(svunpklo(svunpklo(mask_fifth_bytes)), result1234_32_0, svlsl_n_u32_x(svunpklo(svunpklo(mask_fifth_bytes)), svunpklo_u32(svunpklo_u16(b5)), 28));
                    result1234_32_1 = svadd_u32_m(svunpkhi(svunpklo(mask_fifth_bytes)), result1234_32_1, svlsl_n_u32_x(svunpkhi(svunpklo(mask_fifth_bytes)), svunpkhi_u32(svunpklo_u16(b5)), 28));
                    result1234_32_2 = svadd_u32_m(svunpklo(svunpkhi(mask_fifth_bytes)), result1234_32_2, svlsl_n_u32_x(svunpklo(svunpkhi(mask_fifth_bytes)), svunpklo_u32(svunpkhi_u16(b5)), 28));
                    result1234_32_3 = svadd_u32_m(svunpkhi(svunpkhi(mask_fifth_bytes)), result1234_32_3, svlsl_n_u32_x(svunpkhi(svunpkhi(mask_fifth_bytes)), svunpkhi_u32(svunpkhi_u16(b5)), 28));
                }
                // Store decoded varints
                svst1_u32(num_varints_pred_u32_0, output         , result1234_32_0);
                svst1_u32(num_varints_pred_u32_1, output + 1*N8/4, result1234_32_1);
                svst1_u32(num_varints_pred_u32_2, output + 2*N8/4, result1234_32_2);
                svst1_u32(num_varints_pred_u32_3, output + 3*N8/4, result1234_32_3);
                output += num_varints;
            }
            length -= number_of_bytes;
            data   += number_of_bytes;
        }

    }
}
