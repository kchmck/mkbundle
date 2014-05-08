// See copyright notice in Copying.

#ifndef UTIL_H
#define UTIL_H

#include <assert.h>
#include <inttypes.h>

#define WRITE(stream, buf, len) do { \
    size_t ret = fwrite((buf), sizeof(uint8_t), (len), (stream)); \
    assert(ret == (len)); \
} while (0)

#define WRITE_SDNV(stream, sdnv) WRITE(stream, (sdnv)->bytes, (sdnv)->len)

// TODO: check if on big endian and make these noops.
static inline uint32_t swap32(uint32_t x) {
    return __builtin_bswap32(x);
}

static inline uint64_t swap64(uint64_t x) {
    return __builtin_bswap64(x);
}

#define SWAP32(x) *((uint32_t[]){swap32(x)})
#define SWAP64(x) *((uint64_t[]){swap64(x)})

#endif
