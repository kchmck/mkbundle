// See copyright notice in Copying.

#include <assert.h>
#include <inttypes.h>
#include <limits.h>
#include <stdbool.h>
#include <stdlib.h>

#include "sdnv.h"

#ifdef MKBUNDLE_TEST
#include "greatest.h"
#endif

// Allocate an SDNV to hold the number of bytes and point the pointer at it.
static void sdnv_init(sdnv_t **sdnv, size_t byte_count) {
    *sdnv = malloc(sizeof(sdnv_t) + byte_count * sizeof(uint8_t));
    assert(*sdnv);

    **sdnv = (sdnv_t) {
        .len = byte_count,
    };
}

void sdnv_destroy(sdnv_t *b) {
    free(b);
}

// Calculate the upper bound on the number of bytes needed to encode the given
// number of bytes with no compaction. Copied from RFC 5050.
static inline size_t max_bytes(size_t byte_count) {
    // Take the ceiling of (N * 8) / 7.
    return (byte_count * 8 + 7 - 1) / 7;
}

#ifdef MKBUNDLE_TEST
TEST test_max_bytes(void) {
    ASSERT_EQ(max_bytes(1), 2);
    ASSERT_EQ(max_bytes(7), 8);
    ASSERT_EQ(max_bytes(8), 10);
    ASSERT_EQ(max_bytes(128), 147);

    PASS();
}
#endif

// Find the first non-zero byte and return its index. Return the index of the
// last byte if all bytes are zero.
static size_t skip_bytes(const uint8_t *bytes, size_t byte_count) {
    assert(byte_count > 0);

    for (size_t skip = 0; skip < byte_count; skip += 1)
        if (bytes[skip])
            return skip;

    return byte_count - 1;
}

#ifdef MKBUNDLE_TEST
TEST test_skip_bytes(void) {
    ASSERT_EQ(skip_bytes((uint8_t[]){0x00}, 1), 0);
    ASSERT_EQ(skip_bytes((uint8_t[]){0x01}, 1), 0);
    ASSERT_EQ(skip_bytes((uint8_t[]){0x00, 0x01}, 2), 1);
    ASSERT_EQ(skip_bytes((uint8_t[]){0x01, 0x00}, 2), 0);

    PASS();
}
#endif

// Determine if the given most significant byte can be encoded with only one
// byte instead of two based the given number of input bytes.
static bool compact_msb(uint8_t first_byte, size_t byte_count) {
    // These mask off increasing portions of most-significant bite.
    static const uint8_t MASKS[CHAR_BIT] = {
        0x80, 0xC0, 0xE0, 0xF0,
        0xF8, 0xFC, 0xFE, 0xFF,
    };

    // Convert to zero-based index and clamp between 0 and 7.
    byte_count -= 1;
    byte_count %= 8;

    // If the byte contains zero bits under the associated mask, then those bits
    // don't need to be encoded in a whole new byte.
    //
    // For example, consider
    //
    //   first_byte = 0011 1111
    //   byte_count = 2
    //
    // Then this byte can be encoded as the most-significant output byte as
    //
    //   1111 111X  where X is the most-significant bit of the other byte
    //
    return !(first_byte & MASKS[byte_count]);
}

#ifdef MKBUNDLE_TEST
TEST test_compact_msb(void) {
    ASSERT(compact_msb(0x00, 1));
    ASSERT(compact_msb(0x40, 1));
    ASSERT(compact_msb(0x70, 1));

    ASSERT(compact_msb(0x7f, 1));
    ASSERT(compact_msb(0x3f, 2));
    ASSERT(compact_msb(0x1f, 3));
    ASSERT(compact_msb(0x0f, 4));
    ASSERT(compact_msb(0x07, 5));
    ASSERT(compact_msb(0x03, 6));
    ASSERT(compact_msb(0x01, 7));
    ASSERT(compact_msb(0x00, 8));

    ASSERT(!compact_msb(0xff, 1));
    ASSERT(!compact_msb(0x7f, 2));
    ASSERT(!compact_msb(0x3f, 3));
    ASSERT(!compact_msb(0x1f, 4));
    ASSERT(!compact_msb(0x0f, 5));
    ASSERT(!compact_msb(0x07, 6));
    ASSERT(!compact_msb(0x03, 7));
    ASSERT(!compact_msb(0x01, 8));

    ASSERT(compact_msb(0x7f, 9));
    ASSERT(compact_msb(0x00, 16));

    PASS();
}
#endif

typedef struct {
    // Whether to compact the most-significant byte of the input into one byte
    // instead of two.
    bool compact;
    // Number of bytes needed to encode the given input.
    size_t len;
} sdnv_params_t;

