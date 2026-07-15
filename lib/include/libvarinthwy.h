#ifndef LIBVARINTHWY_H
#define LIBVARINTHWY_H

#ifdef __cplusplus
extern "C"
{
#endif

    #include <stdint.h>
    #include <stdio.h>

    size_t vbyte_encode(const uint32_t *in, size_t length, uint8_t *bout);
    void call_varint_decode_hwy(const uint8_t *input, size_t length, uint32_t *output); 
    void varint_decode_arm(const uint8_t *input, size_t length, uint32_t *output);
#ifdef __cplusplus
}
#endif

#endif // LIBVARINTHWY_H
