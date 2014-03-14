// See copyright notice in Copying.

#ifndef BLOCK_H
#define BLOCK_H

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

#include "sdnv.h"

enum { BLOCK_TYPE_DEFAULT = 0x1 };
enum { BLOCK_FLAGS_DEFAULT = 0x0 };

typedef struct {
    uint8_t type;
    uint8_t flags;
    size_t len;
    const uint8_t *data;
} block_params_t;

void block_params_init(block_params_t *p, const uint8_t *data, size_t len);

typedef struct {
    const block_params_t *params;

    sdnv_t *flags;
    sdnv_t *length;
} block_t;

void block_init(block_t *b, const block_params_t *params);

void block_destroy(block_t *b);

void block_build(block_t *b);

void block_write(const block_t *b, FILE *stream);

#endif
