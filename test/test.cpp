#include "libvarinthwy.h"
#include "hwy/highway.h"
#if HWY_ONCE
typedef struct
{
    uint64_t x, y, z;
} URand;
static URand randState = {123, 456, 789};

static uint64_t urand(URand *r)
{
    uint64_t x = r->x, y = r->y, z = r->z;
    r->x = y;
    r->y = z;
    z = x ^ (x << 13);
    z ^= z >> 7;
    z ^= y ^ (y << 17);
    r->z = z;
    return z;
}

static uint64_t bench_urand(void) { return urand(&randState); }

/* Random helper that respects an upper bound */
static inline uint32_t rand_in_range(uint32_t low, uint32_t high_exclusive)
{
    return low + (uint32_t)(bench_urand() % (high_exclusive - low));
}

/* Generate a value that will encode to exactly len bytes */
static inline uint32_t random_value_for_length(uint8_t len)
{
    switch (len)
    {
    case 1:
        return (uint32_t)(bench_urand() & 0x7F);
    case 2:
        return rand_in_range(1u << 7, 1u << 14);
    case 3:
        return rand_in_range(1u << 14, 1u << 21);
    case 4:
        return rand_in_range(1u << 21, 1u << 28);
    default: /* len == 5 */
        return ((uint32_t)bench_urand()) | (1u << 28);
    }
}

/* Pick a varint length according to the distribution weights */
static inline uint8_t pick_length(uint8_t max_len, const int w[5])
{
    int total = 0;
    for (uint8_t i = 0; i < max_len; ++i)
        total += w[i];

    uint32_t r = (uint32_t)(bench_urand() % total);
    int acc = 0;
    for (uint8_t i = 0; i < max_len; ++i)
    {
        acc += w[i];
        if (r < (uint32_t)acc)
            return (uint8_t)(i + 1);
    }
    return max_len;
}

/* Determine varint length by scanning bytes */
static size_t get_varint_length(const uint8_t *p, const uint8_t *end)
{
    size_t len = 0;
    while (p < end && len < 5)
    {
        len++;
        if (!(*p & 0x80)) /* no continuation bit = last byte */
            return len;
        p++;
    }
    return len;
}
int main(void) {
    const size_t N = 1567; // amount of numbers to generate
    const int weights[5] = {85, 5, 4, 3, 3};

    uint8_t *encoded_data = (uint8_t*) malloc(N * 5);
    uint32_t *decoded_data_portable = (uint32_t*) malloc(N * sizeof(uint32_t));
    uint32_t *decoded_data_specific = (uint32_t*) malloc(N * sizeof(uint32_t));
    uint32_t *original_data = (uint32_t*) malloc(N * sizeof(uint32_t));
    size_t decoded_count_portable;
    if (!original_data | !decoded_data_portable | !decoded_data_specific | !encoded_data) {
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

    // Decode using varint_decode_hwy_portable (available on all architectures)
    decoded_count_portable = call_varint_decode_hwy_portable(encoded_data, encoded_length, decoded_data_portable);
    printf("Portable: Decoded %zu intergers from %zu bytes\n\n", decoded_count_portable, encoded_length);
    // Validate portable
    int errors_portable = 0;
    for (size_t i = 0; i < N; ++i) {
        if (original_data[i] != decoded_data_portable[i]) {
            printf("Portable: ERROR at index %zu: expected: %u, got %u\n", i, original_data[i], decoded_data_portable[i]);
            errors_portable++;
            if (errors_portable >= 10) {
                printf("Portable: ... stopping after 10 erors)\n");
                break;
            }
        }
    }
    if (errors_portable == 0) {
        printf("Portable: SUCCESS: All %zu values decoded correctly\n", N);
    } else {
        printf("Portable: Failed: %d errors found\n", errors_portable);
    }
    
    // Decode using varint_decode_hwy_riscv or arm_sve implementation
    #if defined(RISCV)
    size_t decoded_count_specific;
    decoded_count_specific = call_varint_decode_hwy_riscv(encoded_data, encoded_length, decoded_data_specific);
    printf("\n\nSpecific(RISCV): Decoded %zu intergers from %zu bytes\n\n", decoded_count_specific, encoded_length);
    #elif defined(SVE2)
    size_t decoded_count_specific;
    decoded_count_specific = varint_decode_arm(encoded_data, encoded_length, decoded_data_specific);
    printf("\n\nSpecific(SVE2): Decoded %zu intergers from %zu bytes\n\n", decoded_count_specific, encoded_length);
    #endif
    // Validate varint_decode_hwy_riscv or arm_sve
    #if defined(RISCV) || defined(SVE2) 
    int errors_specific = 0;
    for (size_t i = 0; i < N; ++i) {
        if (original_data[i] != decoded_data_specific[i]) {
            printf("RISCV/SVE2: ERROR at index %zu: expected: %u, got %u\n", i, original_data[i], decoded_data_specific[i]);
            errors_specific++;
            if (errors_specific >= 10) {
                printf("RISCV/SVE2: ... stopping after 10 erors)\n");
                break;
            }
        }
    }
    if (errors_specific == 0) {
        printf("RISCV/SVE2: SUCCESS: All %zu values decoded correctly\n", N);
    } else {
        printf("RISCV/SVE2: Failed: %d errors found\n", errors_specific);
    }
    return errors_portable + errors_specific > 0 ? 1 : 0;
    #endif
}
#endif // HWY_ONCE

