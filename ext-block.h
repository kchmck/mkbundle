// See copyright notice in Copying.

#ifndef EXT_BLOCK_H
#define EXT_BLOCK_H

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>

#include "eid.h"
#include "parser.h"

#define ALIST_RESET
#include "alist.h"
#define ALIST_NAME eid_refs
#define ALIST_TYPE eid_t
// With 4 possible schemes and 4 possible SSPs in the primary bundle, there
// are 16 possible different references.
#define ALIST_CAP 16
#define ALIST_SLOT_INIT {0}
#include "alist.h"

typedef enum {
    EXT_BLOCK_DEFAULT = 0,
    EXT_BLOCK_PAYLOAD = 1,
    EXT_BLOCK_PHIB = 5,

    EXT_BLOCK_INVALID,
} ext_block_type_t;

typedef struct {
    uint8_t type;
    uint8_t flags;
    uint32_t length;
    uint32_t ref_count;
    eid_refs_t refs;
} ext_block_t;

void ext_block_init(ext_block_t *b);

void ext_block_serialize(const ext_block_t *b, FILE *stream);

bool ext_block_unserialize(ext_block_t *b, parser_t *p);

void ext_block_write(const ext_block_t *b, FILE *stream);

bool ext_block_add_ref(ext_block_t *b, const char *str);

#endif
