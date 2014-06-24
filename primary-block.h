// See copyright notice in Copying.

#ifndef PRIMARY_BLOCK_H
#define PRIMARY_BLOCK_H

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>

#include "eid.h"
#include "parser.h"
#include "strbuf.h"

#define HTABLE_COMMON
#include "htable.h"

// Table key strings must be associated with a length, since they may not
// necessarily be null terminated.
typedef struct {
    const char *str;
    size_t len;
} eid_table_str_t;

// Define eid_table_t.
#define HTABLE_RESET
#include "htable.h"
#define HTABLE_NAME eid_map
#define HTABLE_KEY_TYPE const eid_table_str_t *
#define HTABLE_DATA_TYPE size_t
#define HTABLE_DEFAULT_SIZE (1u << 6)
#define HTABLE_HASH_KEY(key) fnv(key)
#include "htable.h"

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

    // Maps EID strings to offsets inside eid_buf.
    eid_map_t *eid_map;
    // Holds all EID strings.
    strbuf_t *eid_buf;
} primary_block_t;

// Initialize the block to a default state.
void primary_block_init(primary_block_t *b);

// Free memory held by the block.
void primary_block_destroy(primary_block_t *b);

// Write the params for the block to JSON.
void primary_block_serialize(const primary_block_t *b, FILE *stream);

// Unserialize a block from the parser.
bool primary_block_unserialize(primary_block_t *b, parser_t *p);

// Write the final binary form of the block.
void primary_block_write(const primary_block_t *b, FILE *stream);

// Parse the string into an EID and add it to the block.
bool primary_block_add_eid(primary_block_t *b, eid_t *e, const char *str);

#endif
