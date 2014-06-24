// See copyright notice in Copying.

#include <assert.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "common-block.h"
#include "eid.h"
#include "parser.h"
#include "primary-block.h"
#include "sdnv.h"
#include "strbuf.h"
#include "util.h"

#ifdef MKBUNDLE_TEST
#include "greatest.h"
#endif

// A Fowler/Noll/Vo hash function as detailed at
// http://www.isthe.com/chongo/tech/comp/fnv/
static htable_hash_t fnv(const eid_table_str_t *key) {
    // Standard 32-bit offset basis.
    htable_hash_t hval = 0x811c9dc5;

    for (size_t i = 0; i < key->len; i += 1) {
        hval ^= (uint8_t) key->str[i];
        // Standard 32-bit prime.
        hval *= 0x01000193;
    }

    return hval;
}

#include "htable.c"

enum { BUNDLE_VERSION_DEFAULT = 0x06 };

static inline uint32_t calc_length(const primary_block_t *b) {
    return (uint32_t) (
        SDNV_LEN(SWAP32(b->dest.scheme)) + SDNV_LEN(SWAP32(b->dest.ssp)) +
        SDNV_LEN(SWAP32(b->src.scheme)) + SDNV_LEN(SWAP32(b->src.ssp)) +
        SDNV_LEN(SWAP32(b->report_to.scheme)) + SDNV_LEN(SWAP32(b->report_to.ssp)) +
        SDNV_LEN(SWAP32(b->custodian.scheme)) + SDNV_LEN(SWAP32(b->custodian.ssp)) +
        SDNV_LEN(SWAP32(b->creation_ts)) + SDNV_LEN(SWAP32(b->creation_seq)) +
        SDNV_LEN(SWAP32(b->lifetime)) + SDNV_LEN(SWAP64(b->eid_buf->pos)) +
        b->eid_buf->pos
    );
}

#ifdef MKBUNDLE_TEST
TEST test_calc_length(void) {
    eid_t eid = {
        .scheme = 42,
        .ssp = 42,
    };

    primary_block_t block;
    primary_block_init(&block);

    block.dest = eid;
    block.src = eid;
    block.report_to = eid;
    block.custodian = eid;
    block.creation_ts = block.creation_seq = block.lifetime = 42;
    ASSERT_EQ(calc_length(&block), 12);

    block.lifetime = 0xff;
    ASSERT_EQ(calc_length(&block), 13);

    strbuf_append(&block.eid_buf, "a", 1);
    strbuf_finish(&block.eid_buf);
    ASSERT_EQ(calc_length(&block), 15);

    primary_block_destroy(&block);

    PASS();
}
#endif

// Serialize the strings in the buffer into a JSON array.
static void serialize_eids(const strbuf_t *eids, FILE *stream) {
    size_t pos = 0;

    while (pos < eids->pos) {
        const char *eid = &eids->buf[pos];
        fprintf(stream, "    \"%s\"", eid);

        // Move to the next string.
        pos += strlen(eid) + 1;

        // JSON doesn't support trailing commas.
        if (pos < eids->pos)
            fputc(',', stream);

        fputc('\n', stream);
    }
}

void primary_block_serialize(const primary_block_t *b, FILE *stream) {
    fprintf(stream,
        "\"primary\": {\n"
        "  \"version\": %" PRIu8 ",\n"
        "  \"flags\": %" PRIu32 ",\n"
        "  \"length\": %" PRIu32 ",\n"
        "  \"dest\": [%" PRIu32 ", %" PRIu32 "],\n"
        "  \"src\": [%" PRIu32 ", %" PRIu32 "],\n"
        "  \"report-to\": [%" PRIu32 ", %" PRIu32 "],\n"
        "  \"custodian\": [%" PRIu32 ", %" PRIu32 "],\n"
        "  \"creation-ts\": %" PRIu32 ",\n"
        "  \"creation-seq\": %" PRIu32 ",\n"
        "  \"lifetime\": %" PRIu32 ",\n"
        "  \"eids-size\": %zu,\n"
        "  \"eids\": [\n"
        ,
        b->version,
        b->flags,
        calc_length(b),
        b->dest.scheme, b->dest.ssp,
        b->src.scheme, b->src.ssp,
        b->report_to.scheme, b->report_to.ssp,
        b->custodian.scheme, b->custodian.ssp,
        b->creation_ts,
        b->creation_seq,
        b->lifetime,
        b->eid_buf->pos
    );

    serialize_eids(b->eid_buf, stream);

    fputs(
        "  ]\n"
        "}\n"
        ,
        stream
    );
}

