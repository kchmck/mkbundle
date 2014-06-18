// See copyright notice in Copying.

#ifndef EID_H
#define EID_H

#include <inttypes.h>

#define HTABLE_COMMON
#include "htable.h"

typedef struct {
    const char *str;
    size_t len;
} eid_table_str_t;

htable_hash_t fnv(const eid_table_str_t *key);

// Define eid_table_t.
#define HTABLE_RESET
#include "htable.h"
#define HTABLE_NAME eid_map
#define HTABLE_KEY_TYPE const eid_table_str_t *
#define HTABLE_DATA_TYPE size_t
#define HTABLE_HASH_KEY(key) fnv(key)
#define HTABLE_DEFAULT_SIZE (1u << 6)
#include "htable.h"

typedef struct {
    uint32_t scheme;
    uint32_t ssp;
} eid_t;

#endif
