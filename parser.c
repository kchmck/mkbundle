// See copyright notice in Copying.

#include <assert.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "eid.h"
#include "jsmn.h"
#include "parser.h"
#include "util.h"

#ifdef MKBUNDLE_TEST
#include "greatest.h"
#endif

void parser_init(parser_t *p) {
    *p = (parser_t) {
        .tokens = {{0, 0, 0, 0}},
        .error = false,
    };
}

bool parser_parse(parser_t *p, const char *src, size_t len) {
    jsmn_parser jsmn;
    jsmn_init(&jsmn);

    int ret = jsmn_parse(&jsmn, src, len, p->tokens, ASIZE(p->tokens));

    if (ret <= 0)
        return false;

    p->token = 0;
    p->token_count = (size_t) ret;
    p->src = src;

    parser_advance(p);

    return true;
}

#ifdef MKBUNDLE_TEST
TEST test_parse(void) {
    parser_t parser;
    parser_init(&parser);

    ASSERT(!parser_parse(&parser, "", 0));
    ASSERT(parser_parse(&parser, "[]", 2));
    ASSERT(parser_parse(&parser, "{}", 2));

    static const char J[] =  "{\"version\": 0}";
    ASSERT(parser_parse(&parser, J, sizeof(J) - 1));
    ASSERT_EQ(parser.token_count, 3);
    ASSERT_EQ(parser.cur->type, JSMN_OBJECT);
    ASSERT_EQ(parser.src, J);

    PASS();
}
#endif

// Set the current token pointer.
bool parser_advance(parser_t *p) {
    if (!parser_more(p))
        return false;

    p->cur = &p->tokens[p->token];
    p->token += 1;

    return true;
}

#ifdef MKBUNDLE_TEST
TEST test_advance(void) {
    parser_t parser;
    parser_init(&parser);

    static const char J[] = "{\"a\": 0}";
    ASSERT(parser_parse(&parser, J, sizeof(J) - 1));

    ASSERT(parser_advance(&parser));
    ASSERT(parser_advance(&parser));
    ASSERT(!parser_advance(&parser));

    PASS();
}
#endif

const char *parser_cur_str(const parser_t *p) {
    return &p->src[p->cur->start];
}

size_t parser_cur_len(const parser_t *p) {
    return (size_t)(p->cur->end - p->cur->start);
}

#ifdef MKBUNDLE_TEST
TEST test_cur(void) {
    parser_t parser;
    parser_init(&parser);

    static const char J[] = "{\"a\": \"bcd\"}";
    ASSERT(parser_parse(&parser, J, sizeof(J) - 1));

    ASSERT(parser_advance(&parser));
    ASSERT_EQ(parser_cur_len(&parser), 1);
    ASSERT_EQ(strncmp(parser_cur_str(&parser), "a", 1), 0);

    ASSERT(parser_advance(&parser));
    ASSERT_EQ(parser_cur_len(&parser), 3);
    ASSERT_EQ(strncmp(parser_cur_str(&parser), "bcd", 3), 0);

    PASS();
}
#endif

uint32_t parser_parse_sym(parser_t *p, const char **syms, size_t sym_count) {
    if (p->cur->type != JSMN_STRING) {
        p->error = true;
        return 0;
    }

    for (size_t sym = 0; sym < sym_count; sym += 1) {
        if (strlen(syms[sym]) == parser_cur_len(p) &&
            strncmp(syms[sym], parser_cur_str(p), parser_cur_len(p) ) == 0)
        {
            parser_advance(p);
            return (uint32_t) sym;
        }
    }

    return SYM_INVALID;
}

