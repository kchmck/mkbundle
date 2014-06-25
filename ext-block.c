// See copyright notice in Copying.

#include <assert.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "eid.h"
#include "ext-block.h"
#include "parser.h"
#include "util.h"

#ifdef MKBUNDLE_TEST
#include "greatest.h"
#endif

#include "alist.c"

void ext_block_init(ext_block_t *b) {
    *b = (ext_block_t) {
        .ref_count = 0,
    };

    eid_refs_init(&b->refs);
}

static void serialize_refs(const ext_block_t *b, FILE *stream) {
    for (size_t i = 0; i < b->refs.len; i += 1) {
        fprintf(stream,
            "    [%" PRIu32 ", %" PRIu32 "]"
            ,
            b->refs.slots[i].scheme,
            b->refs.slots[i].ssp
        );

        if (i < b->refs.len - 1)
            fputc(',', stream);

        fputc('\n', stream);
    }
}

#ifdef MKBUNDLE_TEST
TEST test_serialize_refs(void) {
    ext_block_t block;
    ext_block_init(&block);

    FILE *f = fopen("test", "w+");
    char buf[256] = {0};

    rewind(f);
    serialize_refs(&block, f);

    rewind(f);
    fread(buf, sizeof(buf[0]), sizeof(buf), f);
    ASSERT_STR_EQ(buf, "");

    {
        eid_t *eid = eid_refs_push(&block.refs);
        eid->scheme = 42;
        eid->ssp = 84;
    }

    rewind(f);
    serialize_refs(&block, f);

    rewind(f);
    fread(buf, sizeof(char), sizeof(buf), f);
    ASSERT_STR_EQ(buf, "    [42, 84]\n");

    {
        eid_t *eid = eid_refs_push(&block.refs);
        eid->scheme = 4294967295u;
        eid->ssp = 4294967295u;
    }

    rewind(f);
    serialize_refs(&block, f);

    rewind(f);
    fread(buf, sizeof(char), sizeof(buf), f);
    ASSERT_STR_EQ(buf,
        "    [42, 84],\n"
        "    [4294967295, 4294967295]\n");

    fclose(f);

    PASS();
}
#endif

void ext_block_serialize(const ext_block_t *b, FILE *stream) {
    fprintf(stream,
        "\"extension\": {\n"
        "  \"type\": %" PRIu8 ",\n"
        "  \"flags\": %" PRIu8 ",\n"
        "  \"payload-length\": %" PRIu32 ",\n"
        "  \"ref-count\": %" PRIu32 ",\n"
        "  \"refs\": [\n"
        ,
        b->type,
        b->flags,
        b->length,
        b->ref_count
    );

    serialize_refs(b, stream);

    fputs(
        "  ]\n"
        "}\n"
        ,
        stream
    );
}

static bool parse_refs(ext_block_t *b, parser_t *p) {
    if (p->cur->type != JSMN_ARRAY)
        return false;

    int ref_count = p->cur->size;

    parser_advance(p);

    for (int i = 0; i < ref_count; i += 1) {
        eid_t *eid = eid_refs_push(&b->refs);

        if (!eid)
            return false;

        parser_parse_eid(p, eid);
    }

    return true;
}

#ifdef MKBUNDLE_TEST
TEST test_parse_refs(void) {
    ext_block_t block;
    ext_block_init(&block);

    parser_t parser;
    parser_init(&parser);

    static const char J[] = "[[3, 2], [1, 0]]";
    ASSERT(parser_parse(&parser, J, sizeof(J)));

    ASSERT(parse_refs(&block, &parser));
    ASSERT(block.refs.len == 2);
    ASSERT(block.refs.slots[0].scheme == 3);
    ASSERT(block.refs.slots[0].ssp == 2);
    ASSERT(block.refs.slots[1].scheme == 1);
    ASSERT(block.refs.slots[1].ssp == 0);

    PASS();
}
#endif

