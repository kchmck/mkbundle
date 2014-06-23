// See copyright notice in Copying.

#ifndef UTIL_H
#define UTIL_H

#include <assert.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>

#include "strbuf.h"
#include "sdnv.h"

// Get the number of elements in the array.
#define ASIZE(a) (sizeof(a) / sizeof((a)[0]))

// A value and its symbol.
typedef struct {
    uint32_t val;
    const char *sym;
} sym_t;

#define SYM_INVALID UINT32_MAX

// Try to parse one of the given symbols from the given string.
uint32_t sym_parse(const char *str, const sym_t *syms, size_t sym_count);

// Read an entire file into the given buffer.
void collect(strbuf_t **buf, FILE *stream);

#define WRITE(stream, buf, len) do { \
    size_t ret = fwrite((buf), sizeof(uint8_t), (len), (stream)); \
    assert(ret == (len)); \
} while (0)

#define WRITE_SDNV(stream, val) do { \
    sdnv_t *sdnv = SDNV_ENCODE(val); \
    WRITE(stream, sdnv->bytes, sdnv->len); \
    sdnv_destroy(sdnv); \
} while (0)

#define WRITE_EID(stream, eid) do { \
    WRITE_SDNV(stream, SWAP32((eid)->scheme)); \
    WRITE_SDNV(stream, SWAP32((eid)->ssp)); \
} while(0)

// Determine if on a little-endian platform.
static inline bool little_endian(void) {
    // x and y are stored starting at the same memory location, so on a little
    // endian machine, the 1 will be stored in the low byte of x and picked up
    // by y. On a big endian machine, the 1 will be stored in the high byte of
    // x, so y will be 0.
    union {
        uint16_t x;
        uint8_t y;
    } a = {1};

    return a.y == 1;
}

// These macros are required to create valid lvalues.
#define SWAP32(x) *( \
    assert(sizeof(x) == sizeof(uint32_t)), \
    (uint32_t[]) {little_endian() ? __builtin_bswap32(x) : x} \
)

#define SWAP64(x) *( \
    assert(sizeof(x) == sizeof(uint64_t)), \
    (uint64_t[]) {little_endian() ? __builtin_bswap64(x) : x} \
)

#endif