#ifdef MKBUNDLE_TEST
TEST test_parse_sym(void) {
    parser_t parser;
    parser_init(&parser);

    enum { SYM1, SYM2, };
    static const char *MAP[] = {
        [SYM1] = "sym1",
        [SYM2] = "sym2",
    };

    static const char J[] =
        "{\"a\": \"sym2\", \"b\": \"wrong-sym\", \"c\": \"sym1\"}";
    ASSERT(parser_parse(&parser, J, sizeof(J) - 1));

    ASSERT(parser_advance(&parser));
    ASSERT(parser_advance(&parser));
    ASSERT_EQ(parser_parse_sym(&parser, MAP, ASIZE(MAP)), SYM2);

    ASSERT(parser_advance(&parser));
    ASSERT_EQ(parser_parse_sym(&parser, MAP, ASIZE(MAP)), SYM_INVALID);

    ASSERT(parser_advance(&parser));
    ASSERT(parser_advance(&parser));
    ASSERT_EQ(parser_parse_sym(&parser, MAP, ASIZE(MAP)), SYM1);

    PASS();
}
#endif

uint32_t parser_parse_u32(parser_t *p) {
    if (p->cur->type != JSMN_PRIMITIVE) {
        p->error = true;
        return 0;
    }

    const char *start = parser_cur_str(p);
    char *end;

    unsigned long long val = strtoull(start, &end, 10);

    if (end == start || val > UINT32_MAX) {
        p->error = true;
        return 0;
    }

    parser_advance(p);

    return (uint32_t) val;
}

#ifdef MKBUNDLE_TEST
TEST test_parse_u32(void) {
    parser_t parser;
    parser_init(&parser);

    static const char J[] =
        "{\"a\": 0, \"b\": -1, \"c\": 4294967295}";
    ASSERT(parser_parse(&parser, J, sizeof(J) - 1));

    ASSERT(parser_advance(&parser));
    ASSERT(parser_advance(&parser));
    ASSERT_EQ(parser_parse_u32(&parser), 0);

    ASSERT(parser_advance(&parser));
    parser_parse_u32(&parser);
    ASSERT(parser.error);
    parser.error = false;

    ASSERT(parser_advance(&parser));
    ASSERT(parser_advance(&parser));
    ASSERT_EQ(parser_parse_u32(&parser), 4294967295);

    PASS();
}
#endif

uint8_t parser_parse_u8(parser_t *p) {
    uint32_t val = parser_parse_u32(p);

    if (val > UINT8_MAX) {
        p->error = true;
        return 0;
    }

    return (uint8_t) val;
}

#ifdef MKBUNDLE_TEST
TEST test_parse_u8(void) {
    parser_t parser;
    parser_init(&parser);

    static const char J[] = "{\"a\": 0, \"b\": 256, \"c\": 255}";
    ASSERT(parser_parse(&parser, J, sizeof(J) - 1));

    ASSERT(parser_advance(&parser));
    ASSERT(parser_advance(&parser));
    ASSERT_EQ(parser_parse_u8(&parser), 0);

    ASSERT(parser_advance(&parser));
    parser_parse_u8(&parser);
    ASSERT(parser.error);
    parser.error = false;

    ASSERT(parser_advance(&parser));
    ASSERT_EQ(parser_parse_u8(&parser), 255);

    PASS();
}
#endif

bool parser_parse_eid(parser_t *p, eid_t *eid) {
    if (p->cur->type != JSMN_ARRAY || !parser_advance(p))
        return false;

    *eid = (eid_t) {
        .scheme = parser_parse_u32(p),
        .ssp = parser_parse_u32(p),
    };

    return !p->error;
}

#ifdef MKBUNDLE_TEST
TEST test_parse_eid(void) {
    parser_t parser;
    parser_init(&parser);

    static const char J[] = "{\"a\": [42, 0]}";
    ASSERT(parser_parse(&parser, J, sizeof(J) - 1));

    ASSERT(parser_advance(&parser));
    ASSERT(parser_advance(&parser));

    eid_t eid;
    parser_parse_eid(&parser, &eid);

    ASSERT_EQ(eid.scheme, 42);
    ASSERT_EQ(eid.ssp, 0);

    PASS();
}
#endif

#ifdef MKBUNDLE_TEST
SUITE(parser_suite) {
    RUN_TEST(test_parse);
    RUN_TEST(test_advance);
    RUN_TEST(test_cur);
    RUN_TEST(test_parse_sym);
    RUN_TEST(test_parse_u32);
    RUN_TEST(test_parse_u8);
    RUN_TEST(test_parse_eid);
}
#endif
