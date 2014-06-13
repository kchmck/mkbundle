//
// Hopscotch Hashtable
//  - http://people.csail.mit.edu/shanir/publications/disc2008_submission_98.pdf
//  - http://cs.nyu.edu/~lerner/spring11/proj_hopscotch.pdf
//
// Copyright (C) 2011-2014 Mick Koch <mick@kochm.co>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//

#include <assert.h>
#include <limits.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#if defined HTABLE_FIXED
#define HTABLE_SIZE(ht) (HTABLE_DEFAULT_SIZE)
#define HTABLE_MASK(ht) ((HTABLE_DEFAULT_SIZE) - 1)
#else
#define HTABLE_SIZE(ht) ht->size
#define HTABLE_MASK(ht) ht->mask
#endif

#ifndef HTABLE_C_COMMON
#define HTABLE_C_COMMON

#define HTABLE_ALLOC NAME(alloc)
#define HTABLE_SEARCH_SLOTS NAME(search_slots)
#define HTABLE_FIND_SLOT NAME(find_slot)
#define HTABLE_FIND_SWAP NAME(find_swap)
#define HTABLE_ADD_HASH NAME(add_hash)
#define HTABLE_GROW NAME(grow)
#define HTABLE_REHASH NAME(rehash)

// Number of slots in a bucket including the bucket slot itself
#define HTABLE_BUCKET_SIZE (sizeof(htable_fwd_t) * CHAR_BIT)

typedef enum {
  // Found an empty slot
  WALK_FOUND_EMPTY,
  // Found a slot that has an identical hash
  WALK_FOUND_IDENTICAL,
  // Found an empty slot, but it is outside the bucket and needs to be swapped
  WALK_FOUND_EMPTY_SWAP,
  // No empty slot was found
  WALK_ERROR,
} htable_walk_t;

// A slot and the bucket it belongs to
typedef struct {
  size_t bucket;
  size_t slot;
} htable_cursor_t;

// Clamp x so 0 <= x < ht->size
#define HTABLE_CLAMP(x) ((x) & HTABLE_MASK(ht))
#define HTABLE_INCR(x) ((x) = HTABLE_CLAMP((x) + 1))

#endif

#if !defined HTABLE_FIXED
// Allocate a table to hold the given number of slots.
static HTABLE_T *HTABLE_ALLOC(HTABLE_T *ht, size_t size) {
  // Size must be greater than HTABLE_BUCKET_SIZE, so the table can hold at
  // least one full bucket.
  assert(size >= HTABLE_BUCKET_SIZE);

  // Size in bytes of all slots
  size_t nbytes = size * sizeof(HTABLE_SLOT_T);

  ht = realloc(ht, sizeof(HTABLE_T) + nbytes);
  ht->size = size;

  // Because size is always a power of two, subtracting one creates a bitmap
  // that, when ANDed with a hash, creates an index between zero and size:
  //
  //   size     = 0000 1000 0000 = 128
  //   size - 1 = 0000 0111 1111 = 127
  //          AND 1010 1110 0010 = 2786
  //            = 0000 0110 0010 = 98
  //
  ht->mask = size - 1;

  // Zero all hashes so the table appears empty.
  memset(ht->slots, 0, nbytes);

  return ht;
}

void HTABLE_INIT(HTABLE_T **htp) {
  *htp = HTABLE_ALLOC(NULL, (HTABLE_DEFAULT_SIZE));
}

void HTABLE_DESTROY(HTABLE_T *ht) {
  free(ht);
}
#endif

// Search for a slot that hashes to the given key starting at the hash bucket.
static bool HTABLE_SEARCH_SLOTS(htable_cursor_t *c, const HTABLE_T *ht,
                                const HTABLE_KEY_TYPE key)
{
  // Forward bitmap of hash bucket
  htable_fwd_t fwd;

  // Find the hash bucket.
  htable_hash_t h = HTABLE_HASH_KEY(key);
  c->bucket = HTABLE_CLAMP(h);

  for (fwd = ht->slots[c->bucket].fwd, c->slot = c->bucket; fwd;
       fwd >>= 1, HTABLE_INCR(c->slot))
    if (fwd & 1 && ht->slots[c->slot].hash == h)
      return true;

  return false;
}

