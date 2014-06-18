// See copyright notice in Copying.

#include <inttypes.h>

#include "eid.h"

#include "htable.c"

htable_hash_t fnv(const eid_table_str_t *key) {
    htable_hash_t hval = 0x811c9dc5;

    for (size_t i = 0; i < key->len; i += 1) {
        hval ^= (uint8_t) key->str[i];
        hval *= 0x01000193;
    }

    return hval;
}
