// See copyright notice in Copying.

#ifndef PRIMARY_BLOCK_H
#define PRIMARY_BLOCK_H

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>

#include "eid.h"
#include "parser.h"
#include "strbuf.h"

typedef struct {
    uint8_t version;
    uint32_t flags;
    uint32_t length;

    eid_t dest;
    eid_t src;
    eid_t report_to;
    eid_t custodian;

    uint32_t creation_ts;
    uint32_t creation_seq;
    uint32_t lifetime;
    uint32_t eids_size;

    eid_map_t *eid_map;
    strbuf_t *eid_buf;
} primary_block_t;

// Initialize the given block to a default state.
void primary_block_init(primary_block_t *b);

// Free memory held by the given block.
void primary_block_destroy(primary_block_t *b);

// Write the params for the given block to JSON.
void primary_block_serialize(const primary_block_t *b, FILE *stream);

// Unserialize a block from the given parser.
void primary_block_unserialize(primary_block_t *b, parser_t *p);

// Write the final binary form of the given block.
void primary_block_write(const primary_block_t *b, FILE *stream);

// Parse the given string into an EID and add it to the given block.
bool primary_block_add_eid(primary_block_t *b, eid_t *e, const char *str);

#endif
