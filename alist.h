//
// Copyright (C) 2012-2014 Mick Koch <mick@kochm.co>
//
// This program is free software. It comes without any warranty, to the extent
// permitted by applicable law. You can redistribute it and/or modify it under
// the terms of the Do WTF You Want To Public License, Version 2, as published
// by Sam Hocevar. See http://sam.zoy.org/wtfpl/COPYING for more details.
//

//
// Bare bones arraylist
//
// Parameters:
//   ALIST_NAME: name to prepend to all types and functions
//   ALIST_TYPE: datatype of each array slot
//   ALIST_CAP: capacity of the list
//   ALIST_SLOT_INIT: initializer for each slot
//

// Undefine all parameters.
#ifdef ALIST_RESET
#undef ALIST_RESET

#undef ALIST_NAME
#undef ALIST_TYPE
#undef ALIST_CAP
#undef ALIST_SLOT_INIT

#undef NAME

#elif !defined(ALIST_NAME) || !defined(ALIST_TYPE) || !defined(ALIST_CAP) || \
      !defined(ALIST_SLOT_INIT)
#error "ALIST_NAME, ALIST_TYPE, ALIST_CAP, ALIST_SLOT_INIT must be defined"
#else

#include <stddef.h>

#define NAME(suffix) EXPAND(ALIST_NAME, suffix)
#define CONCAT(name, suffix) name ## _ ## suffix
#define EXPAND(name, suffix) CONCAT(name, suffix)

#define ALIST_T NAME(t)
#define ALIST_INIT NAME(init)
#define ALIST_PUSH NAME(push)
#define ALIST_POP NAME(pop)

typedef struct {
    size_t len;
    ALIST_TYPE slots[ALIST_CAP];
} ALIST_T;

void ALIST_INIT(ALIST_T *l);
ALIST_TYPE *ALIST_PUSH(ALIST_T *l);
ALIST_TYPE *ALIST_POP(ALIST_T *l);

#endif
