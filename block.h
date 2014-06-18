// See copyright notice in Copying.

#ifndef BLOCK_H
#define BLOCK_H

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "ext-block.h"
#include "primary-block.h"

typedef enum {
    BLOCK_TYPE_PRIMARY,
    BLOCK_TYPE_EXT,

    BLOCK_TYPE_INVALID,
} block_type_t;

// An wrapper around available block types.
typedef struct {
    block_type_t type;

    union {
        primary_block_t primary;
        ext_block_t ext;
    };
} block_t;

// Initialize the block to a default state.
void block_init(block_t *b);

// Free any memory held by the block.
void block_destroy(block_t *b);

// Parse the parameters in the buffer into a specific block. Return true on
// success and false otherwise.
bool block_unserialize(block_t *b, const char *buf, size_t len);

// Write the binary form of the block to the file.
void block_write(const block_t *b, FILE *stream);

#endif
