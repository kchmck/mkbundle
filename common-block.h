// See copyright notice in Copying.

#ifndef COMMON_BLOCK_H
#define COMMON_BLOCK_H

#define FLAG_DEFAULT 0u

#define FLAG_IS_FRAGMENT (1u << 0)
#define FLAG_ADMIN (1u << 1)
#define FLAG_NO_FRAGMENT (1u << 2)
#define FLAG_CUSTODY (1u << 3)
#define FLAG_SINGLETON (1u << 4)
#define FLAG_ACK (1u << 5)

#define PRIO_RESET ~(0x3u << 7)
#define PRIO_BULK (0x0u << 7)
#define PRIO_NORMAL (0x1u << 7)
#define PRIO_EXPEDITED (0x2u << 7)

#define REPORT_RECEPTION (1u << 14)
#define REPORT_CUSTODY (1u << 15)
#define REPORT_FORWARDING (1u << 16)
#define REPORT_DELIVERY (1u << 17)
#define REPORT_DELETION (1u << 18)

#define FLAG_REPLICATE (1u << 0)
#define FLAG_TRANSMIT_STATUS (1u << 1)
#define FLAG_DELETE_BUNDLE (1u << 2)
#define FLAG_LAST_BLOCK (1u << 3)
#define FLAG_DISCARD_BLOCK (1u << 4)
#define FLAG_FORWARDED (1u << 5)
#define FLAG_CONTAINS_REF (1u << 6)

#define FLAG_INVALID (1u << 31)

#endif
