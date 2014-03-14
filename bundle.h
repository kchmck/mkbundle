// See copyright notice in Copying.

#ifndef BUNDLE_H
#define BUNDLE_H

#include <stdio.h>
#include <inttypes.h>

#include "block.h"
#include "sdnv.h"
#include "strbuf.h"

enum { BUNDLE_VERSION_DEFAULT = 0x06};

typedef enum {
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
} flag_t;

typedef struct {
    uint32_t scheme;
    uint32_t ssp;
} eid_param_t;

typedef struct {
    uint8_t version;
    flag_t flags;

    eid_param_t dest;
    eid_param_t src;
    eid_param_t report_to;
    eid_param_t custodian;

    uint32_t creation_ts;
    uint32_t creation_seq;
    uint32_t lifetime;

    strbuf_t *dict;
} bundle_params_t;

void bundle_params_init(bundle_params_t *p);

void bundle_params_destroy(bundle_params_t *p);

typedef struct {
    sdnv_t *scheme;
    sdnv_t *ssp;
} eid_t;

typedef struct {
    const bundle_params_t *params;

    eid_t dest;
    eid_t src;
    eid_t report_to;
    eid_t custodian;

    sdnv_t *flags;
    sdnv_t *creation_ts;
    sdnv_t *creation_seq;
    sdnv_t *lifetime;
    sdnv_t *dict_len;
    sdnv_t *length;

    const block_t *block;
} bundle_t;

void bundle_init(bundle_t *b, const bundle_params_t *params,
                 const block_t *block);

void bundle_destroy(bundle_t *b);

void bundle_build(bundle_t *b);

void bundle_write(const bundle_t *b, FILE *stream);

#endif
