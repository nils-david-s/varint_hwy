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
using Decoder = size_t (*)(const uint8_t*, size_t, uint32_t*);

bool test_decoder(uint8_t *encoded_data, size_t encoded_length, uint32_t *original_data, size_t original_count, const char* name, Decoder decoder) {
    printf("--------------------------------\nBEGIN: TEST: %s \n", name);
    size_t N = original_count;
    uint32_t *decoded_data = (uint32_t*) malloc(N * sizeof(uint32_t));
    size_t decoded_count;
    if (!decoded_data) {
        fprintf(stderr, "Failed to allocate memory\n");
        return 1;
    }
    decoded_count = decoder(encoded_data, encoded_length, decoded_data);
    printf("%s: Decoded %zu intergers from %zu bytes\n\n", name, decoded_count, encoded_length);
    // Validate
    int errors = 0;
    for (size_t i = 0; i < N; ++i) {
        if (original_data[i] != decoded_data[i]) {
            printf("%s: ERROR at index %zu: expected: %u, got %u\n", name, i, original_data[i], decoded_data[i]);
            errors++;
            if (errors >= 10) {
                printf("%s: ... stopping after 10 erors)\n", name);
                break;
            }
        }
    }
    free(decoded_data); 
    if (errors == 0) {
        printf("Portable: SUCCESS: All %zu values decoded correctly\n", N);
        printf("END: TEST: %s \n--------------------------------\n", name);
        return true;
    } else {
        printf("Portable: Failed: %d errors found\n", errors);
        printf("END: TEST: %s \n--------------------------------\n", name);
        return false;
    }
}

