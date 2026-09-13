#ifdef __cplusplus
extern "C"
{
#endif

    #include <stdint.h>
    #include <stdio.h>

    size_t vbyte_encode(const uint32_t *in, size_t length, uint8_t *bout);
#ifdef __cplusplus
}
#endif

