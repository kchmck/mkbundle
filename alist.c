//
// Copyright (C) 2012-2014 Mick Koch <mick@kochm.co>
//
// This program is free software. It comes without any warranty, to the extent
// permitted by applicable law. You can redistribute it and/or modify it under
// the terms of the Do WTF You Want To Public License, Version 2, as published
// by Sam Hocevar. See http://sam.zoy.org/wtfpl/COPYING for more details.
//

#include <stdlib.h>

void ALIST_INIT(ALIST_T *l) {
    *l = (ALIST_T) {
        .len = 0,
        .slots = {ALIST_SLOT_INIT},
    };
}

ALIST_TYPE *ALIST_PUSH(ALIST_T *l) {
    if (l->len == ALIST_CAP)
        return NULL;

    ALIST_TYPE *slot = &l->slots[l->len];
    l->len += 1;

    return slot;
}

ALIST_TYPE *ALIST_POP(ALIST_T *l) {
    if (!l->len)
        return NULL;

    l->len -= 1;

    return &l->slots[l->len];
}