static bool parse_eids(strbuf_t **eids, parser_t *p) {
    if (p->cur->type != JSMN_ARRAY)
        return false;

    int eid_count = p->cur->size;

    parser_advance(p);

    for (int i = 0; i < eid_count; i += 1) {
        if (p->cur->type != JSMN_STRING)
            return false;

        strbuf_append(eids, parser_cur_str(p), parser_cur_len(p));
        strbuf_finish(eids);

        parser_advance(p);
    }

    return true;
}

#ifdef MKBUNDLE_TEST
TEST test_parse_eids(void) {
    strbuf_t *buf;
    strbuf_init(&buf, 256);

    parser_t parser;
    parser_init(&parser);

    static const char J[] = "{\"a\": [\"a\", \"b\"]}";
    ASSERT(parser_parse(&parser, J, sizeof(J) - 1));

    ASSERT(parser_advance(&parser));
    ASSERT(parser_advance(&parser));
    ASSERT(parse_eids(&buf, &parser));

    ASSERT_EQ(buf->pos, 4);
    ASSERT_EQ(buf->buf[0], 'a');
    ASSERT_EQ(buf->buf[2], 'b');

    strbuf_destroy(buf);

    PASS();
}
#endif

bool primary_block_unserialize(primary_block_t *b, parser_t *p) {
    enum {
        SYM_VERSION,
        SYM_FLAGS,
        SYM_LENGTH,
        SYM_DEST,
        SYM_SRC,
        SYM_REPORT_TO,
        SYM_CUSTODIAN,
        SYM_CREATION_TS,
        SYM_CREATION_SEQ,
        SYM_LIFETIME,
        SYM_EIDS_SIZE,
        SYM_EIDS,

        SYM_MAX,
        SYM_MASK = (1 << SYM_MAX) - 1,
    };

    static const char *MAP[] = {
        [SYM_VERSION] = "version",
        [SYM_FLAGS] = "flags",
        [SYM_LENGTH] = "length",
        [SYM_DEST] = "dest",
        [SYM_SRC] = "src",
        [SYM_REPORT_TO] = "report-to",
        [SYM_CUSTODIAN] = "custodian",
        [SYM_CREATION_TS] = "creation-ts",
        [SYM_CREATION_SEQ] = "creation-seq",
        [SYM_LIFETIME] = "lifetime",
        [SYM_EIDS_SIZE] = "eids-size",
        [SYM_EIDS] = "eids",
    };

    if (!parser_advance(p))
        return false;

    // Bitmap where each bit represents if a symbol has been visited.
    uint32_t symbols = 0;

    while (parser_more(p)) {
        uint32_t sym = parser_parse_sym(p, MAP, ASIZE(MAP));
        symbols |= 1u << sym;

        switch (sym) {
        case SYM_VERSION:
            b->version = parser_parse_u8(p);
        break;

        case SYM_FLAGS:
            b->flags = parser_parse_u32(p);
        break;

        case SYM_LENGTH:
            b->length = parser_parse_u32(p);
        break;

        case SYM_DEST:
            parser_parse_eid(p, &b->dest);
        break;

        case SYM_SRC:
            parser_parse_eid(p, &b->src);
        break;

        case SYM_REPORT_TO:
            parser_parse_eid(p, &b->report_to);
        break;

        case SYM_CUSTODIAN:
            parser_parse_eid(p, &b->custodian);
        break;

        case SYM_CREATION_TS:
            b->creation_ts = parser_parse_u32(p);
        break;

        case SYM_CREATION_SEQ:
            b->creation_seq = parser_parse_u32(p);
        break;

        case SYM_LIFETIME:
            b->lifetime = parser_parse_u32(p);
        break;

        case SYM_EIDS_SIZE:
            b->eids_size = parser_parse_u32(p);
        break;

        case SYM_EIDS:
            if (!parse_eids(&b->eid_buf, p))
                return false;
        break;

        case SYM_INVALID:
            return false;
        break;
        }

        if (p->error)
            return false;
    }

    return symbols == SYM_MASK;
}

