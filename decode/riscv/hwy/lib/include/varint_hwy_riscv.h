#ifdef __cplusplus
extern "C"
{
#endif

    #include <stdint.h>
    #include <stdio.h>

    size_t call_varint_decode_hwy_riscv(const uint8_t *input, size_t length, uint32_t *output); 
    size_t call_varint_decode_hwy_riscv_wmacclib(const uint8_t *input, size_t length, uint32_t *output); 
    size_t call_varint_decode_hwy_riscv_nomask(const uint8_t *input, size_t length, uint32_t *output); 
    size_t call_varint_decode_hwy_riscv_mul_add(const uint8_t *input, size_t length, uint32_t *output); 
#ifdef __cplusplus
}
#endif
