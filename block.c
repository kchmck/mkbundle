// See copyright notice in Copying.

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

#include "block.h"
#include "util.h"

void block_params_init(block_params_t *p, const uint8_t *data, size_t len) {
    *p = (block_params_t) {
        .type = BLOCK_TYPE_DEFAULT,
        .flags = BLOCK_FLAGS_DEFAULT,
        .len = len,
        .data = data,
    };
}

void block_init(block_t *b, const block_params_t *params) {
    *b = (block_t) {
        .params = params,
    };
}

void block_destroy(block_t *b) {
    sdnv_destroy(b->flags);
    sdnv_destroy(b->length);
}

void block_build(block_t *b) {
    b->flags = SDNV_ENCODE(b->params->flags);
    b->length = SDNV_ENCODE(SWAP64(b->params->len));
}

void block_write(const block_t *b, FILE *stream) {
    WRITE(&b->params->type, sizeof(b->params->type));
    WRITE_SDNV(b->flags);
    WRITE_SDNV(b->length);
    WRITE(b->params->data, b->params->len);
}