HTABLE_DATA_TYPE *HTABLE_LOOKUP(HTABLE_T *ht, const HTABLE_KEY_TYPE key) {
  htable_cursor_t search;

  if (HTABLE_SEARCH_SLOTS(&search, ht, key))
    return &ht->slots[search.slot].data;

  return NULL;
}

HTABLE_DATA_TYPE *HTABLE_REMOVE(HTABLE_T *ht, const HTABLE_KEY_TYPE key) {
  htable_cursor_t search;

  if (!HTABLE_SEARCH_SLOTS(&search, ht, key))
    return NULL;

  // Mark slot as empty.
  ht->slots[search.slot].hash = 0;
  // Disassociate slot from hash bucket.
  ht->slots[search.bucket].fwd ^= 1u << HTABLE_CLAMP(search.slot - search.bucket);

  return &ht->slots[search.slot].data;
}

// Try to find a slot suitable for the hash h.
static htable_walk_t HTABLE_FIND_SLOT(htable_cursor_t *dest, const HTABLE_T *ht,
                                      size_t out, htable_hash_t h)
{
  // Look within the hash bucket.
  dest->slot = dest->bucket;

  do {
    if (!ht->slots[dest->slot].hash)
      return WALK_FOUND_EMPTY;

    if (ht->slots[dest->slot].hash == h)
      return WALK_FOUND_IDENTICAL;
  } while (HTABLE_INCR(dest->slot) != out);

  // No empty slot was found within the bucket, so search outside it.
  while (dest->slot != dest->bucket) {
    if (!ht->slots[dest->slot].hash)
      return WALK_FOUND_EMPTY_SWAP;

    HTABLE_INCR(dest->slot);
  }

  return WALK_ERROR;
}

// Try to find a slot closer to slot's bucket to swap places with.
static bool HTABLE_FIND_SWAP(htable_cursor_t *swap, const HTABLE_T *ht,
                             size_t slot)
{
// Farthest away the bucket slot can be from the given slot
#define SWAP_LOOKBACK (HTABLE_BUCKET_SIZE - 1)
// Ensures the number of bits in the fwd bitmap doesn't extend past the given
// slot
#define SWAP_MASK_INIT ((1UL << SWAP_LOOKBACK) - 1)

  htable_fwd_t mask, fwd;

  for (mask = SWAP_MASK_INIT, swap->bucket = HTABLE_CLAMP(slot - SWAP_LOOKBACK);
       mask; mask >>= 1, HTABLE_INCR(swap->bucket))
    for (fwd = ht->slots[swap->bucket].fwd & mask, swap->slot = swap->bucket;
         fwd; fwd >>= 1, HTABLE_INCR(swap->slot))
      if (fwd & 1)
        return true;

  return false;

#undef SWAP_MASK_INIT
#undef SWAP_LOOKBACK
}

