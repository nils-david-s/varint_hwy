#include <stdint.h>
#include <stdio.h>

size_t varint_decode_scalar_tail(const uint8_t *input, int length, uint32_t *output)
{
    size_t processed = 0;
    while (length > 0) {
        uint32_t result = 0;
        uint8_t shift = 0;

        while(1) {
            uint8_t byte = *input;
            ++input;
            --length;
            result |= (uint32_t)(byte & 0x7f) << shift;
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
