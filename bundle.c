// See copyright notice in Copying.

#include <inttypes.h>
#include <stdio.h>

#include "block.h"
#include "bundle.h"
#include "util.h"
#include "sdnv.h"

void bundle_params_init(bundle_params_t *p) {
    *p = (bundle_params_t) {
        .version = BUNDLE_VERSION_DEFAULT,
        .flags = FLAG_DEFAULT,
    };

    strbuf_init(&p->dict, 1 << 8);
}

void bundle_params_destroy(bundle_params_t *p) {
    strbuf_destroy(p->dict);
}

static void eid_build(eid_t *e, const eid_param_t *params) {
    e->scheme = SDNV_ENCODE(SWAP32(params->scheme));
    e->ssp = SDNV_ENCODE(SWAP32(params->ssp));
}

static void eid_destroy(eid_t *e) {
    sdnv_destroy(e->scheme);
    sdnv_destroy(e->ssp);
}

static inline uint32_t bundle_length(const bundle_t *b) {
    return b->dest.scheme->len + b->dest.ssp->len +
           b->src.scheme->len + b->src.ssp->len +
           b->report_to.scheme->len + b->report_to.ssp->len +
           b->custodian.scheme->len + b->custodian.ssp->len +
           b->creation_ts->len + b->creation_seq->len +
           b->lifetime->len +
           b->dict_len->len +
           b->params->dict->pos;
}

void bundle_build(bundle_t *b) {
    eid_build(&b->dest, &b->params->dest);
    eid_build(&b->src, &b->params->src);
    eid_build(&b->report_to, &b->params->report_to);
    eid_build(&b->custodian, &b->params->custodian);

    b->flags = SDNV_ENCODE(SWAP32(b->params->flags));
    b->creation_ts = SDNV_ENCODE(SWAP32(b->params->creation_ts));
    b->creation_seq = SDNV_ENCODE(SWAP32(b->params->creation_seq));
    b->lifetime = SDNV_ENCODE(SWAP32(b->params->lifetime));
    b->dict_len = SDNV_ENCODE(SWAP64(b->params->dict->pos));

    b->length = SDNV_ENCODE(SWAP64(bundle_length(b)));
}

void bundle_init(bundle_t *b, const bundle_params_t *params,
                 const block_t *block)
{
    *b = (bundle_t) {
        .params = params,
        .block = block,
    };
}

void bundle_destroy(bundle_t *b) {
    sdnv_destroy(b->flags);
    sdnv_destroy(b->length);

    eid_destroy(&b->dest);
    eid_destroy(&b->src);
    eid_destroy(&b->report_to);
    eid_destroy(&b->custodian);

    sdnv_destroy(b->creation_ts);
    sdnv_destroy(b->creation_seq);
    sdnv_destroy(b->lifetime);

    sdnv_destroy(b->dict_len);
}

void bundle_write(const bundle_t *b, FILE *stream) {
#define WRITE_EID(eid) do { \
    WRITE_SDNV(stream, (eid)->scheme); \
    WRITE_SDNV(stream, (eid)->ssp); \
} while(0)

    WRITE(stream, &b->params->version, sizeof(b->params->version));
    WRITE_SDNV(stream, b->flags);
    WRITE_SDNV(stream, b->length);

    WRITE_EID(&b->dest);
    WRITE_EID(&b->src);
    WRITE_EID(&b->report_to);
    WRITE_EID(&b->custodian);

    WRITE_SDNV(stream, b->creation_ts);
    WRITE_SDNV(stream, b->creation_seq);
    WRITE_SDNV(stream, b->lifetime);
    WRITE_SDNV(stream, b->dict_len);

    WRITE(stream, b->params->dict->buf, b->params->dict->pos);
    block_write(b->block, stream);

#undef WRITE_EID
}