static void sdnv_params_init(sdnv_params_t *p, const uint8_t *bytes,
                             size_t byte_count)
{
    // Number of most-significant bytes to skip in the input.
    size_t skip = skip_bytes(bytes, byte_count);
    // Notice that 1 <= byte_count - skip <= len.
    size_t len = max_bytes(byte_count - skip);
    bool compact = compact_msb(bytes[skip], byte_count - skip);

    // Note that len is always greater than zero before this test because
    // skip < byte_count <= len.
    if (compact)
        len -= 1;

    // Even if all input bytes are zero, one output byte is still needed to
    // encode that.
    if (!len)
        len = 1;

    *p = (sdnv_params_t) {
        .compact = compact,
        .len = len,
    };
}

size_t sdnv_len(const uint8_t *bytes, size_t byte_count) {
    sdnv_params_t params;
    sdnv_params_init(&params, bytes, byte_count);

    return params.len;
}

#ifdef MKBUNDLE_TEST
TEST test_sdnv_len(void) {
    ASSERT_EQ(sdnv_len((uint8_t[]){0xff}, 1), 2);
    ASSERT_EQ(sdnv_len((uint8_t[]){0x7f}, 1), 1);
    ASSERT_EQ(sdnv_len((uint8_t[])
        {0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00,
         0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff}, 16), 18);
    ASSERT_EQ(sdnv_len((uint8_t[]){0x00, 0x00, 0x00, 0xff}, 4), 2);

    PASS();
}
#endif

sdnv_t *sdnv_encode(const uint8_t *bytes, size_t byte_count) {
// The value of the "continue" bit.
#define CONTINUE (1u << 7)

    static const uint8_t MASKS[] = {
        0X7F, 0X3F, 0X1F, 0x0F,
        0x07, 0x03, 0x01, 0x00,
    };

    sdnv_params_t params;
    sdnv_params_init(&params, bytes, byte_count);

    // The output SDNV itself.
    sdnv_t *out;
    sdnv_init(&out, params.len);

    // The current bit index. Start on the most significant bit.
    size_t bit = 0;

    // The high bits of the current byte.
    uint8_t hi;
    // The low bits of the previous byte.
    uint8_t lo = 0;

    // The current input and output byte. Iterate from least-significant to
    // most-significant byte.
    size_t i = byte_count - 1;
    size_t j = params.len - 1;

    // Loop until most-significant output byte.
    while (j) {
        // Mask off the bits to use from the current input byte and put them
        // where they should appear in the output byte.
        hi = (uint8_t)(bytes[i] & MASKS[bit]);
        hi = (uint8_t)(hi << bit);

        // Build the current output byte.
        out->bytes[j] = CONTINUE | hi | lo;

        // Save the bits that weren't used for the next iteration.
        lo = (uint8_t)(bytes[i] & ~MASKS[bit]);
        lo = (uint8_t)(lo >> (7u - bit));

        // On every 8 iterations, no bits of the current input byte are used, so
        // perform another iteration over the current input byte to get its bits.
        if (bit != 7)
            i -= 1;

        j -= 1;

        // Move to the next bit, clamped between 0 and 7.
        bit += 1;
        bit %= 8;
    }

    // The most-significant output byte always includes the low bits of the
    // last-visited input byte.
    out->bytes[0] = CONTINUE | lo;

    // If the most-significant non-zero input byte was compacted, then it wasn't
    // visited in the loop above, so use its bits to fill the remaining space in
    // the most-significant output byte.
    if (params.compact)
        out->bytes[0] = (uint8_t)(out->bytes[0] | bytes[i] << bit);

    // Unset the continue bit on the last output byte.
    out->bytes[params.len - 1] &= (uint8_t) ~CONTINUE;

    return out;

#undef CONTINUE
}

