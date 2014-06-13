// See copyright notice in Copying.

#ifndef SDNV_H
#define SDNV_H

#include <inttypes.h>
#include <stdlib.h>

// An encoded SDNV.
typedef struct {
    // Length of bytes array.
    size_t len;
    // Encoded bytes themselves.
    uint8_t bytes[];
} sdnv_t;

size_t sdnv_len(const uint8_t *bytes, size_t byte_count);

#define SDNV_LEN(x) sdnv_len((const uint8_t *) &(x), sizeof(x))

// Encode the given bytes into an SDNV.
sdnv_t *sdnv_encode(const uint8_t *bytes, size_t byte_count);

// Encode the given variable into an SDNV.
#define SDNV_ENCODE(x) sdnv_encode((uint8_t *) &(x), sizeof(x))

// Free the memory held by the given SDNV.
void sdnv_destroy(sdnv_t *b);

#endif