HTABLE_DATA_TYPE *HTABLE_ADD_HASH(HTABLE_T *ht, htable_hash_t h) {
  // Destination slot and swap slot
  htable_cursor_t dest, swap;
  // Index of the first slot not in the hash bucket
  size_t out;

  dest.bucket = HTABLE_CLAMP(h);
  out = HTABLE_CLAMP(dest.bucket + HTABLE_BUCKET_SIZE);

  switch (HTABLE_FIND_SLOT(&dest, ht, out, h)) {
  case WALK_FOUND_EMPTY:
  break;

  case WALK_FOUND_IDENTICAL:
    return &ht->slots[dest.slot].data;

  case WALK_FOUND_EMPTY_SWAP:
    // Continually swap closer to the bucket until swapped into it.
    do {
      if (!HTABLE_FIND_SWAP(&swap, ht, dest.slot))
        return NULL;

      // Copy the swap slot forward.
      ht->slots[dest.slot].hash = ht->slots[swap.slot].hash;
      ht->slots[dest.slot].data = ht->slots[swap.slot].data;

      // Disassociate old slot from swap hash bucket.
      ht->slots[swap.bucket].fwd ^= 1u << HTABLE_CLAMP(swap.slot - swap.bucket);
      // Associate new slot with swap hash bucket.
      ht->slots[swap.bucket].fwd |= 1u << HTABLE_CLAMP(dest.slot - swap.bucket);

      // Continue swapping if still not in hash bucket.
      dest.slot = swap.slot;
    } while (dest.slot < dest.bucket || dest.slot >= out);
  break;

  case WALK_ERROR:
    return NULL;
  }

  // Mark slot as taken.
  ht->slots[dest.slot].hash = h;
  // Associate slot with hash bucket.
  ht->slots[dest.bucket].fwd |= 1u << HTABLE_CLAMP(dest.slot - dest.bucket);

  return &ht->slots[dest.slot].data;
}

#if !defined HTABLE_FIXED
// Rehash ht into nht: O(n). Return true if rehash was successful and false if
// not.
static bool HTABLE_REHASH(HTABLE_T *ht, HTABLE_T *nht) {
  // Current slot
  HTABLE_SLOT_T *slot;
  HTABLE_DATA_TYPE *data;

  for (size_t i = 0; slot = &ht->slots[i], i < HTABLE_SIZE(ht); i += 1) {
    // Skip empty slots.
    if (!slot->hash)
      continue;

    data = HTABLE_ADD_HASH(nht, slot->hash);

    if (!data)
      return false;

    *data = slot->data;
  }

  return true;
}

// Grow and rehash the hash table: O(n). Return true if successful and false if
// not.
static HTABLE_T *HTABLE_GROW(HTABLE_T *ht) {
  HTABLE_T *nht = NULL;

  for (;;) {
    ht->size *= 2;
    nht = HTABLE_ALLOC(nht, ht->size);

    if (HTABLE_REHASH(ht, nht)) {
      free(ht);
      return nht;
    }
  }
}

HTABLE_DATA_TYPE *HTABLE_ADD(HTABLE_T **htp, const HTABLE_KEY_TYPE key) {
  HTABLE_T *ht;
  htable_hash_t h;
  HTABLE_DATA_TYPE *data;

  h = HTABLE_HASH_KEY(key);

  for (;;) {
    ht = *htp;
    data = HTABLE_ADD_HASH(ht, h);

    if (data)
      return data;

    *htp = HTABLE_GROW(ht);
  }
}
#else
HTABLE_DATA_TYPE *HTABLE_ADD(HTABLE_T *ht, const HTABLE_KEY_TYPE key) {
  return HTABLE_ADD_HASH(ht, HTABLE_HASH_KEY(key));
}

bool HTABLE_ADD_PAIRS(HTABLE_T *ht, const HTABLE_PAIR_T *pairs, size_t npairs) {
  const HTABLE_PAIR_T *pair;
  HTABLE_DATA_TYPE *data;

  for (size_t i = 0; pair = &pairs[i], i < npairs; i += 1) {
    data = HTABLE_ADD(ht, pair->key);

    if (!data)
      return false;

    *data = pair->data;
  }

  return true;
}
#endif

void HTABLE_ITER_INIT(HTABLE_ITER_T *it, HTABLE_T *ht) {
  it->ht = ht;
  it->slot = 0;
}

HTABLE_DATA_TYPE *HTABLE_ITER_NEXT(HTABLE_ITER_T *it) {
  HTABLE_DATA_TYPE *data;

  while (it->slot < HTABLE_SIZE(it->ht)) {
    if (it->ht->slots[it->slot].hash) {
      data = &it->ht->slots[it->slot].data;
      it->slot += 1;

      return data;
    }

    it->slot += 1;
  }

  return NULL;
}
