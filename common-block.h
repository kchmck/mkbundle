// See copyright notice in Copying.

#ifndef COMMON_BLOCK_H
#define COMMON_BLOCK_H

enum {
    FLAG_DEFAULT = 0,

    FLAG_IS_FRAGMENT = 1 << 0,
    FLAG_ADMIN = 1 << 1,
    FLAG_NO_FRAGMENT = 1 << 2,
    FLAG_CUSTODY = 1 << 3,
    FLAG_SINGLETON = 1 << 4,
    FLAG_ACK = 1 << 5,

    PRIO_RESET = ~(0x3 << 7),
    PRIO_BULK = 0x0 << 7,
    PRIO_NORMAL = 0x1 << 7,
    PRIO_EXPEDITED = 0x2 << 7,

    REPORT_RECEPTION = 1 << 14,
    REPORT_CUSTODY = 1 << 15,
    REPORT_FORWARDING = 1 << 16,
    REPORT_DELIVERY = 1 << 17,
    REPORT_DELETION = 1 << 18,

    FLAG_INVALID = 1 << 31,
};

#endif