#ifdef MKBUNDLE_TEST
TEST test_sdnv_encode(void) {
    sdnv_t *sdnv;
    uint16_t in16;
    uint32_t in32;

    sdnv = sdnv_encode((uint8_t[]){0x88}, 1);
    ASSERT_EQ(sdnv->len, 2);
    ASSERT_EQ(sdnv->bytes[0], 0x81);
    ASSERT_EQ(sdnv->bytes[1], 0x08);
    sdnv_destroy(sdnv);

    sdnv = sdnv_encode((uint8_t[]){0xff}, 1);
    ASSERT_EQ(sdnv->len, 2);
    ASSERT_EQ(sdnv->bytes[0], 0x81);
    ASSERT_EQ(sdnv->bytes[1], 0x7f);
    sdnv_destroy(sdnv);

    sdnv = sdnv_encode((uint8_t[]){0x7f}, 1);
    ASSERT_EQ(sdnv->len, 1);
    ASSERT_EQ(sdnv->bytes[0], 0x7f);
    sdnv_destroy(sdnv);

    sdnv = sdnv_encode((uint8_t[]){0x00}, 1);
    ASSERT_EQ(sdnv->len, 1);
    ASSERT_EQ(sdnv->bytes[0], 0x00);
    sdnv_destroy(sdnv);

    sdnv = sdnv_encode((uint8_t[]){0x0a, 0xbc}, 2);
    ASSERT_EQ(sdnv->len, 2);
    ASSERT_EQ(sdnv->bytes[0], 0x95);
    ASSERT_EQ(sdnv->bytes[1], 0x3c);
    sdnv_destroy(sdnv);

    in16 = 0xbc0a;
    sdnv = sdnv_encode((uint8_t *)(&in16), 2);
    ASSERT_EQ(sdnv->len, 2);
    ASSERT_EQ(sdnv->bytes[0], 0x95);
    ASSERT_EQ(sdnv->bytes[1], 0x3c);
    sdnv_destroy(sdnv);

    sdnv = sdnv_encode((uint8_t[]){0x42, 0x34}, 2);
    ASSERT_EQ(sdnv->len, 3);
    ASSERT_EQ(sdnv->bytes[0], 0x81);
    ASSERT_EQ(sdnv->bytes[1], 0x84);
    ASSERT_EQ(sdnv->bytes[2], 0x34);
    sdnv_destroy(sdnv);

    sdnv = sdnv_encode((uint8_t[]){0x00, 0xff, 0xff, 0xff}, 4);
    ASSERT_EQ(sdnv->len, 4);
    ASSERT_EQ(sdnv->bytes[0], 0x87);
    ASSERT_EQ(sdnv->bytes[1], 0xff);
    ASSERT_EQ(sdnv->bytes[2], 0xff);
    ASSERT_EQ(sdnv->bytes[3], 0x7f);
    sdnv_destroy(sdnv);

    sdnv = sdnv_encode((uint8_t[]){0x00, 0x00, 0x00, 0xff}, 4);
    ASSERT_EQ(sdnv->len, 2);
    ASSERT_EQ(sdnv->bytes[0], 0x81);
    ASSERT_EQ(sdnv->bytes[1], 0x7f);
    sdnv_destroy(sdnv);

    sdnv = sdnv_encode((uint8_t[])
        {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}, 7);
    ASSERT_EQ(sdnv->len, 8);
    ASSERT_EQ(sdnv->bytes[0], 0xff);
    ASSERT_EQ(sdnv->bytes[1], 0xff);
    ASSERT_EQ(sdnv->bytes[2], 0xff);
    ASSERT_EQ(sdnv->bytes[3], 0xff);
    ASSERT_EQ(sdnv->bytes[4], 0xff);
    ASSERT_EQ(sdnv->bytes[5], 0xff);
    ASSERT_EQ(sdnv->bytes[6], 0xff);
    ASSERT_EQ(sdnv->bytes[7], 0x7f);
    sdnv_destroy(sdnv);

    sdnv = sdnv_encode((uint8_t[])
        {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}, 8);
    ASSERT_EQ(sdnv->len, 10);
    ASSERT_EQ(sdnv->bytes[0], 0x81);
    for (size_t i = 1; i <= 8; i += 1)
        ASSERT_EQ(sdnv->bytes[i], 0xff);
    ASSERT_EQ(sdnv->bytes[9], 0x7f);
    sdnv_destroy(sdnv);

    sdnv = sdnv_encode((uint8_t[])
        {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00}, 9);
    ASSERT_EQ(sdnv->len, 11);
    ASSERT_EQ(sdnv->bytes[0], 0x83);
    for (size_t i = 1; i <= 8; i += 1)
        ASSERT_EQ(sdnv->bytes[i], 0xff);
    ASSERT_EQ(sdnv->bytes[9], 0xfe);
    ASSERT_EQ(sdnv->bytes[10], 0x00);
    sdnv_destroy(sdnv);

    sdnv = sdnv_encode((uint8_t[])
        {0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00,
         0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff}, 14);
    ASSERT_EQ(sdnv->len, 15);
    ASSERT_EQ(sdnv->bytes[0], 0xbf);
    ASSERT_EQ(sdnv->bytes[1], 0xe0);
    ASSERT_EQ(sdnv->bytes[2], 0x8f);
    ASSERT_EQ(sdnv->bytes[3], 0xf8);
    ASSERT_EQ(sdnv->bytes[4], 0x83);
    ASSERT_EQ(sdnv->bytes[5], 0xfe);
    ASSERT_EQ(sdnv->bytes[6], 0x80);
    ASSERT_EQ(sdnv->bytes[7], 0xff);
    ASSERT_EQ(sdnv->bytes[8], 0xc0);
    ASSERT_EQ(sdnv->bytes[9], 0x9f);
    ASSERT_EQ(sdnv->bytes[10], 0xf0);
    ASSERT_EQ(sdnv->bytes[11], 0x87);
    ASSERT_EQ(sdnv->bytes[12], 0xfc);
    ASSERT_EQ(sdnv->bytes[13], 0x81);
    ASSERT_EQ(sdnv->bytes[14], 0x7f);
    sdnv_destroy(sdnv);

    sdnv = sdnv_encode((uint8_t[])
        {0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00,
         0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff}, 16);
    ASSERT_EQ(sdnv->len, 18);
    ASSERT_EQ(sdnv->bytes[0], 0x81);
    ASSERT_EQ(sdnv->bytes[1], 0xff);
    ASSERT_EQ(sdnv->bytes[2], 0x80);
    ASSERT_EQ(sdnv->bytes[3], 0xbf);
    ASSERT_EQ(sdnv->bytes[4], 0xe0);
    ASSERT_EQ(sdnv->bytes[5], 0x8f);
    ASSERT_EQ(sdnv->bytes[6], 0xf8);
    ASSERT_EQ(sdnv->bytes[7], 0x83);
    ASSERT_EQ(sdnv->bytes[8], 0xfe);
    ASSERT_EQ(sdnv->bytes[9], 0x80);
    ASSERT_EQ(sdnv->bytes[10], 0xff);
    ASSERT_EQ(sdnv->bytes[11], 0xc0);
    ASSERT_EQ(sdnv->bytes[12], 0x9f);
    ASSERT_EQ(sdnv->bytes[13], 0xf0);
    ASSERT_EQ(sdnv->bytes[14], 0x87);
    ASSERT_EQ(sdnv->bytes[15], 0xfc);
    ASSERT_EQ(sdnv->bytes[16], 0x81);
    ASSERT_EQ(sdnv->bytes[17], 0x7f);
    sdnv_destroy(sdnv);

    sdnv = sdnv_encode((uint8_t[]){0x00, 0x00, 0x00, 0x7f}, 4);
    ASSERT_EQ(sdnv->len, 1);
    ASSERT_EQ(sdnv->bytes[0], 0x7f);
    sdnv_destroy(sdnv);

    sdnv = sdnv_encode((uint8_t[]){0x00, 0x00, 0x3f, 0xff}, 4);
    ASSERT_EQ(sdnv->len, 2);
    ASSERT_EQ(sdnv->bytes[0], 0xff);
    ASSERT_EQ(sdnv->bytes[1], 0x7f);
    sdnv_destroy(sdnv);

    sdnv = sdnv_encode((uint8_t[]){0x00, 0x00, 0x7f, 0xff}, 4);
    ASSERT_EQ(sdnv->len, 3);
    ASSERT_EQ(sdnv->bytes[0], 0x81);
    ASSERT_EQ(sdnv->bytes[1], 0xff);
    ASSERT_EQ(sdnv->bytes[2], 0x7f);
    sdnv_destroy(sdnv);

    sdnv = sdnv_encode((uint8_t[]){0x00, 0x00, 0xff, 0xff}, 4);
    ASSERT_EQ(sdnv->len, 3);
    ASSERT_EQ(sdnv->bytes[0], 0x83);
    ASSERT_EQ(sdnv->bytes[1], 0xff);
    ASSERT_EQ(sdnv->bytes[2], 0x7f);
    sdnv_destroy(sdnv);

    in32 = 0xff3f0000;
    sdnv = sdnv_encode((uint8_t *) &in32, 4);
    ASSERT_EQ(sdnv->len, 2);
    ASSERT_EQ(sdnv->bytes[0], 0xff);
    ASSERT_EQ(sdnv->bytes[1], 0x7f);
    sdnv_destroy(sdnv);

    PASS();
}
#endif

#ifdef MKBUNDLE_TEST
SUITE(sdnv_suite) {
    RUN_TEST(test_max_bytes);
    RUN_TEST(test_skip_bytes);
    RUN_TEST(test_compact_msb);
    RUN_TEST(test_sdnv_encode);
    RUN_TEST(test_sdnv_len);
}
#endif