bool ext_block_unserialize(ext_block_t *b, parser_t *p) {
    enum {
        SYM_TYPE,
        SYM_FLAGS,
        SYM_LENGTH,
        SYM_REF_COUNT,
        SYM_REFS,

        SYM_MAX,
        SYM_MASK = (1 << SYM_MAX) - 1
    };

    static const char *MAP[] = {
        [SYM_TYPE] = "type",
        [SYM_FLAGS] = "flags",
        [SYM_LENGTH] = "payload-length",
        [SYM_REF_COUNT] = "ref-count",
        [SYM_REFS] = "refs",
    };

    if (!parser_advance(p))
        return false;

    uint32_t symbols = 0;

    while (parser_more(p)) {
        uint32_t sym = parser_parse_sym(p, MAP, ASIZE(MAP));
        symbols |= 1u << sym;

        switch (sym) {
        case SYM_TYPE:
            b->type = parser_parse_u8(p);
        break;

        case SYM_FLAGS:
            b->flags = parser_parse_u8(p);
        break;

        case SYM_LENGTH:
            b->length = parser_parse_u32(p);
        break;

        case SYM_REF_COUNT:
            b->ref_count = parser_parse_u32(p);
        break;

        case SYM_REFS:
            if (!parse_refs(b, p))
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

void ext_block_write(const ext_block_t *b, FILE *stream) {
    WRITE(stream, &b->type, sizeof(b->type));
    WRITE_SDNV(stream, b->flags);

    if (b->ref_count) {
        WRITE_SDNV(stream, SWAP32(b->ref_count));

        for (size_t i = 0; i < b->refs.len; i += 1)
            WRITE_EID(stream, &b->refs.slots[i]);
    }

    WRITE_SDNV(stream, SWAP32(b->length));
}

static bool parse_ref(eid_t *e, const char *str) {
    const char *sep = strchr(str, ':');

    if (!sep)
        return false;

    char *end;

    {
        const char *start = str;
        e->scheme = (uint32_t) strtoul(start, &end, 10);

        if (end == start)
            return false;
    }

    {
        const char *start = sep + 1;
        e->ssp = (uint32_t) strtoul(start, &end, 10);

        if (end == start)
            return false;
    }

    return true;
}

#ifdef MKBUNDLE_TEST
TEST test_parse_ref(void) {
    eid_t eid = {0, 0};

    ASSERT(!parse_ref(&eid, "4"));
    ASSERT(!parse_ref(&eid, ":2"));
    ASSERT(!parse_ref(&eid, "4:"));

    ASSERT(!parse_ref(&eid, "a:2"));
    ASSERT(!parse_ref(&eid, "4:a"));

    ASSERT(parse_ref(&eid, "4:2"));
    ASSERT(eid.scheme == 4);
    ASSERT(eid.ssp == 2);

    PASS();
}
#endif

bool ext_block_add_ref(ext_block_t *b, const char *str) {
    eid_t *eid = eid_refs_push(&b->refs);

    if (!eid)
        return false;

    return parse_ref(eid, str);
}

#ifdef MKBUNDLE_TEST
TEST test_ext_block_add_ref(void) {
    ext_block_t block;
    ext_block_init(&block);

    ASSERT(ext_block_add_ref(&block, "4:2"));
    ASSERT(block.refs.len == 1);
    ASSERT(block.refs.slots[0].scheme == 4);
    ASSERT(block.refs.slots[0].ssp == 2);

    ASSERT(ext_block_add_ref(&block, "0:1"));
    ASSERT(block.refs.len == 2);
    ASSERT(block.refs.slots[1].scheme == 0);
    ASSERT(block.refs.slots[1].ssp == 1);

    PASS();
}
#endif

#ifdef MKBUNDLE_TEST
SUITE(ext_block_suite) {
    RUN_TEST(test_serialize_refs);
    RUN_TEST(test_parse_refs);
    RUN_TEST(test_parse_ref);
    RUN_TEST(test_ext_block_add_ref);
}
#endif
