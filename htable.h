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

// Example usage:
//
//     #define HTABLE_COMMON
//     #include "htable.h"
//
//     #define HTABLE_RESET
//     #include "htable.h"
//     #define HTABLE_NAME my_map
//     #define HTABLE_KEY_TYPE const key_t
//     #define HTABLE_DATA_TYPE data_t
//     #define HTABLE_HASH_KEY(key) my_hash(key)
//     #define HTABLE_DEFAULT_SIZE (1 << 10)
//     #include "htable.h"
//     #include "htable.c"
//
// This will create the definitions and implementation of a hash table with the
// type my_map_t that maps key_t's to data_t's. It uses the my_hash function to
// hash keys and has a default size of 1024 entries. It can be operated on with
// my_map_add, my_map_lookup, etc.
//

#if defined HTABLE_RESET
#undef HTABLE_RESET

// Undefine all parameters.
#undef HTABLE_NAME
#undef HTABLE_KEY_TYPE
#undef HTABLE_DATA_TYPE
#undef HTABLE_HASH_KEY
#undef HTABLE_DEFAULT_SIZE
#undef HTABLE_STATIC

// Undefine H file functions
#undef NAME

// Undefine C file functions
#undef HTABLE_SIZE
#undef HTABLE_MASK

#elif defined HTABLE_COMMON
#undef HTABLE_COMMON

#ifndef HTABLE_H_COMMON
#define HTABLE_H_COMMON

#include <inttypes.h>
#include <stddef.h>

// A lookahead bitmap
typedef uint_fast32_t htable_fwd_t;
// The hash of a key
typedef uint_fast32_t htable_hash_t;

#endif

#elif !defined HTABLE_NAME || !defined HTABLE_KEY_TYPE || \
      !defined HTABLE_DATA_TYPE || !defined HTABLE_HASH_KEY || \
      !defined HTABLE_DEFAULT_SIZE
#error "HTABLE_NAME, HTABLE_DATA_TYPE, HTABLE_KEY_TYPE, HTABLE_HASH_KEY, " \
       "and HTABLE_DEFAULT_SIZE must be defined"
#elif HTABLE_DEFAULT_SIZE & (HTABLE_DEFAULT_SIZE - 1)
#error "HTABLE_DEFAULT_SIZE must be a power of two"
#else

// Paste suffix to the end of HTABLE_NAME. This requires two levels of
// expansion.
#define NAME(suffix) EXPAND(HTABLE_NAME, suffix)
#define EXPAND(name, suffix) CONCAT(name, suffix)
#define CONCAT(name, suffix) name ## _ ## suffix

#define HTABLE_T NAME(t)
#define HTABLE_SLOT_T NAME(slot_t)

#define HTABLE_INIT NAME(init)
#define HTABLE_DESTROY NAME(destroy)
#define HTABLE_LOOKUP NAME(lookup)
#define HTABLE_ADD NAME(add)
#define HTABLE_REMOVE NAME(remove)

#define HTABLE_PAIR_T NAME(pair_t)
#define HTABLE_ADD_PAIRS NAME(add_pairs)

#define HTABLE_ITER_T NAME(iter_t)
#define HTABLE_ITER_INIT NAME(iter_init)
#define HTABLE_ITER_NEXT NAME(iter_next)

typedef struct {
  htable_hash_t hash;
  htable_fwd_t fwd;
  HTABLE_DATA_TYPE data;
} HTABLE_SLOT_T;

typedef struct {
#if !defined HTABLE_STATIC
  size_t size;
  htable_hash_t mask;
  HTABLE_SLOT_T slots[];
#else
  HTABLE_SLOT_T slots[HTABLE_DEFAULT_SIZE];
#endif
} HTABLE_T;

#if !defined HTABLE_STATIC
// Allocate and initialized the table.
void HTABLE_INIT(HTABLE_T **htp);
// Deallocate the table. This doesn't deallocate the data in the individual
// slots! Use an iterator to do that before destroying the table.
void HTABLE_DESTROY(HTABLE_T *ht);
// Add data for key: O(1).
HTABLE_DATA_TYPE *HTABLE_ADD(HTABLE_T **htp, const HTABLE_KEY_TYPE key);
#else
typedef struct {
  const HTABLE_KEY_TYPE key;
  HTABLE_DATA_TYPE data;
} HTABLE_PAIR_T;

HTABLE_DATA_TYPE *HTABLE_ADD(HTABLE_T *ht, const HTABLE_KEY_TYPE key);
void HTABLE_ADD_PAIRS(HTABLE_T *ht, const HTABLE_PAIR_T *pairs, size_t npairs);
#endif

// Lookup data for key: O(1). Return NULL if no data matches key.
HTABLE_DATA_TYPE *HTABLE_LOOKUP(HTABLE_T *ht, const HTABLE_KEY_TYPE key);
// Remove data for key: O(1). Return NULL if no data matches key.
HTABLE_DATA_TYPE *HTABLE_REMOVE(HTABLE_T *ht, const HTABLE_KEY_TYPE key);

typedef struct {
  HTABLE_T *ht;
  size_t slot;
} HTABLE_ITER_T;

// Initialize the iterator with a hash table.
void HTABLE_ITER_INIT(HTABLE_ITER_T *it, HTABLE_T *ht);
// Get the next item from the iterator.
HTABLE_DATA_TYPE *HTABLE_ITER_NEXT(HTABLE_ITER_T *it);

#endif