#ifdef MKBUNDLE_TEST
TEST test_primary_block_unserialize(void) {
    primary_block_t block;
    primary_block_init(&block);

    static const char J[] =
        "{\"version\": 42, \"flags\": 42, \"length\": 42, \"dest\": [0, 1],"
        " \"src\": [1, 0], \"report-to\": [0, 1], \"custodian\": [1, 0],"
        " \"creation-ts\": 42, \"creation-seq\": 42, \"lifetime\": 42,"
        " \"eids-size\": 42, \"eids\": [\"a\", \"b\"]}";
    parser_t parser;
    parser_init(&parser);
    ASSERT(parser_parse(&parser, J, sizeof(J) - 1));
    ASSERT(primary_block_unserialize(&block, &parser));

    primary_block_destroy(&block);

    PASS();
}
#endif

void primary_block_init(primary_block_t *b) {
    *b = (primary_block_t) {
        .version = BUNDLE_VERSION_DEFAULT,
        .flags = FLAG_DEFAULT,
    };

    eid_map_init(&b->eid_map);
    strbuf_init(&b->eid_buf, 1 << 8);
}

void primary_block_destroy(primary_block_t *b) {
    strbuf_destroy(b->eid_buf);
    eid_map_destroy(b->eid_map);
}

void primary_block_write(const primary_block_t *b, FILE *stream) {
    WRITE(stream, &b->version, sizeof(b->version));
    WRITE_SDNV(stream, SWAP32(b->flags));
    WRITE_SDNV(stream, SWAP32(b->length));

    WRITE_EID(stream, &b->dest);
    WRITE_EID(stream, &b->src);
    WRITE_EID(stream, &b->report_to);
    WRITE_EID(stream, &b->custodian);

    WRITE_SDNV(stream, SWAP32(b->creation_ts));
    WRITE_SDNV(stream, SWAP32(b->creation_seq));
    WRITE_SDNV(stream, SWAP32(b->lifetime));
    WRITE_SDNV(stream, SWAP32(b->eids_size));

    WRITE(stream, b->eid_buf->buf, b->eid_buf->pos);
}

static size_t add_eid(primary_block_t *b, const char *str, size_t len) {
    const eid_table_str_t s = {
        .str = str,
        .len = len,
    };

    size_t *pos = eid_map_lookup(b->eid_map, &s);

    if (pos)
        return *pos;

    pos = eid_map_add(&b->eid_map, &s);
    assert(pos);

    *pos = b->eid_buf->pos;

    strbuf_append(&b->eid_buf, str, len);
    strbuf_finish(&b->eid_buf);

    return *pos;
}

#ifdef MKBUNDLE_TEST
TEST test_add_eid(void) {
    primary_block_t block;
    primary_block_init(&block);

    ASSERT(add_eid(&block, "ab", 2) == 0);
    ASSERT(add_eid(&block, "cd", 2) == 3);

    ASSERT(add_eid(&block, "ab", 2) == 0);
    ASSERT(add_eid(&block, "cd", 2) == 3);

    primary_block_destroy(&block);

    PASS();
}
#endif

bool primary_block_add_eid(primary_block_t *b, eid_t *e, const char *str) {
    const char *sep = strchr(str, ':');

    if (!sep)
        return false;

    *e = (eid_t) {
        .scheme = (uint32_t) add_eid(b, str, (size_t)(sep - str)),
        .ssp = (uint32_t) add_eid(b, sep + 1, strlen(sep + 1)),
    };

    return true;
}

#ifdef MKBUNDLE_TEST
TEST test_primary_block_add_eid(void) {
    primary_block_t block;
    primary_block_init(&block);

    ASSERT(!primary_block_add_eid(&block, &block.dest, "a"));

    ASSERT(primary_block_add_eid(&block, &block.dest, "a:"));
    ASSERT(block.dest.scheme == 0);
    ASSERT(block.dest.ssp == 2);

    ASSERT(primary_block_add_eid(&block, &block.dest, ":b"));
    ASSERT(block.dest.scheme == 2);
    ASSERT(block.dest.ssp == 3);

    ASSERT(primary_block_add_eid(&block, &block.dest, "c:d"));
    ASSERT(block.dest.scheme == 5);
    ASSERT(block.dest.ssp == 7);

    ASSERT(primary_block_add_eid(&block, &block.dest, "a:d"));
    ASSERT(block.dest.scheme == 0);
    ASSERT(block.dest.ssp == 7);

    primary_block_destroy(&block);

    PASS();
}
#endif

#ifdef MKBUNDLE_TEST
SUITE(primary_block_suite) {
    RUN_TEST(test_calc_length);
    RUN_TEST(test_parse_eids);
    RUN_TEST(test_primary_block_unserialize);
    RUN_TEST(test_add_eid);
    RUN_TEST(test_primary_block_add_eid);
}
#endif
