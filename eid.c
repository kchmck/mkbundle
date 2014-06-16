// See copyright notice in Copying.

#include "eid.h"

#include "htable.c"

htable_hash_t fnv(const char *key) {
    htable_hash_t hval = 0x811c9dc5;

    while (*key) {
        hval ^= (uint8_t) *key;
        hval *= 0x01000193;

        key += 1;
    }

    return hval;
}
