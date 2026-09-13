#ifdef __cplusplus
extern "C"
{
#endif

    #include <stdint.h>
    #include <stdio.h>

    size_t varint_decode_arm(const uint8_t *input, size_t length, uint32_t *output);
    size_t varint_decode_arm_early_branch(const uint8_t *input, size_t length, uint32_t *output);
#ifdef __cplusplus
}
#endif
