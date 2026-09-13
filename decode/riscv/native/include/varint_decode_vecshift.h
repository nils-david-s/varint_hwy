#ifdef __cplusplus
extern "C"
{
#endif

    #include <stdint.h>
    #include <stdio.h>

    size_t varint_decode_vecshift(const uint8_t *data, size_t length, uint32_t *output);
#ifdef __cplusplus
}
#endif

