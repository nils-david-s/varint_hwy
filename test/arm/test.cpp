#include "hwy/highway.h"

#include <test_utils.h>
#include <varint_hwy_portable.h>
#include <varint_scalar.h>
#include <varint_encode.h>
#include <varint_arm.h>

#if HWY_ONCE
int main(void) {
    const size_t N = 1567; // amount of numbers to generate
    const int weights[5] = {85, 5, 4, 3, 3};
    uint8_t *encoded_data = (uint8_t*) malloc(N * 5);
    uint32_t *original_data = (uint32_t*) malloc(N * sizeof(uint32_t));
    bool success = true;

    if (!original_data | !encoded_data) {
        fprintf(stderr, "Failed to allocate memory\n");
        return 1;
    }
    
    size_t dist_counts[5] = {0};
    for (size_t i = 0; i < N; ++i) {
        uint8_t len = pick_length(5, weights);
        original_data[i] = random_value_for_length(len);
        dist_counts[len-1]++;
    }

    printf("Generated %zu values with distribution:\n", N);
    for (int i = 0; i < 5; ++i) {
        printf("  %d-byte: %zu (%.1f%%)\n", i+1, dist_counts[i], 100.0 * dist_counts[i] / N);
    }
    printf("\n");

    // Encode varints
    size_t encoded_length = vbyte_encode(original_data, N, encoded_data);
    printf("Encoded to %zu bytes (avg %.2f bytes/value)\n\n", encoded_length, (double)encoded_length / N);

    size_t encoded_dist[5] = {0};
    size_t encoded_count = 0;
    const uint8_t *p = encoded_data;
    const uint8_t *end = encoded_data + encoded_length;
    while (p < end) {
        size_t len = get_varint_length(p, end);
        if (len == 0 || len > 5) {
            break;
        }
        encoded_dist[len-1]++;
        encoded_count++;
        p += len;
    }
    // Verify encoded distribution
    printf("Encoded distribution verification:\n");
    for (int i = 0; i < 5; i++) {
        printf(" %d-byte: %zu (%.1f%%)\n", i+1, encoded_dist[i], 100.0 * encoded_dist[i] / encoded_count);
    }
    printf("\n");

    success &= test_decoder(encoded_data, encoded_length, original_data, N, "scalar_tail", varint_decode_scalar_tail);
    success &= test_decoder(encoded_data, encoded_length, original_data, N, "scalar", varint_decode_scalar);
    success &= test_decoder(encoded_data, encoded_length, original_data, N, "Highway portable", call_varint_decode_hwy_portable);
    success &= test_decoder(encoded_data, encoded_length, original_data, N, "Highway portable early branch", call_varint_decode_hwy_portable_early_branch);
    success &= test_decoder(encoded_data, encoded_length, original_data, N, "Highway portable nomask", call_varint_decode_hwy_portable_nomask);
    success &= test_decoder(encoded_data, encoded_length, original_data, N, "Highway portable wmacclib", call_varint_decode_hwy_portable_wmacclib);
    success &= test_decoder(encoded_data, encoded_length, original_data, N, "Arm", varint_decode_arm);
    success &= test_decoder(encoded_data, encoded_length, original_data, N, "Arm early branch", varint_decode_arm_early_branch);

    return success ? 0 : 1;
}
#endif // HWY_ONCE
